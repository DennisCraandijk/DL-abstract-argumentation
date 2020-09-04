package net.sf.probo2.exec;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Random;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

import net.sf.probo2.config.Configuration;
import net.sf.probo2.log.LogFile;
import net.sf.tweety.arg.dung.parser.FileFormat;
import net.sf.tweety.arg.dung.parser.TgfParser;
import net.sf.tweety.arg.dung.semantics.Problem;
import net.sf.tweety.arg.dung.syntax.Argument;
import net.sf.tweety.commons.ParserException;
import net.sf.tweety.commons.util.Pair;
import net.sf.tweety.commons.util.Triple;

/**
 * This is the main class for performing benchmarks of argumentation solvers.
 * 
 * @author Matthias Thimm
 */
public abstract class Probo2_RunBenchmark {
	
	/**
	 * For system calls.
	 */
	private static class SolverCallee implements Callable<Pair<String,String>>{
		private String commandline;
		private Configuration config;
		public SolverCallee(String commandline, Configuration config){
			this.commandline = commandline;			
			this.config = config;
		}
		@Override
		public Pair<String,String> call() throws Exception {
			Process child = null;
			BufferedReader reader = null;
			try{
				child = Runtime.getRuntime().exec(commandline);
				child.waitFor();		
				String output = "";
				reader = new BufferedReader(new InputStreamReader(child.getInputStream()));
				String line = "";			
				while((line = reader.readLine())!= null) {
					output += line + "\n";
				}
				reader.close();
				// check for errors
				reader = new BufferedReader(new InputStreamReader(child.getErrorStream())); 
				line = "";		
				String error = "";
				while((line = reader.readLine())!= null) {
					error += line + "\n";
				}
				reader.close();
				child.destroy();
				error.trim();
				return new Pair<String,String>(output,error);
			}finally{
				if(child!= null) child.destroyForcibly();
				Probo2_RunBenchmark.cleanUp(config);
			}			
		}		
	}	
	
	/**
	 * For handling timeouts.
	 */
	private static class SolverCalleeWithTime implements Callable<Triple<String,String,Long>>{
		private String commandline;
		private Configuration config;
		public SolverCalleeWithTime(String commandline, Configuration config){
			this.commandline = commandline;			
			this.config = config;
		}
		@Override
		public Triple<String,String,Long> call() throws Exception {
			Process child = null;
			BufferedReader reader = null;
			try{
				child = Runtime.getRuntime().exec("time -p " + commandline);
				child.waitFor();		
				String output = "";
				reader = new BufferedReader(new InputStreamReader(child.getInputStream()));
				String line = "";			
				while((line = reader.readLine())!= null) {
					output += line + "\n";
				}
				reader.close();
				// check for errors
				reader = new BufferedReader(new InputStreamReader(child.getErrorStream())); 
				line = "";		
				String error = "";
				while((line = reader.readLine())!= null) {
					error += line + "\n";
				}
				reader.close();
				child.destroy();
				error.trim();
				String[] lines = error.split("\n");
				Double sys = new Double(lines[lines.length-2].substring(4).trim())*1000;
				Double user = new Double(lines[lines.length-1].substring(3).trim())*1000;
				error = "";
				for(int i = 0; i < lines.length-3; i++)
					error += lines[i] + "\n";
				error.trim();
				return new Triple<String,String,Long>(output,error,Math.round(sys+user));
			}finally{
				if(child!= null) child.destroyForcibly();
				Probo2_RunBenchmark.cleanUp(config);
			}
		}		
	}

	/** Get this process's id.
	 * @return this process's id.
	 */
	public static long getPID() {
	    String processName =
	      java.lang.management.ManagementFactory.getRuntimeMXBean().getName();
	    return Long.parseLong(processName.split("@")[0]);
	  }
	
	/**
	 * Cleans up the mess from solvers (such as lingering java processes)
	 * @throws IOException
	 * @throws InterruptedException
	 */
	public static void cleanUp(Configuration config) throws IOException, InterruptedException{
		long pid = getPID();
		Process child = null;
		Process child2 = null;
		BufferedReader reader = null;
		String[] cmd = { "/bin/sh", "-c", "ps -e | grep java" };
		child = Runtime.getRuntime().exec(cmd);
		child.waitFor();	
		reader = new BufferedReader(new InputStreamReader(child.getInputStream()));
		String line = "";			
		while((line = reader.readLine())!= null) {
			line = line.trim();
			long other = new Long(line.substring(0, line.indexOf(" ")));
			if(other != pid){
				child2 = Runtime.getRuntime().exec("kill -q " + other);
				child2.waitFor();		
				child2.destroy();
			}			
		}
		reader.close();
		child.destroy();		
		// clean up other processes that seem not to be killed sometimes		
		for(String p: config.KILLPROCESSES){
			child = Runtime.getRuntime().exec("killall -q " + p);
			child.waitFor();	
			child.destroy();
		}
	}
	
