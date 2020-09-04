package net.sf.jAFBenchGen.jAFBenchGen;

import org.graphstream.algorithm.Toolkit;
import org.graphstream.algorithm.generator.BaseGenerator;
import org.graphstream.algorithm.generator.WattsStrogatzGenerator;
import org.graphstream.graph.Edge;
import org.graphstream.graph.Node;

public class WattsStrogatzAF extends AF
{
	public WattsStrogatzAF(int numargs, int baseDegree, double beta,
			double probabilityCycle) throws Exception
	{
		super();
		if (numargs <= 0)
			throw new Exception("Number of arguments must be greater than 0");

		if (probabilityCycle < 0 || probabilityCycle > 1)
			throw new Exception(
					"Probability of argument being in a cycle must be between 0 and 1");

		BaseGenerator gen = new WattsStrogatzGenerator(numargs, baseDegree, beta);

		gen.setDirectedEdges(true, true);

		gen.addSink(this.g);
		gen.begin();
		while (gen.nextEvents())
		{
		}
		gen.end();

		while (this.numSCCs() >= numargs * (1.00 - probabilityCycle))
		{
			Edge ex = Toolkit.randomEdge(this.g);
			if (this.edgeBetweenNodes(ex.getNode1(), ex.getNode0()))
				continue;
			this.g.addEdge(new String("" + this.g.getEdgeCount()),
					(Node) ex.getNode1(), (Node) ex.getNode0(), true);
		}
	}
}
