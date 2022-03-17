package net.sf.probo.benchmark;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Random;
import java.util.Set;
import java.util.TreeSet;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

import org.yaml.snakeyaml.Yaml;

import net.sf.tweety.arg.dung.parser.TgfParser;
import net.sf.tweety.arg.dung.DungTheory;
import net.sf.tweety.arg.dung.parser.AbstractDungParser;
import net.sf.tweety.arg.dung.parser.FileFormat;
import net.sf.tweety.arg.dung.prover.AbstractSolver;
import net.sf.tweety.arg.dung.prover.GroundTruthSolver;
import net.sf.tweety.arg.dung.semantics.ArgumentStatus;
import net.sf.tweety.arg.dung.semantics.Extension;
import net.sf.tweety.arg.dung.semantics.Labeling;
import net.sf.tweety.arg.dung.semantics.Problem;
import net.sf.tweety.arg.dung.syntax.Argument;
import net.sf.tweety.arg.dung.writer.DungWriter;
import net.sf.tweety.commons.util.Pair;
import net.sf.tweety.commons.util.RandomSubsetIterator;
import net.sf.tweety.commons.util.SetTools;
import net.sf.tweety.commons.util.SubsetIterator;
import net.sf.tweety.commons.util.Triple;

/**
 * This is the main class for performing benchmarks of argumentation solvers.
 * 
 * @author Matthias Thimm
 */
public abstract class Benchmark {

	/**
	 * Return values
	 */
	private static final int WRONG_NUMBER_ARGUMENTS = -1;
	private static final int MISSING_FILE = -2;
	private static final int CORRUPTED_CONFIGURATION = -3;
	
	
	/** Use the new method for killing processes (untested) */
	private static final boolean NEW_KILL_METHOD = false;
	private static final String PATH_TO_CHILDRENSH = "./children.sh";
	
	/**
	 * Solvers to be tested
	 */
	private static String[] solvers = null; //{"solvers/tweetysolver-v1.0.7.sh"};
	
	/**
	 * The log of the benchmark.
	 */
	private static String log = "log_" + System.currentTimeMillis();
	
	/**
	 * Argumentation frameworks to be tested
	 */
	private static String[] filesTgf = null; //{"examples/ex1.tgf", "examples/ex2.tgf", "examples/ex3.tgf"};
	private static String[] filesApx = null; //{"examples/ex1.apx", "examples/ex2.apx", "examples/ex3.apx"};
	
	/**
	 *  The problems to be considered
	 */
	private static Set<Problem> allProblems;
	
	/** The number of arguments tested for the problem DC. */
	private static int DC_NUMBEROFARGUMENTS = -1; //10;
	/** The number of arguments tested for the problem DS. */
	private static int DS_NUMBEROFARGUMENTS = -1; //10;
	/** The number of extensions tested for the problem DE. */
	private static int DE_NUMBEROFEXTENSIONS = -1; //10;
	/** The number of labelings tested for the problem DL. */
	private static int DL_NUMBEROFLABELINGS = -1; //10;
	
	/** One supported format for each solver. */
	private static Map<String,FileFormat> supportedFormat;
	
	/** Timeout for problems (in seconds) */
	private static long timeout = 60*30; // 30 minutes
	
	/**
	 * TweetySolver is the reference solver that provides
	 * correct answers to queries.
	 */
	private static AbstractSolver referenceSolver;
	
