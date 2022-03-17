package net.sf.jAFBenchGen.jAFBenchGen;

import net.sf.jAFBenchGen.jAFBenchGen.AF.Attack;

public class AFFormatter
{
	public static final String APX_PREFIX = "a";
	public static final String APX_ARG_DECLARATION = "arg";
	public static final String APX_ATT_DECLARATION = "att";

	public static String Apx(AF af)
	{
		String ret = "";
		for (Integer arg : af.getArguments())
		{
			ret += AFFormatter.APX_ARG_DECLARATION + "("
					+ AFFormatter.APX_PREFIX + arg + ").\n";
		}

		for (Attack att : af.getAttacks())
		{
			ret += AFFormatter.APX_ATT_DECLARATION + "("
					+ AFFormatter.APX_PREFIX + att.getSource() + ","
					+ AFFormatter.APX_PREFIX + att.getTarget() + ").\n";
		}

		return ret;
	}
}
