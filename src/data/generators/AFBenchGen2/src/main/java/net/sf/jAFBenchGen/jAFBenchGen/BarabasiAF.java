package net.sf.jAFBenchGen.jAFBenchGen;

import org.graphstream.algorithm.Toolkit;
import org.graphstream.algorithm.generator.BarabasiAlbertGenerator;
import org.graphstream.algorithm.generator.BaseGenerator;
import org.graphstream.graph.Edge;
import org.graphstream.graph.Node;


public class BarabasiAF extends AF
{

	public BarabasiAF(int numargs, double probabilityCycle) throws Exception
	{
		super();
		if (numargs <= 0)
			throw new Exception("Number of arguments must be greater than 0");
		
		if (probabilityCycle < 0 || probabilityCycle > 1)
			throw new Exception("Probability of argument being in a cycle must be between 0 and 1");
		
		BaseGenerator gg = new BarabasiAlbertGenerator();
		
		gg.setDirectedEdges(true,true);
		
		gg.addSink(this.g);
		
		gg.begin();
		do
		{
			gg.nextEvents();
		} while (this.g.getNodeCount() <= numargs);
		gg.end();
		
		while (this.numSCCs() >= numargs*(1.00 - probabilityCycle))
		{
			Edge ex = Toolkit.randomEdge(this.g);
			if (this.edgeBetweenNodes(ex.getNode1(), ex.getNode0()))
				continue;
			this.g.addEdge(new String(""+this.g.getEdgeCount()), (Node) ex.getNode1(), (Node) ex.getNode0(), true);
		}
	}


}