	/**
	 * For handling timeouts.
	 */
	private static class SolverCallee implements Callable<Pair<String,String>>{
		private String commandline;
		public SolverCallee(String commandline){
			this.commandline = commandline;			
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
				cleanUp();
			}			
		}		
	}
	
	/**
	 * For handling timeouts.
	 */
	private static class SolverCalleeWithTime implements Callable<Triple<String,String,Long>>{
		private String commandline;
		public SolverCalleeWithTime(String commandline){
			this.commandline = commandline;			
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
				cleanUp();
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
	public static void cleanUp() throws IOException, InterruptedException{
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
				child2 = Runtime.getRuntime().exec("kill " + other);
				child2.waitFor();		
				child2.destroy();
				//System.out.println("Killed PID " + other);
			}			
		}
		reader.close();
		child.destroy();		
		// clean up other processes that seem not to be killed sometimes		
		String[] proc = {"clingo_linux_64", "clingo", "conarg", "python", "swipl", "maxsat", "clasp_linux_64"};
		for(String p: proc){
			String[] cmd2 = { "/bin/sh", "-c", "ps -e | grep " + p };
			child = Runtime.getRuntime().exec(cmd2);
			child.waitFor();	
			reader = new BufferedReader(new InputStreamReader(child.getInputStream()));
			line = "";			
			while((line = reader.readLine())!= null) {
				line = line.trim();
				long other = new Long(line.substring(0, line.indexOf(" ")));
				child2 = Runtime.getRuntime().exec("kill " + other);
				child2.waitFor();		
				child2.destroy();
				
			}		
			reader.close();
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
	private static Pair<String,String> invokeExecutable(String commandline) throws IOException, InterruptedException{
		ExecutorService executor = Executors.newSingleThreadExecutor();
		Future<Pair<String,String>> future = null;;
		try{
			// handle timeout				
			future = executor.submit(new SolverCallee(commandline));
			Pair<String,String> result = future.get(Benchmark.timeout, TimeUnit.SECONDS);
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
	private static Triple<String,String,Long> invokeExecutableAndGetTime(String commandline) throws IOException, InterruptedException{
		ExecutorService executor = Executors.newSingleThreadExecutor();
		Future<Triple<String,String,Long>> future = null;
		try{
			// handle timeout				
			future = executor.submit(new SolverCalleeWithTime(commandline));
			Triple<String,String,Long> result = future.get(Benchmark.timeout, TimeUnit.SECONDS);
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
	 * Kill a process on in the shell
	 * @param name
	 * @throws InterruptedException
	 * @throws IOException
	 */
	private static void killProcess(String name) throws InterruptedException, IOException{
		if(!Benchmark.NEW_KILL_METHOD){
			Process child = Runtime.getRuntime().exec("pkill " + name);
			child.waitFor();	
			child.destroy();
		}else{
			long myPid = getPID();
			Process child = null;
			Process child2 = null;
			Process child3 = null;
			BufferedReader reader = null;
			name = name.substring(0,name.length()-2);

			// search for parents
			String[] cmd = { "/bin/sh", "-c", "ps -e | grep " + name.substring(0,name.length()-2) }; // for some odd reason bash-scripts in ps are listed as "script.s" instead of "script.sh"
			child = Runtime.getRuntime().exec(cmd);
			child.waitFor();
			reader = new BufferedReader(new InputStreamReader(child.getInputStream()));
			String line = "";
			TreeSet<Long> parentPids = new TreeSet<>(Collections.reverseOrder());
			while((line = reader.readLine())!= null) {
				line = line.trim();
				long other = new Long(line.substring(0, line.indexOf(" ")));
				parentPids.add(other);
			}
			reader.close();
			child.destroy();

			// search for children and grandchildren and so on
			TreeSet<Long> childPids = new TreeSet<>(Collections.reverseOrder());
			childPids.addAll(parentPids);
			for(long pid : parentPids)
			{
				child2 = Runtime.getRuntime().exec(Benchmark.PATH_TO_CHILDRENSH + " " + pid);
				child2.waitFor();
				reader = new BufferedReader(new InputStreamReader(child2.getInputStream()));
				while((line = reader.readLine())!= null) {
					//System.out.println(line.trim());
					childPids.add(new Long(line.trim()));
				}
				reader.close();
				child2.destroy();
			}

			// kill everything
			for(long pid : childPids)
			{
				if (myPid != pid) {
					child3 = Runtime.getRuntime().exec("pkill -P " /* + "-e "*/ + pid);
					/*reader = new BufferedReader(new InputStreamReader(child3.getInputStream()));
					while((line = reader.readLine())!= null) {
						System.out.println(line.trim());
					}
					reader.close();*/
					child3.waitFor();
					child3.destroy();
				}
			}
		}
	}
		
	/**
	 * Returns a map that maps all solvers to their corresponding supported problems.
	 * @return a map that maps all solvers to their corresponding supported problems.
	 * @throws IOException 
	 * @throws InterruptedException 
	 */
	private static Map<String,Collection<Problem>> supportedProblems() throws IOException, InterruptedException{
		Map<String,Collection<Problem>> map = new HashMap<String,Collection<Problem>>(); 
		for(String solver: solvers){
			try{
			Pair<String,String> result = Benchmark.invokeExecutable(solver + " --problems");
			map.put(solver, Problem.getProblems(result.getFirst()));
			if(result.getSecond() != "")
				Benchmark.logError(result.getSecond(), solver);
			}catch(IllegalArgumentException e){
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
	private static Map<String,FileFormat> supportedFormat() throws IOException, InterruptedException{
		Map<String,FileFormat> map = new HashMap<String,FileFormat>();
		for(String solver: solvers){
			Pair<String,String> result = Benchmark.invokeExecutable(solver + " --formats");
			Collection<FileFormat> formats = FileFormat.getFileFormats(result.getFirst());
			//Ignore CNF
			formats.remove(FileFormat.CNF);			
			map.put(solver, formats.iterator().next());
			if(result.getSecond() != "")
				Benchmark.logError(result.getSecond(), solver);
		}
		return map;
	}
	
	/**
	 * Returns the file of the given index and format.
	 * @param fo some file format.
	 * @param idx some index.
	 * @return a file
	 */
	private static String getFileOfFormat(FileFormat fo, int idx){
		if(fo.equals(FileFormat.APX))
			return filesApx[idx];
		if(fo.equals(FileFormat.TGF))
			return filesTgf[idx];
		throw new RuntimeException("Unknown file format");
	}
		
	/**
	 * Logs the given message to standard out and to
	 * the log file.
	 * @param str some string
	 * @throws IOException 
	 */
	private static void log(String str) throws IOException{
		PrintWriter writer = new PrintWriter(new BufferedWriter(new FileWriter(Benchmark.log, true)));
		writer.println(str);
		writer.close();
		System.out.println(str);
	}
	
	/**
	 * Logs the given message to the error log
	 * @param str some string
	 * @param solver the solver which produced the error
	 * @throws IOException 
	 */
	private static void logError(String str, String solver) throws IOException{
		PrintWriter writer = new PrintWriter(new BufferedWriter(new FileWriter(solver + ".errlog", true)));
		writer.println(str);
		writer.close();
		//System.out.println(str);
	}
		
	/**
	 * Load configuration file
	 * @param file
	 * @return  -2 if the configuration file is missing
	 * 			-3 if the configuration file is corrupted
	 */
	@SuppressWarnings({ "unchecked" })
	private static int loadConfiguration(String file)
	{
		// configuration parameters keys
		String configurationParameterSolvers = "solvers";
		String configurationParameterProblems = "problems";
		String configurationParameterFilesTgf = "filesTgf";
		String configurationParameterFilesApx = "filesApx";
		String configurationParameterNumArgsDC = "DC_NUMBEROFARGUMENTS";
		String configurationParameterNumArgsDS = "DS_NUMBEROFARGUMENTS";
		String configurationParameterNumExtsDE = "DE_NUMBEROFEXTENSIONS";
		String configurationParameterNumLabsDL = "DL_NUMBEROFLABELINGS";
		String configurationParameterTimeout = "TIMEOUT";
		
		// configuration parameters realms
		String configurationTypeStringArray = "StringArray";
		String configurationTypeInteger = "Integer";
		
		// association key - realms
		Map<String,String> configurationParameters  = new HashMap<String,String>();
		configurationParameters.put(configurationParameterSolvers, configurationTypeStringArray);
		configurationParameters.put(configurationParameterProblems, configurationTypeStringArray);
		configurationParameters.put(configurationParameterFilesTgf, configurationTypeStringArray);
		configurationParameters.put(configurationParameterFilesApx, configurationTypeStringArray);
		configurationParameters.put(configurationParameterNumArgsDC, configurationTypeInteger);
		configurationParameters.put(configurationParameterNumArgsDS, configurationTypeInteger);
		configurationParameters.put(configurationParameterNumExtsDE, configurationTypeInteger);
		configurationParameters.put(configurationParameterNumLabsDL, configurationTypeInteger);
		configurationParameters.put(configurationParameterTimeout, configurationTypeInteger);
		
		InputStream input = null;
		try{
			input = new FileInputStream(new File(file));
		}catch (FileNotFoundException e){
			return(MISSING_FILE);
		}
		Yaml yaml = new Yaml();
		
		Map<String, Object> data = (Map<String, Object>) yaml.load(input);
		
		for (Map.Entry<String, String> confParameter : configurationParameters.entrySet()){
			if (!data.containsKey(confParameter.getKey())) {
				return CORRUPTED_CONFIGURATION;
			}
			
			if (confParameter.getValue().equals(configurationTypeStringArray)){
				ArrayList<String> fromConf = (ArrayList<String>)data.get(confParameter.getKey());
				String toLoad [] = new String[fromConf.size()];
				for (int i = 0; i < toLoad.length; i++){
					toLoad[i] = fromConf.get(i);
				}
				
				if (confParameter.getKey().equals(configurationParameterSolvers))
					solvers = toLoad;
				if (confParameter.getKey().equals(configurationParameterProblems)){
					allProblems = new HashSet<Problem>();
					for(String s: toLoad)
						allProblems.add(Problem.getProblem(s));
				}						
				if (confParameter.getKey().equals(configurationParameterFilesTgf))
					filesTgf = toLoad;
				if (confParameter.getKey().equals(configurationParameterFilesApx))
					filesApx = toLoad;				
			}
			
			if (confParameter.getValue().equals(configurationTypeInteger)){
				int toLoad = (Integer)((Integer)data.get(confParameter.getKey()));
				
				if (confParameter.getKey().equals(configurationParameterNumArgsDC))
					DC_NUMBEROFARGUMENTS = toLoad;
				if (confParameter.getKey().equals(configurationParameterNumArgsDS))
					DS_NUMBEROFARGUMENTS = toLoad;
				if (confParameter.getKey().equals(configurationParameterNumExtsDE))
					DE_NUMBEROFEXTENSIONS = toLoad;
				if (confParameter.getKey().equals(configurationParameterNumLabsDL))
					DL_NUMBEROFLABELINGS = toLoad;
				if (confParameter.getKey().equals(configurationParameterTimeout))
					timeout = toLoad;
			}
		}
		
		return 0;
	}
	
	/**
	 * Main benchmark method
	 * @param the configuration file
	 * @throws IOException 
	 * @throws InterruptedException
	 * @return  -1 if there is a wrong number of arguments passed
	 */
	public static void main(String[] args) throws IOException, InterruptedException{
		//args = new String[2];
		//args[0] = "sampleConfiguration.yaml";
		//args[1] = "examples/solutions";
		
		// load configuration file
		if (args.length != 2){
			System.err.println("Usage:");
			System.err.println("probo <configuration file> <path to solutions>");
			System.exit(WRONG_NUMBER_ARGUMENTS);
		}
		
		int resLoadConfiguration = Benchmark.loadConfiguration(args[0]);
		if (resLoadConfiguration != 0){
			
			if (resLoadConfiguration == MISSING_FILE){
				System.err.println(args[0] + " does not exists");
			}
			
			if (resLoadConfiguration == CORRUPTED_CONFIGURATION){
				System.err.println("Configuration file corrupted");
			}
			System.exit(resLoadConfiguration);
		}
		
		Benchmark.referenceSolver = new GroundTruthSolver(args[1]);
		// get from all solvers their supported problems
		Map<String,Collection<Problem>> supportedProblems = Benchmark.supportedProblems();
		// get from all solvers one of their supported formats
		Benchmark.supportedFormat = Benchmark.supportedFormat();		
		for(Problem problem: allProblems){
			Benchmark.log("=========================================================================================================================================================");
			Benchmark.log("Problem: " + problem.toString());
			Benchmark.log("Solvers supporting the problem:");
			Collection<String> supportingSolvers = new HashSet<String>();
			for(String solver: solvers){
				if(supportedProblems.get(solver).contains(problem)){
					supportingSolvers.add(solver);
					Benchmark.log("\t- " + solver);
				}
			}			
			AbstractDungParser tgfParser = new TgfParser();
			Triple<String,String,Long> result;
			Benchmark.log("");
			Benchmark.log("Benchmark run:");
			long time;
			String cmd;
			for(int i = 0; i < filesTgf.length; i++){				
				// for problems without further parameters
				// just compare output with output of reference solver
				DungTheory aaf = tgfParser.parse(new FileReader(new File(filesTgf[i])));			
				if(problem.subProblem().equals(Problem.SubProblem.EC)){
					Extension solution = AbstractDungParser.parseArgumentList(referenceSolver.solve(problem, new File(filesApx[i]), FileFormat.APX, ""));												
					for(String solver: supportingSolvers){
						cmd = solver + " -p " + problem.toString() + " -f " + Benchmark.getFileOfFormat(supportedFormat.get(solver), i) + " -fo " + supportedFormat.get(solver).toString();
						result = Benchmark.invokeExecutableAndGetTime(cmd);
						Benchmark.killProcess(solver.substring(solver.indexOf("/")+1));
						if(result == null){
							Benchmark.log("I;" + cmd + ";" +  "incorrect" + ";" + "-1");
							continue;
						}
						time = result.getThird();
						Collection<Argument> s = AbstractDungParser.parseArgumentList(result.getFirst());
						Benchmark.log("I;" + cmd + ";" + (s.equals(solution)? "correct": "incorrect") + ";" + time);
						if(result.getSecond() != "")
							Benchmark.logError(result.getSecond(), solver);
					}
				}
				if(problem.subProblem().equals(Problem.SubProblem.EE)){
					Collection<Collection<Argument>> solution = AbstractDungParser.parseExtensionList(referenceSolver.solve(problem, new File(filesApx[i]), FileFormat.APX, ""));
					for(String solver: supportingSolvers){
						cmd = solver + " -p " + problem.toString() + " -f " + Benchmark.getFileOfFormat(supportedFormat.get(solver), i) + " -fo " + supportedFormat.get(solver).toString();
						result = Benchmark.invokeExecutableAndGetTime(cmd);				
						Benchmark.killProcess(solver.substring(solver.indexOf("/")+1));
						if(result == null){
							Benchmark.log("I;" + cmd + ";" +  "incorrect" + ";" + "-1");
							continue;
						}
						time = result.getThird();
						try{
							Collection<Collection<Argument>> s = AbstractDungParser.parseExtensionList(result.getFirst());
							Benchmark.log("I;" + cmd + ";" + (s.equals(solution)? "correct": "incorrect") + ";" + time);
						}catch(IllegalArgumentException e){
							Benchmark.log("I;" + cmd + ";" + "incorrect" + ";" + time);
							Benchmark.logError("ERROR (wrong output): " + cmd + " ('" + result.getFirst() + "')", solver);
						}
									
						if(result.getSecond() != "")
							Benchmark.logError(result.getSecond(), solver);
					}
				}
				if(problem.subProblem().equals(Problem.SubProblem.SE)){
					Collection<Collection<Argument>> solution = AbstractDungParser.parseExtensionList(referenceSolver.solve(Problem.getProblem("EE-" + problem.semantics()), new File(filesApx[i]), FileFormat.APX, ""));
					for(String solver: supportingSolvers){
						cmd = solver + " -p " + problem.toString() + " -f " + Benchmark.getFileOfFormat(supportedFormat.get(solver), i) + " -fo " + supportedFormat.get(solver).toString();
						result = Benchmark.invokeExecutableAndGetTime(cmd);				
						Benchmark.killProcess(solver.substring(solver.indexOf("/")+1));
						if(result == null){
							Benchmark.log("I;" + cmd + ";" +  "incorrect" + ";" + "-1");
							continue;
						}
						time = result.getThird();
						// check for non-existence of extensions
						if (solution == null || solution.isEmpty()){
							Benchmark.log("I;" + cmd + ";" +  (result.getFirst().trim().toLowerCase().equals("no") ? "correct": "incorrect") + ";" + time);							
						}else{
							try{
								Collection<Argument> s = AbstractDungParser.parseArgumentList(result.getFirst());
								Benchmark.log("I;" + cmd + ";" + (solution.contains(s)? "correct": "incorrect") + ";" + time);
							}catch(IllegalArgumentException e){
								Benchmark.log("I;" + cmd + ";incorrect;" + time);
							}
						}
						if(result.getSecond() != "")
							Benchmark.logError(result.getSecond(), solver);
					}
				}
				if(problem.subProblem().equals(Problem.SubProblem.EL)){
					//TODO fix this 
					Collection<Labeling> solution = null;//referenceSolver.solveEL(problem.semantics(), aaf);
					for(String solver: supportingSolvers){
						cmd = solver + " -p " + problem.toString() + " -f " + Benchmark.getFileOfFormat(supportedFormat.get(solver), i) + " -fo " + supportedFormat.get(solver).toString();
						result = Benchmark.invokeExecutableAndGetTime(cmd);				
						Benchmark.killProcess(solver.substring(solver.indexOf("/")+1));
						if(result == null){
							Benchmark.log("I;" + cmd + ";" +  "incorrect" + ";" + "-1");
							continue;
						}
						time = result.getThird();
						Collection<Labeling> s = AbstractDungParser.parseLabelingList(result.getFirst());
						Benchmark.log("I;" + cmd + ";" + (s.equals(solution)? "correct": "incorrect") + ";" + time);
						if(result.getSecond() != "")
							Benchmark.logError(result.getSecond(), solver);
					}
				}
				if(problem.subProblem().equals(Problem.SubProblem.ES)){				
					Extension solution = AbstractDungParser.parseArgumentList(referenceSolver.solve(problem, new File(filesApx[i]), FileFormat.APX, ""));
					for(String solver: supportingSolvers){
						cmd = solver + " -p " + problem.toString() + " -f " + Benchmark.getFileOfFormat(supportedFormat.get(solver), i) + " -fo " + supportedFormat.get(solver).toString();
						result = Benchmark.invokeExecutableAndGetTime(cmd);	
						Benchmark.killProcess(solver.substring(solver.indexOf("/")+1));
						if(result == null){
							Benchmark.log("I;" + cmd + ";" +  "incorrect" + ";" + "-1");
							continue;
						}
						time = result.getThird();
						Extension s = AbstractDungParser.parseArgumentList(result.getFirst());
						Benchmark.log("I;" + cmd + ";" + (s.equals(solution)? "correct": "incorrect") + ";" + time);
						if(result.getSecond() != "")
							Benchmark.logError(result.getSecond(), solver);
					}
				}
				if(problem.subProblem().equals(Problem.SubProblem.DX)){
					boolean solution = !referenceSolver.solve(Problem.getProblem("EE-" + problem.semantics()), new File(filesApx[i]), FileFormat.APX, "").isEmpty();
					for(String solver: supportingSolvers){
						cmd = solver + " -p " + problem.toString() + " -f " + Benchmark.getFileOfFormat(supportedFormat.get(solver), i) + " -fo " + supportedFormat.get(solver).toString();
						result = Benchmark.invokeExecutableAndGetTime(cmd);		
						Benchmark.killProcess(solver.substring(solver.indexOf("/")+1));
						if(result == null){
							Benchmark.log("I;" + cmd + ";" +  "incorrect" + ";" + "-1");
							continue;
						}
						time = result.getThird();
						boolean s = AbstractDungParser.parseBoolean(result.getFirst());
						Benchmark.log("I;" + cmd + ";" + (s == solution? "correct": "incorrect") + ";" + time);
						if(result.getSecond() != "")
							Benchmark.logError(result.getSecond(), solver);
					}
				}
				if(problem.subProblem().equals(Problem.SubProblem.DN)){
					//TODO fix this
					boolean solution = false;//referenceSolver.solveDN(problem.semantics(), aaf);
					for(String solver: supportingSolvers){
						cmd = solver + " -p " + problem.toString() + " -f " + Benchmark.getFileOfFormat(supportedFormat.get(solver), i) + " -fo " + supportedFormat.get(solver).toString();
						result = Benchmark.invokeExecutableAndGetTime(cmd);					
						Benchmark.killProcess(solver.substring(solver.indexOf("/")+1));
						if(result == null){
							Benchmark.log("I;" + cmd + ";" +  "incorrect" + ";" + "-1");
							continue;
						}
						time = result.getThird();
						boolean s = AbstractDungParser.parseBoolean(result.getFirst());
						Benchmark.log("I;" + cmd + ";" + (s == solution? "correct": "incorrect") + ";" + time);
						if(result.getSecond() != "")
							Benchmark.logError(result.getSecond(), solver);
					}
				}
				if(problem.subProblem().equals(Problem.SubProblem.DC)){
					// DC is about checking whether a given argument is credulously accepted
					// for that we randomly pick #DC_NUMBEROFARGUMENTS of the arguments of the argumentation framework					
					Collection<Argument> toBeTested = new HashSet<Argument>();
					List<Argument> allArgs = new LinkedList<Argument>(aaf);
					Random rand = new Random();
					if(aaf.size() < Benchmark.DC_NUMBEROFARGUMENTS)
						toBeTested.addAll(allArgs);
					else while(toBeTested.size() < Benchmark.DC_NUMBEROFARGUMENTS)
						toBeTested.add(allArgs.get(rand.nextInt(aaf.size())));
										
					Collection<Argument> solution = AbstractDungParser.parseArgumentList(referenceSolver.solve(Problem.getProblem("EC-" + problem.semantics()), new File(filesApx[i]), FileFormat.APX, ""));
					for(String solver: supportingSolvers){
						for(Argument a: toBeTested){
							cmd = solver + " -p " + problem.toString() + " -f " + Benchmark.getFileOfFormat(supportedFormat.get(solver), i) + " -fo " + supportedFormat.get(solver).toString() + " -a " + a.getName();
							result = Benchmark.invokeExecutableAndGetTime(cmd);		
							Benchmark.killProcess(solver.substring(solver.indexOf("/")+1));
							if(result == null){
								Benchmark.log("I;" + cmd + ";" +  "incorrect" + ";" + "-1");
								continue;
							}
							time = result.getThird();
							boolean s = AbstractDungParser.parseBoolean(result.getFirst());
							Benchmark.log("I;" + cmd + ";" + ((s && solution.contains(a) || (!s && !solution.contains(a)))? "correct": "incorrect") + ";" + time);							
							if(result.getSecond() != "")
								Benchmark.logError(result.getSecond(), solver);
						}
					}
				}
				if(problem.subProblem().equals(Problem.SubProblem.DS)){
					// DS is about checking whether a given argument is skeptically accepted
					// for that we randomly pick #DS_NUMBEROFARGUMENTS of the arguments of the argumentation framework					
					Collection<Argument> toBeTested = new HashSet<Argument>();
					List<Argument> allArgs = new LinkedList<Argument>(aaf);
					Random rand = new Random();
					if(aaf.size() < Benchmark.DC_NUMBEROFARGUMENTS)
						toBeTested.addAll(allArgs);
					else while(toBeTested.size() < Benchmark.DC_NUMBEROFARGUMENTS)
						toBeTested.add(allArgs.get(rand.nextInt(aaf.size())));
					
					Collection<Argument> solution = AbstractDungParser.parseArgumentList(referenceSolver.solve(Problem.getProblem("ES-" + problem.semantics()), new File(filesApx[i]), FileFormat.APX, ""));
					for(String solver: supportingSolvers){
						for(Argument a: toBeTested){
							cmd = solver + " -p " + problem.toString() + " -f " + Benchmark.getFileOfFormat(supportedFormat.get(solver), i) + " -fo " + supportedFormat.get(solver).toString() + " -a " + a.getName();
							result = Benchmark.invokeExecutableAndGetTime(cmd);	
							Benchmark.killProcess(solver.substring(solver.indexOf("/")+1));
							if(result == null){
								Benchmark.log("I;" + cmd + ";" +  "incorrect" + ";" + "-1");
								continue;
							}
							time = result.getThird();
							boolean s = AbstractDungParser.parseBoolean(result.getFirst());
							Benchmark.log("I;" + cmd + ";" + ((s && solution.contains(a) || (!s && !solution.contains(a)))? "correct": "incorrect") + ";" + time);
							if(result.getSecond() != "")
								Benchmark.logError(result.getSecond(), solver);
						}
					}
				}
				if(problem.subProblem().equals(Problem.SubProblem.DE)){
					// DE is about checking whether a given set of arguments is an extension
					// for that we 0.5 #DE_NUMBEROFEXTENSIONS from the reference solver and generate
					// 0.5 #DE_NUMBEROFEXTENSIONS random argument sets 
					Collection<Collection<Argument>> toBeTested = new HashSet<Collection<Argument>>();
					Collection<Collection<Argument>> solution = AbstractDungParser.parseExtensionList(referenceSolver.solve(Problem.getProblem("EE-" + problem.semantics()), new File(filesApx[i]), FileFormat.APX, ""));
					SetTools<Collection<Argument>> setTools = new SetTools<Collection<Argument>>();
					List<Set<Collection<Argument>>> subsets = new ArrayList<Set<Collection<Argument>>>(setTools.subsets(solution, Math.min(solution.size(), Benchmark.DE_NUMBEROFEXTENSIONS/2)));
					toBeTested.addAll(subsets.get(new Random().nextInt(subsets.size())));
					SubsetIterator<Argument> it = new RandomSubsetIterator<Argument>(new HashSet<Argument>(aaf),false);
					while(toBeTested.size() < Benchmark.DE_NUMBEROFEXTENSIONS && toBeTested.size() < Math.pow(2, aaf.size())){
						toBeTested.add(it.next());
					}
					for(String solver: supportingSolvers){
						for(Collection<Argument> arguments: toBeTested){
							Extension e = new Extension(arguments);
							cmd = solver + " -p " + problem.toString() + " -f " + Benchmark.getFileOfFormat(supportedFormat.get(solver), i) + " -fo " + supportedFormat.get(solver).toString() + " -a " + DungWriter.writeArguments(arguments);
							result = Benchmark.invokeExecutableAndGetTime(cmd);
							Benchmark.killProcess(solver.substring(solver.indexOf("/")+1));
							if(result == null){
								Benchmark.log("I;" + cmd + ";" +  "incorrect" + ";" + "-1");
								continue;
							}
							time = result.getThird();
							boolean s = AbstractDungParser.parseBoolean(result.getFirst());
							Benchmark.log("I;" + cmd + ";" + ((s && solution.contains(e) || (!s && !solution.contains(e)))? "correct": "incorrect") + ";" + time);	
							if(result.getSecond() != "")
								Benchmark.logError(result.getSecond(), solver);
						}
					}					
				}
				if(problem.subProblem().equals(Problem.SubProblem.DL)){
					// DL is about checking whether a given labeling is valid labeling
					// for that we 0.5 #DL_NUMBEROFLABELINGS from the reference solver and generate
					// 0.5 #DE_NUMBEROFLABELINGS random labelings 
					Collection<Labeling> toBeTested = new HashSet<Labeling>();
					// TODO: fix this
					Collection<Labeling> solution = null;//referenceSolver.solveEL(problem.semantics(), aaf);
					SetTools<Labeling> setTools = new SetTools<Labeling>();
					List<Set<Labeling>> subsets = new ArrayList<Set<Labeling>>(setTools.subsets(solution, Math.min(solution.size(), Benchmark.DL_NUMBEROFLABELINGS/2)));
					toBeTested.addAll(subsets.get(new Random().nextInt(subsets.size())));
					Random rand = new Random();
					while(toBeTested.size() < Benchmark.DL_NUMBEROFLABELINGS && toBeTested.size() < Math.pow(2, aaf.size())){
						Labeling l = new Labeling();
						for(Argument a: aaf){
							int r = rand.nextInt(3);
							if(r == 0) l.put(a, ArgumentStatus.IN);
							else if (r==1) l.put(a, ArgumentStatus.OUT);
							else l.put(a, ArgumentStatus.UNDECIDED);								
						}
						toBeTested.add(l);
					}
					for(String solver: supportingSolvers){
						for(Labeling lab: toBeTested){
							cmd = solver + " -p " + problem.toString() + " -f " + Benchmark.getFileOfFormat(supportedFormat.get(solver), i) + " -fo " + supportedFormat.get(solver).toString() + " -a " + DungWriter.writeLabeling(lab);
							result = Benchmark.invokeExecutableAndGetTime(cmd);			
							Benchmark.killProcess(solver.substring(solver.indexOf("/")+1));
							if(result == null){
								Benchmark.log("I;" + cmd + ";" +  "incorrect" + ";" + "-1");
								continue;
							}
							time = result.getThird();
							boolean s = AbstractDungParser.parseBoolean(result.getFirst());
							Benchmark.log("I;" + cmd + ";" + ((s && solution.contains(lab) || (!s && !solution.contains(lab)))? "correct": "incorrect") + ";" + time);
							if(result.getSecond() != "")
								Benchmark.logError(result.getSecond(), solver);
						}
					}	
				}				
			}
			Benchmark.log("=========================================================================================================================================================");
		}		
	}
}