	/**
	 * Executes the given command on the commandline and returns the output.
	 * @param commandline some command
	 * @return the output of the execution
	 * @throws IOException
	 * @throws InterruptedException 
	 */
	private static Pair<String,String> invokeExecutable(String commandline,Configuration config ) throws IOException, InterruptedException{
		ExecutorService executor = Executors.newSingleThreadExecutor();
		Future<Pair<String,String>> future = null;;
		try{
			// handle timeout				
			future = executor.submit(new SolverCallee(commandline,config));
			Pair<String,String> result = future.get(config.TIMEOUT, TimeUnit.SECONDS);
			executor.shutdownNow();			
		    return result; 
		} catch (TimeoutException | ExecutionException e) {
			// null string indicates that a timeout has occurred
			if(future != null)
				future.cancel(true);
			executor.shutdownNow();
			return null;
		}
	}
	
	/**
	 * Executes the given command on the commandline and returns the output.
	 * @param commandline some command
	 * @return the output of the execution
	 * @throws IOException
	 * @throws InterruptedException 
	 */
	private static Triple<String,String,Long> invokeExecutableAndGetTime(String commandline, Configuration config) throws IOException, InterruptedException{
		ExecutorService executor = Executors.newSingleThreadExecutor();
		Future<Triple<String,String,Long>> future = null;
		try{
			// handle timeout				
			future = executor.submit(new SolverCalleeWithTime(commandline, config));
			Triple<String,String,Long> result = future.get(config.TIMEOUT, TimeUnit.SECONDS);
			executor.shutdownNow();
		    return result; 
		} catch (TimeoutException | ExecutionException e) {
			// null string indicates that a timeout has occurred
			if(future != null)
				future.cancel(true);
			executor.shutdownNow();
			return null;
		}
	}
	/**
	 * Returns a map that maps all solvers to their corresponding supported problems.
	 * @return a map that maps all solvers to their corresponding supported problems.
	 * @throws IOException 
	 * @throws InterruptedException 
	 */
	private static Map<String,Collection<Problem>> supportedProblems(Configuration config, LogFile log) throws IOException, InterruptedException{
		Map<String,Collection<Problem>> map = new HashMap<String,Collection<Problem>>(); 
		for(String solver: config.SOLVERS){
			try{
			Pair<String,String> result = Probo2_RunBenchmark.invokeExecutable(solver + " --problems",config);
			map.put(solver, Problem.getProblems(result.getFirst()));
			if(result.getSecond() != "")
				log.logGeneralError(solver, result.getSecond());
			}catch(Exception e){
				throw new RuntimeException("Solver " + solver + " provided wrong problem specification");
			}			
		}
		return map;
	}
	
	/**
	 * Returns a map that maps all solvers to one of their supported file formats.
	 * @return a map that maps all solvers to one of their supported file formats.
	 * @throws IOException 
	 * @throws InterruptedException 
	 */
	private static Map<String,FileFormat> supportedFormat(Configuration config, LogFile log) throws IOException, InterruptedException{
		Map<String,FileFormat> map = new HashMap<String,FileFormat>();
		for(String solver: config.SOLVERS){
			Pair<String,String> result = Probo2_RunBenchmark.invokeExecutable(solver + " --formats", config);
			Collection<FileFormat> formats = FileFormat.getFileFormats(result.getFirst());
			//Ignore CNF
			formats.remove(FileFormat.CNF);			
			map.put(solver, formats.iterator().next());
			if(result.getSecond() != "")
				log.logGeneralError(solver, result.getSecond());
		}
		return map;
	}
	
	/**
	 * Samples for each file the given number of arguments that will be queried in DC/DS queries (at random)
	 * @param config
	 * @param the number of arguments to be sampled.
	 * @return
	 * @throws ParserException
	 * @throws IOException
	 */
	private static Map<String,Collection<String>> sampleAdditionalArguments(Configuration config, int num) throws ParserException, IOException{
		Random rand = new Random();
		TgfParser parser = new TgfParser();
		Map<String,Collection<String>> result = new HashMap<String,Collection<String>>();
		List<Argument> arguments = new LinkedList<Argument>();
		for(String tgfFile: config.FILES_TGF){
			arguments.clear();
			List<String> argNames = new LinkedList<String>();
			arguments.addAll(parser.parseBeliefBaseFromFile(tgfFile));			
			for(int i = 0; i < num; i++)
				argNames.add(arguments.get(rand.nextInt(arguments.size())).getName());
			result.put(tgfFile, argNames);
		}
		return result;
	}	
		
