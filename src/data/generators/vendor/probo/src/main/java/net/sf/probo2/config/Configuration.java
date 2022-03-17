package net.sf.probo2.config;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.apache.commons.io.FileUtils;
import org.apache.commons.io.filefilter.DirectoryFileFilter;
import org.yaml.snakeyaml.Yaml;

import net.sf.probo2.util.FilenameFilter;
import net.sf.tweety.arg.dung.semantics.Problem;

/**
 * @author mthimm
 *
 */
public class Configuration {
	
	// configuration parameters keys
	private static String configurationParameterSolvers = "SOLVERS";
	private static String configurationParameterProblems = "PROBLEMS";
	private static String configurationParameterFilesTgf = "FILES_TGF";
	private static String configurationParameterFilesApx = "FILES_APX";
	private static String configurationParameterNumArgsDC = "DC_NUMBEROFARGUMENTS";
	private static String configurationParameterNumArgsDS = "DS_NUMBEROFARGUMENTS";
	private static String configurationParameterTimeout = "TIMEOUT";
	private static String configurationParameterOutput = "OUTPUT";
	private static String configurationParameterRepetitions = "REPETITIONS";
	private static String configurationParameterKillProcesses = "KILLPROCESSES";
			
	/** Solvers to be tested */
	public List<String> SOLVERS = new ArrayList<String>();	
	/** The output log of the benchmark run */
	public String OUTPUT = "log_" + System.currentTimeMillis() + ".txt";
	/** Argumentation frameworks (TGF) to be tested */
	public List<String> FILES_TGF = new ArrayList<String>();
	/** Argumentation frameworks (APX) to be tested */
	public List<String> FILES_APX = new ArrayList<String>();	
	/**  The problems to be considered */
	public List<Problem> PROBLEMS = new ArrayList<Problem>();	
	/** The number of arguments tested for the problem DC. */
	public int DC_NUMBEROFARGUMENTS = 10;
	/** The number of arguments tested for the problem DS. */
	public int DS_NUMBEROFARGUMENTS = 10;	
	/** The timeout for solver calls. */
	public int TIMEOUT = 600;	
	/** The number of repetitions per solver call. */
	public int REPETITIONS = 1;	
	/** Processes to be killed after solver calls */
	public List<String> KILLPROCESSES = new ArrayList<String>();	
	
	/**
	 * Private constructor 
	 */
	private Configuration(){		
	}
	
	/**
	 * Reads the configuration from file and returns it.
	 * @param pathToConfiguration path to probo2 configuration file
	 * @return a configuration or null (if something went wrong)
	 */
	@SuppressWarnings("unchecked")
	public static Configuration getConfiguration(String pathToConfiguration){
		Configuration config = new Configuration();
		// configuration parameters realms
		String configurationTypeStringArray = "StringArray";
		String configurationTypeString = "String";
		String configurationTypeInteger = "Integer";
				
		// association key - realms
		Map<String,String> configurationParameters  = new HashMap<String,String>();
		configurationParameters.put(configurationParameterSolvers, configurationTypeStringArray);
		configurationParameters.put(configurationParameterProblems, configurationTypeStringArray);
		configurationParameters.put(configurationParameterFilesTgf, configurationTypeStringArray);
		configurationParameters.put(configurationParameterFilesApx, configurationTypeStringArray);
		configurationParameters.put(configurationParameterKillProcesses, configurationTypeStringArray);
		configurationParameters.put(configurationParameterNumArgsDC, configurationTypeInteger);
		configurationParameters.put(configurationParameterNumArgsDS, configurationTypeInteger);
		configurationParameters.put(configurationParameterTimeout, configurationTypeInteger);
		configurationParameters.put(configurationParameterOutput, configurationTypeString);
		configurationParameters.put(configurationParameterRepetitions, configurationTypeInteger);
				
		InputStream input = null;
		try{
			input = new FileInputStream(new File(pathToConfiguration));
		}catch (FileNotFoundException e){
			System.err.println("ERROR: " + pathToConfiguration + " does not exist.");	
			return null;
		}
		Yaml yaml = new Yaml();
		Map<String, Object> data = (Map<String, Object>) yaml.load(input);
				
		for (Map.Entry<String, String> confParameter : configurationParameters.entrySet()){
			if(!data.containsKey(confParameter.getKey()))
				continue;
			
			if (confParameter.getKey().equals(configurationParameterNumArgsDC))
				config.DC_NUMBEROFARGUMENTS = (Integer)((Integer)data.get(confParameter.getKey()));
			if (confParameter.getKey().equals(configurationParameterNumArgsDS))
				config.DS_NUMBEROFARGUMENTS = (Integer)((Integer)data.get(confParameter.getKey()));
			if (confParameter.getKey().equals(configurationParameterTimeout))
				config.TIMEOUT = (Integer)((Integer)data.get(confParameter.getKey()));
			if (confParameter.getKey().equals(configurationParameterRepetitions))
				config.REPETITIONS = (Integer)((Integer)data.get(confParameter.getKey()));
			
			if (confParameter.getKey().equals(configurationParameterOutput))
				config.OUTPUT = (String)((String)data.get(confParameter.getKey()));
			
			if (confParameter.getKey().equals(configurationParameterSolvers)){
				for (String s: (ArrayList<String>)data.get(confParameter.getKey()))
					config.SOLVERS.add(s);				
			}
			
			if (confParameter.getKey().equals(configurationParameterProblems)){				
				for (String s: (ArrayList<String>) data.get(confParameter.getKey()))
					config.PROBLEMS.add(Problem.getProblem(s));				
			}
			
			if (confParameter.getKey().equals(configurationParameterFilesTgf)){
				for(String s: (ArrayList<String>) data.get(confParameter.getKey()))
					for(File f: FileUtils.listFiles(new File(s), new FilenameFilter(".tgf"),  DirectoryFileFilter.DIRECTORY)){
						config.FILES_TGF.add(f.getAbsolutePath());
					}
				Collections.sort(config.FILES_TGF);
			}
			
			if (confParameter.getKey().equals(configurationParameterFilesApx)){
				for(String s: (ArrayList<String>) data.get(confParameter.getKey()))
					for(File f: FileUtils.listFiles(new File(s), new FilenameFilter(".apx"),  DirectoryFileFilter.DIRECTORY)){
						config.FILES_APX.add(f.getAbsolutePath());
					}
				Collections.sort(config.FILES_APX);
			}
			
			if (confParameter.getKey().equals(configurationParameterKillProcesses)){
				for (String s: (ArrayList<String>)data.get(confParameter.getKey()))
					config.KILLPROCESSES.add(s);				
			}
		}			
		// check if files are in order
		if(config.FILES_APX.size() != config.FILES_TGF.size()){
			System.err.println("ERROR: numbers of APX and TGF files are different.");
			return null;
		}		
		return config;
	}
	
	/**
	 * Returns "true" if at least one DC problem is to be solved.
	 * @return "true" if at least one DC problem is to be solved.
	 */
	public boolean hasDCProblem(){
		for(Problem p: this.PROBLEMS)
			if(p.subProblem().equals(Problem.SubProblem.DC))
				return true;		
		return false;
	}
	
	/**
	 * Returns "true" if at least one DS or DC problem is to be solved.
	 * @return "true" if at least one DS or DC problem is to be solved.
	 */
	public boolean hasDSProblem(){
		for(Problem p: this.PROBLEMS)
			if(p.subProblem().equals(Problem.SubProblem.DS))
				return true;		
		return false;
	}
}
