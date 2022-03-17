package net.sf.jAFBenchGen.jAFBenchGen;

import org.graphstream.algorithm.generator.Generator;
import org.graphstream.algorithm.generator.RandomGenerator;


public class ERAF extends AF
{
	public ERAF(int numargs, double prob)
	{
		super();
		Generator gg = new RandomGenerator(prob * ((double)numargs), true, true);
		
		gg.addSink(this.g);
		
		gg.begin();
		do
		{
			gg.nextEvents();
		}while (this.g.getNodeCount() <= numargs);
		
		gg.end();
	}
}