	/**
	 * Main benchmark method
	 * @param args program parameters 
	 */
	public static void main(String[] args) throws IOException, InterruptedException{
		args = new String[1];
		args[0] = "sampleConfiguration2.yaml";
		
		// load configuration file
		if (args.length != 1){
			System.err.println("Usage:");
			System.err.println("probo2-run-benchmark <configuration file>");
			System.exit(1);
		}
		System.out.println("probo2-run-benchmark started.");
		Configuration config = Configuration.getConfiguration(args[0]);
		if(config == null){					
			System.exit(1);
			System.err.println("Error: configuration not found");
		}
		System.out.println("configuration loaded.");
		LogFile log = new LogFile(config.OUTPUT);
		System.out.println("logfile " + config.OUTPUT + " created.");
		// get from all solvers their supported problems
		Map<String,Collection<Problem>> supportedProblems = Probo2_RunBenchmark.supportedProblems(config,log);
		System.out.println("supported problems of solvers collected.");
		// get from all solvers one of their supported formats
		Map<String,FileFormat> supportedFormat = Probo2_RunBenchmark.supportedFormat(config,log);
		System.out.println("supported file formats of solvers collected.");
		// if a DC problem is to be solved determine arguments
		Map<String,Collection<String>> additionalArgumentsForDC = new HashMap<String,Collection<String>>();
		if(config.hasDCProblem()){
			additionalArgumentsForDC = Probo2_RunBenchmark.sampleAdditionalArguments(config, config.DC_NUMBEROFARGUMENTS);
			System.out.println("arguments for DC determined.");
		}
		// if a DS problem is to be solved determine arguments
		Map<String,Collection<String>> additionalArgumentsForDS = new HashMap<String,Collection<String>>();
		if(config.hasDCProblem()){
			additionalArgumentsForDS = Probo2_RunBenchmark.sampleAdditionalArguments(config, config.DS_NUMBEROFARGUMENTS);
			System.out.println("arguments for DS determined.");
		}
		// Do benchmark
		System.out.println("benchmark started.");
		for(int it = 1; it <= config.REPETITIONS; it++){
			log.logStartRepetition(it);			
			for(Problem problem: config.PROBLEMS){
				log.logStartProblem(problem);
				Collection<String> supportingSolvers = new HashSet<String>();
				for(String solver: config.SOLVERS){
					if(supportedProblems.get(solver).contains(problem)){
						supportingSolvers.add(solver);
						log.logSupportingSolver(solver);
					}
				}	
				for(int i = 0; i < config.FILES_APX.size(); i++){
					String apxFile = config.FILES_APX.get(i);
					String tgfFile = config.FILES_TGF.get(i);
					for(String solver: supportingSolvers){					
						if(problem.subProblem().equals(Problem.SubProblem.DC)){
							for(String arg: additionalArgumentsForDC.get(tgfFile)){
								String file = supportedFormat.get(solver).equals(FileFormat.APX) ? apxFile : tgfFile;
								String cmd = solver + " -p " + problem.toString() + " -f " + file + " -fo " + supportedFormat.get(solver).toString() + " -a " + arg;
								Triple<String, String, Long> result = Probo2_RunBenchmark.invokeExecutableAndGetTime(cmd,config);
								if(result == null)
									log.logInstanceRun(solver, file, arg, cmd, "", "", true, 0);
								else
									log.logInstanceRun(solver, file, arg, cmd, result.getFirst(), result.getSecond(), false, result.getThird());
							}
						}else if (problem.subProblem().equals(Problem.SubProblem.DS)){
							for(String arg: additionalArgumentsForDS.get(tgfFile)){
								String file = supportedFormat.get(solver).equals(FileFormat.APX) ? apxFile : tgfFile;
								String cmd = solver + " -p " + problem.toString() + " -f " + file + " -fo " + supportedFormat.get(solver).toString() + " -a " + arg;
								Triple<String, String, Long> result = Probo2_RunBenchmark.invokeExecutableAndGetTime(cmd,config);
								if(result == null)
									log.logInstanceRun(solver, file, arg, cmd, "", "", true, 0);
								else
									log.logInstanceRun(solver, file, arg, cmd, result.getFirst(), result.getSecond(), false, result.getThird());
							}
						}else{
							String file = supportedFormat.get(solver).equals(FileFormat.APX) ? apxFile : tgfFile;
							String cmd = solver + " -p " + problem.toString() + " -f " + file + " -fo " + supportedFormat.get(solver).toString();
							Triple<String, String, Long> result = Probo2_RunBenchmark.invokeExecutableAndGetTime(cmd,config);
							if(result == null)
								log.logInstanceRun(solver, file, "", cmd, "", "", true, 0);
							else
								log.logInstanceRun(solver, file, "", cmd, result.getFirst(), result.getSecond(), false, result.getThird());
						}
					}
				}
				log.logEndProblem(problem);
			}
			log.logEndRepetition(it);
		}
		System.out.println("all finished.");
	}
}
