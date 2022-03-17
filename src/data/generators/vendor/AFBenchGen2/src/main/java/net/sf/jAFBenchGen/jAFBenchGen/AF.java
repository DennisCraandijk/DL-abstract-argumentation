package net.sf.jAFBenchGen.jAFBenchGen;

import java.util.Collection;
import java.util.HashSet;
import java.util.Set;

import org.graphstream.algorithm.TarjanStronglyConnectedComponents;
import org.graphstream.graph.Edge;
import org.graphstream.graph.Graph;
import org.graphstream.graph.Node;
import org.graphstream.graph.implementations.MultiGraph;

public abstract class AF
{
	public static final String TYPE_BARABASI = "TYPE_BARABASI";
	public static final String TYPE_EPPSTEIN = "TYPE_EPPSTEIN";
	public static final String TYPE_ER = "TYPE_ER";
	public static final String TYPE_SW = "TYPE_SW";

	public class Attack
	{
		private Integer source = null;
		private Integer target = null;

		public Attack(Integer s, Integer t)
		{
			this.source = s;
			this.target = t;
		}

		public Integer getSource()
		{
			return this.source;
		}

		public Integer getTarget()
		{
			return this.target;
		}

		public int hashCode()
		{
			int hashFirst = source != null ? source.hashCode() : 0;
			int hashSecond = target != null ? target.hashCode() : 0;

			return (hashFirst + hashSecond) * hashSecond + hashFirst;
		}

		public boolean equals(Object other)
		{
			if (other instanceof Attack)
			{
				Attack otherPair = (Attack) other;
				return ((this.source == otherPair.getSource() || (this.source != null
						&& otherPair.getSource() != null && this.source
							.equals(otherPair.getSource()))) && (this.target == otherPair
						.getTarget() || (this.target != null
						&& otherPair.getTarget() != null && this.target
							.equals(otherPair.getTarget()))));
			}

			return false;
		}

		public String toString()
		{
			return "(" + source + ", " + target + ")";
		}

	}

	// protected class mySimpleGraph<V, E> extends AbstractBaseGraph<V, E>
	// implements DirectedGraph<V, E>
	// {
	//
	// private static final long serialVersionUID = 3545796589454112304L;
	//
	// /**
	// * Creates a new simple graph with the specified edge factory.
	// *
	// * @param ef
	// * the edge factory of the new graph.
	// */
	// public mySimpleGraph(EdgeFactory<V, E> ef)
	// {
	// super(ef, false, true);
	// }
	//
	// /**
	// * Creates a new simple graph.
	// *
	// * @param edgeClass
	// * class on which to base factory for edges
	// */
	// public mySimpleGraph(Class<? extends E> edgeClass)
	// {
	// this(new ClassBasedEdgeFactory<V, E>(edgeClass));
	// }
	// }
	//
	// protected class simpleVertexFactory implements VertexFactory<Integer>
	// {
	//
	// int counter = -1;
	//
	// public simpleVertexFactory()
	// {
	// this.counter = 0;
	// }
	//
	// public Integer createVertex()
	// {
	// return new Integer(this.counter++);
	// }
	//
	// }

	protected Graph g = null;

	protected TarjanStronglyConnectedComponents cc = null;

	public AF()
	{
		this.g = new MultiGraph("");
	}

	public Collection<Integer> getArguments()
	{
		Collection<Integer> ret = new HashSet<Integer>();
		for (Integer i = 0; i < this.g.getNodeCount(); i++)
			ret.add(i);
		return ret;
	}

	public Set<Attack> getAttacks()
	{
		HashSet<Attack> ret = new HashSet<Attack>();
		for (Edge e : this.g.getEdgeSet())
		{
			ret.add(new Attack(e.getNode0().getIndex(), e.getNode1().getIndex()));
		}
		return ret;
	}

	public int numSCCs()
	{
		this.cc = new TarjanStronglyConnectedComponents();
		cc.init(this.g);

		cc.compute();

		int max = 0;

		for (Node n : this.g.getNodeSet())
		{
			if ((Integer) n.getAttribute(cc.getSCCIndexAttribute()) > max)
				max = n.getAttribute(cc.getSCCIndexAttribute());
		}
		return max;
	}

	public void display()
	{
		for (Node n : this.g.getNodeSet())
			n.addAttribute("ui.label", n.getIndex());
		this.g.display();
	}
	
	protected boolean edgeBetweenNodes(Node source, Node target)
	{
		for (Edge e : this.g.getEdgeSet())
		{
			if (e.getNode0().equals(source) && e.getNode1().equals(target))
				return true;
		}
		return false;
	}

}
