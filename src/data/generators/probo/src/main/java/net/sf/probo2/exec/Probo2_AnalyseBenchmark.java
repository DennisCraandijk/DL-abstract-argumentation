package net.sf.probo2.exec;

import java.io.BufferedReader;
import java.io.DataInputStream;
import java.io.FileInputStream;
import java.io.InputStreamReader;
import java.util.HashMap;
import java.util.Map;

import net.sf.probo2.config.Configuration;
import net.sf.tweety.arg.dung.semantics.Problem;

/**
 * Analyses log files created by Probo2_RunBenchmark and prints statistics.
 * 
 * @author Matthias Thimm
 *
 */
public class Probo2_AnalyseBenchmark {
	
	// returns the value of the line
	private static String getLineValue(String str){
		return str.substring(str.indexOf("=")+1).trim();
	}
	
	/**
	 * Main analysis method
	 * @param args program parameters 
	 */
	public static void main(String[] args){
		//args = new String[2];
		//args[0] = "sampleConfiguration2.yaml";
		//args[1] = "log_1522758755098.txt";
		
		// load configuration file
		if (args.length != 2){
			System.err.println("Usage:");
			System.err.println("probo2-analyse-benchmark <configuration file> <log file>");
			System.exit(1);
		}
		System.out.println("probo2-analyse-benchmark started.");
		Configuration config = Configuration.getConfiguration(args[0]);
		if(config == null){					
			System.exit(1);
			System.err.println("Error: configuration not found");
		}
		System.out.println("configuration loaded.");
						
		try{
			FileInputStream fstream = new FileInputStream(args[1]);
			DataInputStream in = new DataInputStream(fstream);
			BufferedReader br = new BufferedReader(new InputStreamReader(in));
			String line;
			
			// data structures for analysis			
			// inverted index for tfg files
			Map<String,Integer> tgfFiles2Idx = new HashMap<String,Integer>();
			for(int i = 0; i < config.FILES_TGF.size();i++)
				tgfFiles2Idx.put(config.FILES_TGF.get(i), i);
			// inverted index for apx files
			Map<String,Integer> apxFiles2Idx = new HashMap<String,Integer>();
			for(int i = 0; i < config.FILES_APX.size();i++)
				apxFiles2Idx.put(config.FILES_APX.get(i), i);
			// inverted index for problems
			Map<Problem,Integer> problems2Idx = new HashMap<Problem,Integer>();
			for(int i = 0; i < config.PROBLEMS.size();i++)
				problems2Idx.put(config.PROBLEMS.get(i), i);
			// inverted index for solvers
			Map<String,Integer> solver2Idx = new HashMap<String,Integer>();
			for(int i = 0; i < config.SOLVERS.size();i++)
				solver2Idx.put(config.SOLVERS.get(i), i);
			System.out.println("inverted indices prepared.");			
			// records timeouts
			// timeouts[NO_REPETITION][NO_PROBLEM][NO_SOLVER][NO_BENCHMARK][NO_PARAMETER]
			boolean timeouts[][][][][] = new boolean[config.REPETITIONS][config.PROBLEMS.size()][][][];
			// records runtimes
			// runtimes[NO_REPETITION][NO_PROBLEM][NO_SOLVER][NO_BENCHMARK][NO_PARAMETER]
			long runtimes[][][][][] = new long[config.REPETITIONS][config.PROBLEMS.size()][][][];
			System.out.println("data structures prepared.");	
			int repetition = 0, problem = 0, solver = 0, benchmark = 0, parameter = -1, supportedSolvers = 0;
			boolean init = false;
			while ((line = br.readLine()) != null) {
				if(line.startsWith("#"))
					repetition = new Integer(getLineValue(line));
				else if(line.startsWith("P")){
					problem = problems2Idx.get(Problem.getProblem(getLineValue(line)));
					supportedSolvers = 0;
					init = false;
				}else if(line.startsWith("S")){
					supportedSolvers++;
				}else if(line.startsWith("I")){
					if(init == false){
						if(config.PROBLEMS.get(problem).subProblem().equals(Problem.SubProblem.DC)){
							timeouts[repetition-1][problem] = new boolean[supportedSolvers][config.FILES_TGF.size()][config.DC_NUMBEROFARGUMENTS];
							runtimes[repetition-1][problem] = new long[supportedSolvers][config.FILES_TGF.size()][config.DC_NUMBEROFARGUMENTS];
						}else if(config.PROBLEMS.get(problem).subProblem().equals(Problem.SubProblem.DS)){
							timeouts[repetition-1][problem] = new boolean[supportedSolvers][config.FILES_TGF.size()][config.DS_NUMBEROFARGUMENTS];
							runtimes[repetition-1][problem] = new long[supportedSolvers][config.FILES_TGF.size()][config.DS_NUMBEROFARGUMENTS];
						} else{
							timeouts[repetition-1][problem] = new boolean[supportedSolvers][config.FILES_TGF.size()][1];
							runtimes[repetition-1][problem] = new long[supportedSolvers][config.FILES_TGF.size()][1];
						}
						init = true;
					}
					benchmark = tgfFiles2Idx.containsKey(getLineValue(line)) ? tgfFiles2Idx.get(getLineValue(line)) : apxFiles2Idx.get(getLineValue(line));
					
				}else if(line.startsWith("V")){
					solver = solver2Idx.get(getLineValue(line));
				}else if(line.startsWith("A")){
					parameter++;
				}else if(line.startsWith("C")){
					//nothing to do?
				}else if(line.startsWith("O")){
					//TODO verify output?
				}else if(line.startsWith("E")){
					//nothing to do?
				}else if(line.startsWith("T")){
					timeouts[repetition-1][problem][solver][benchmark][parameter] = getLineValue(line).equals("timeout");
					System.out.println("Repetition #" + repetition + ", " + config.PROBLEMS.get(problem) + ", " + config.SOLVERS.get(solver) + ", " + "file #" + benchmark + ", param #"+ parameter + ", " + 
							((timeouts[repetition-1][problem][solver][benchmark][parameter])?"TIMEOUT":"NO TIMEOUT"));
				}else if(line.startsWith("R")){
					runtimes[repetition-1][problem][solver][benchmark][parameter] = new Long(getLineValue(line));
					System.out.println("Repetition #" + repetition + ", " + config.PROBLEMS.get(problem) + ", " + config.SOLVERS.get(solver) + ", " + "file #" + benchmark + ", param #"+ parameter + ", " + 
							runtimes[repetition-1][problem][solver][benchmark][parameter] + "ms");
					if(!(config.PROBLEMS.get(problem).subProblem().equals(Problem.SubProblem.DC) && parameter + 1 < config.DC_NUMBEROFARGUMENTS
							|| config.PROBLEMS.get(problem).subProblem().equals(Problem.SubProblem.DS) && parameter + 1 < config.DS_NUMBEROFARGUMENTS))						
					parameter = -1;					
				}else if(line.startsWith("F")){
					//nothing to do?
				}else if(line.startsWith("$")){
					//nothing to do?
				}				
			}
			in.close();
			System.out.println("log has been read.");
			// print some tables
			for(Problem p: config.PROBLEMS) {
				// for each solver the number of solved instances
				Map<String,Integer> numInstances = new HashMap<String,Integer>();
				// for each solver the total runtime on solved instances
				Map<String,Long> rtInstances = new HashMap<String,Long>();				
				for(int s = 0; s < timeouts[0][problems2Idx.get(p)].length; s++) {
					int num = 0;
					long rt = 0;
					for(int i = 0; i < config.FILES_TGF.size(); i++) {
						for(int j = 0; j < timeouts[0][problems2Idx.get(p)][s][i].length; j++) {
							int numTimeouts = 0;
							long rtfile = 0;
							for (int r = 0; r < config.REPETITIONS; r++) {
								if(timeouts[r][problems2Idx.get(p)][s][i][j])
									numTimeouts += 1;
								else rtfile += runtimes[r][problems2Idx.get(p)][s][i][j];
							}
							if(numTimeouts < config.REPETITIONS) {
								num++;
								rt += (rtfile/(config.REPETITIONS - numTimeouts));
							}
						}
					}
					numInstances.put(config.SOLVERS.get(s), num);
					rtInstances.put(config.SOLVERS.get(s), rt);
				}
				// print table
				System.out.println();
				System.out.println("Problem " + p);
				for(String s: numInstances.keySet()) {
					System.out.println(s + "\t" + numInstances.get(s) + "\t" + rtInstances.get(s) + "ms");
				}
			}
		}catch (Exception e){
			System.err.println("General error while processing log file " + args[0]);
			e.printStackTrace();
		}	
		System.out.println();
		System.out.println("all finished.");
	}
}
