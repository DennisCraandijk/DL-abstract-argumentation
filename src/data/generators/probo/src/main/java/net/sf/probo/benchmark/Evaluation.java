package net.sf.probo.benchmark;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.StringTokenizer;

import net.sf.tweety.arg.dung.semantics.Problem;
import net.sf.tweety.commons.util.Triple;

/**
 * This is the main class for evaluating solvers. It gets as input
 * the output of the Benchmark class and provides a ranking of the
 * solvers with runtimes on each problem.
 * 
 * @author Matthias Thimm
 */
public abstract class Evaluation {

	/** The log file*/
	private static String benchmark_log = "log_1404819796319";

	// Maps each problem to the solvers, their overall runtime, and the number of instances they solved
	private static Map<Problem,Set<Triple<String,Long,Long>>> statistics = new HashMap<Problem,Set<Triple<String,Long,Long>>>();
	
	/**
	 * Main evaluation method.
	 * @param args
	 * @throws IOException 
	 */
	public static void main(String[] args) throws IOException{
		BufferedReader in = new BufferedReader(new FileReader(new File(Evaluation.benchmark_log)));
		String row = null;
		String cmd = null;
		String solver = null;
		Problem problem = null;
		long runtime = 0;
		StringTokenizer tokenizer1, tokenizer2 = null;
		while ((row = in.readLine()) != null) {
			// only rows starting with "I;" are relevant
			row = row.trim();
			if(!row.startsWith("I;"))
				continue;
			//separate line by ";"
			tokenizer1 = new StringTokenizer(row, ";");
			// ignore first token (this is "I");
			tokenizer1.nextToken();
			// next token is the command line which includes solver name and problem, the rest is irrelevant
			cmd = tokenizer1.nextToken();
			tokenizer2 = new StringTokenizer(cmd, " ");
			solver = tokenizer2.nextToken().trim();
			tokenizer2.nextToken();
			// next token is the problem definition
			problem = Problem.getProblem(tokenizer2.nextToken().trim());
			// if result was correct get runtime
			if(tokenizer1.nextToken().trim().equals("correct")){
				runtime = new Long(tokenizer1.nextToken().trim());
				// get correct entry from statistics
				Set<Triple<String,Long,Long>> entries = null;
				if(Evaluation.statistics.containsKey(problem))
					entries = Evaluation.statistics.get(problem);
				else{
					entries = new HashSet<Triple<String,Long,Long>>();
					Evaluation.statistics.put(problem, entries);
				}
				// check whether solver is contained
				Triple<String,Long,Long> e = null;
				for(Triple<String,Long,Long> entry: entries){
					if(entry.getFirst().equals(solver)){
						e = entry;
						break;
					}
				}
				if(e == null){
					e = new Triple<String,Long,Long>();
					e.setFirst(solver);
					e.setSecond(0L);
					e.setThird(0L);
					entries.add(e);
				}
				e.setSecond(e.getSecond() + runtime);
				e.setThird(e.getThird() + 1);
			}			
		}
		in.close();
		// Give some output
		for(Problem prob: Evaluation.statistics.keySet()){
			System.out.println("=========================================================================================================================================================");
			System.out.println(prob);
			System.out.println();
			for(Triple<String,Long,Long> tr: Evaluation.statistics.get(prob)){
				System.out.println(tr.getFirst() + "\t" + tr.getSecond() + "\t" + tr.getThird());
			}
			System.out.println("=========================================================================================================================================================");
		}
	}
}
