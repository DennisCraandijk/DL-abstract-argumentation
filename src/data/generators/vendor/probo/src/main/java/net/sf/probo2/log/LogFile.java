package net.sf.probo2.log;

import java.io.BufferedWriter;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;

import net.sf.tweety.arg.dung.semantics.Problem;

/**
 * For logging benchmarks.
 * 
 * @author Matthias Thimm
 *
 */
public class LogFile {
	/** The actual file */
	private String file;
		
	/**
	 * Creates a new log file.
	 * @param file some file
	 */
	public LogFile(String file){
		this.file = file;
	}	
		
	private void logText(String text) throws IOException{
		PrintWriter writer = new PrintWriter(new BufferedWriter(new FileWriter(this.file, true)));
		writer.println(text);
		writer.close();
	}
	
	public void logGeneralError(String solver, String errormessage) throws IOException{
		this.logText("X=" + solver);
		this.logText("Y=" + errormessage);
	}
	
	public void logStartRepetition(int it) throws IOException{
		this.logText("#=" + it);
	}
	public void logEndRepetition(int it) throws IOException{
		this.logText("$=" + it);
	}
	
	public void logStartProblem(Problem problem) throws IOException{
		this.logText("P=" + problem.toString());
	}
	
	public void logEndProblem(Problem problem) throws IOException{
		this.logText("F=" + problem.toString());
	}
	
	public void logSupportingSolver(String solver) throws IOException{
		this.logText("S=" + solver);
	}
	
	public void logInstanceRun(String solver, String file, String additionalParameter, String cmd, String output, String error, boolean timeout, long rtime) throws IOException{
		this.logText("I=" + file);
		this.logText("V=" + solver);
		this.logText("A=" + additionalParameter);
		this.logText("C=" + cmd);
		this.logText("O=" + output.trim());
		this.logText("E=" + error);
		this.logText("T=" + (timeout ? "timeout" : "intime"));
		this.logText("R=" + rtime);		
	}
}
