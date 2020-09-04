package net.sf.jAFBenchGen.jAFBenchGen;

import org.apache.commons.cli.BasicParser;
import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.CommandLineParser;
import org.apache.commons.cli.HelpFormatter;
import org.apache.commons.cli.Option;
import org.apache.commons.cli.OptionBuilder;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;

public class Generator
{
	public static final String typeBarabasi = "BarabasiAlbert";
	public static final String typeWattsStrogatz = "WattsStrogatz";
	public static final String typeER = "ErdosRenyi";
	
	public static final String version = "0.1alpha1";

	private static void help(String message, Options options, int ret)
	{
		if (message != null)
			System.err.println(message+"\n\n");
		HelpFormatter formatter = new HelpFormatter();
		formatter.printHelp("jAFBenchGen", options);
		System.exit(ret);
	}
	
	private static void mandatory(String option, Options options)
	{
		Generator.help("Please specify option -" + option, options, -1);
	}
	
	private static void optionIgnored(String option)
	{
		System.err.println("Option -" + option + " ignored");
	}

	@SuppressWarnings("static-access")
	public static void main(String[] args) throws Exception
	{
		String type = null;
		Integer num = null;
		Double probCycles = null;
		Double probAttacks = null;
		Integer baseDegree = null;
		Double beta = null;
		Boolean display = false;
		Boolean help = false;
		
		AF af = null;

		String optionProbCycles = "BA_WS_probCycles";
		String optionProbAttacks = "ER_probAttacks";
		String optionBaseDegree = "WS_baseDegree";
		String optionBeta = "WS_beta";
		String optionNumargs = "numargs";
		String optionType = "type";

		Options options = new Options();

		options.addOption(new Option("help", "print this message"));
		options.addOption(new Option("display", "display the generated graph"));
		options.addOption(new Option("version", "display version"));

		options.addOption(OptionBuilder.withArgName("positive integer")
				.hasArg().withDescription("number of arguments")
				.create(optionNumargs));

		options.addOption(OptionBuilder
				.withArgName(
						Generator.typeBarabasi + "|"
								+ Generator.typeWattsStrogatz + "|"
								+ Generator.typeER).hasArg()
				.withDescription("structure type").create(optionType));

		options.addOption(OptionBuilder
				.withArgName("floating point number")
				.hasArg()
				.withDescription(
						"probability that an argument is part of a cycle (used with "
								+ Generator.typeBarabasi + " and "
								+ Generator.typeWattsStrogatz + " only)")
				.create(optionProbCycles));

		options.addOption(OptionBuilder
				.withArgName("floating point number")
				.hasArg()
				.withDescription(
						"probability of having an attack between two arguments (used with "
								+ Generator.typeER + " only)")
				.create(optionProbAttacks));

		options.addOption(OptionBuilder
				.withArgName("positive integer")
				.hasArg()
				.withDescription(
						"base degree for each node (used with "
								+ Generator.typeWattsStrogatz
								+ " only). It must be the case that nunmargs >> baseDegree >> log(baseDegree) >> 1 to guarantee that the graph is connected.")
				.create(optionBaseDegree));

		options.addOption(OptionBuilder
				.withArgName("floating point number")
				.hasArg()
				.withDescription(
						"probability to 'rewire' an edge (used with "
								+ Generator.typeER + " only)")
				.create(optionBeta));

		CommandLineParser parser = new BasicParser();
		try
		{
			// parse the command line arguments
			CommandLine line = parser.parse(options, args);

			if (line.hasOption("version"))
			{
				System.out.println(version);
				System.exit(0);
			}
			
			type = line.getOptionValue(optionType);
			if (line.getOptionValue(optionNumargs) != null)
				num = Integer.valueOf(line.getOptionValue(optionNumargs));

			if (line.getOptionValue(optionProbCycles) != null)
				probCycles = Double.valueOf(line
						.getOptionValue(optionProbCycles));

			if (line.getOptionValue(optionProbAttacks) != null)
				probAttacks = Double.valueOf(line
						.getOptionValue(optionProbAttacks));

			if (line.getOptionValue(optionBaseDegree) != null)
				baseDegree = Integer.valueOf(line
						.getOptionValue(optionBaseDegree));

			if (line.getOptionValue(optionBeta ) != null)
				beta = Double.valueOf(line.getOptionValue(optionBeta ));

			if (line.hasOption("help"))
				help = true;

			if (line.hasOption("display"))
				display = true;

			if (help)
				Generator.help(null, options, 0);

			if (num == null)
				Generator.mandatory(optionNumargs, options);

			if (type == null)
				Generator.mandatory(optionType, options);

			if (type.equals(Generator.typeBarabasi))
			{
				if (probCycles == null)
					Generator.mandatory(optionProbCycles, options);
				
				if (probAttacks != null)
					Generator.optionIgnored(optionProbAttacks);
				
				if (baseDegree != null)
					Generator.optionIgnored(optionBaseDegree);
				
				if (beta != null)
					Generator.optionIgnored(optionBeta);
				
				af = new BarabasiAF(num, probCycles);
			}
			
			if (type.equals(Generator.typeER))
			{
				if (probAttacks == null)
					Generator.mandatory(optionProbAttacks, options);
				
				if (probCycles != null)
					Generator.optionIgnored(optionProbCycles);
				
				if (baseDegree != null)
					Generator.optionIgnored(optionBaseDegree);
				
				if (beta != null)
					Generator.optionIgnored(optionBeta);
				
				af = new ERAF(num, probAttacks);
			}
			
			if (type.equals(Generator.typeWattsStrogatz))
			{
				if (probCycles == null)
					Generator.mandatory(optionProbCycles, options);
				
				if (baseDegree == null)
					Generator.mandatory(optionBaseDegree, options);
				
				if (beta == null)
					Generator.mandatory(optionBeta, options);
				
				if (probAttacks != null)
					Generator.optionIgnored(optionProbAttacks);
				
				af = new WattsStrogatzAF(num, baseDegree, beta, probCycles);
			}

		} catch (ParseException exp)
		{
			// oops, something went wrong
			System.err.println("Parsing failed.  Reason: " + exp.getMessage());
		}
		
		System.out.print(AFFormatter.Apx(af));
		
		if (display)
			af.display();
	}
}
