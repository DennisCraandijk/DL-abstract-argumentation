from __future__ import annotations

import copy
import pickle
from pathlib import Path
from typing import Set, TextIO, Union

from networkx.classes.digraph import DiGraph
from networkx.classes.function import (
    set_edge_attributes,
    set_node_attributes,
    non_edges,
)

from src.data.utils import apx2nxgraph, nxgraph2apx


class ArgumentationFramework:
    """
    Argumentation Framework class to manipulate the AF and to view properties and representations
    """

    graph: DiGraph

    @classmethod
    def from_pkl(cls, pkl_path: Path):
        """
        Initialize object from a pickle containing the state dict
        """
        with open(pkl_path, "rb") as file:
            state_dict = pickle.load(file)
        return cls(**state_dict)

    @classmethod
    def from_apx(cls, apx: Union[str, list, TextIO], graph_id=None):
        """
        Initalize object from apx file
        """
        if isinstance(apx, str):
            apx = apx.split("\n")
        graph = apx2nxgraph(apx)

        return cls(graph, graph_id)

    def __init__(self, graph: DiGraph, graph_id=None, extensions=None):
        self.extensions = extensions if extensions is not None else {}
        self.graph = graph
        self.graph_id = graph_id

    def to_apx(self):
        """ Make an apx from the AF """
        return nxgraph2apx(self.graph)

    def edge_difference(self, other_af: ArgumentationFramework) -> Set[tuple]:
        """ Set of edge differences between two AFs"""
        edges1 = set(self.graph.edges)
        edges2 = set(other_af.graph.edges)
        return edges1.symmetric_difference(edges2)

    def edge_hamming_distance(self, other_af: ArgumentationFramework):
        """ Calculate the distance to another af in terms of number of different edges"""
        return len(self.edge_difference(other_af))

    def extensions_containing_args(
            self, semantic, arg_set: Union[set, frozenset]
    ) -> Set[frozenset]:
        """ Returns extensions containing specified arguments"""
        extensions = {
            extension
            for extension in self.extensions[semantic]
            if arg_set.issubset(extension)
        }
        return extensions

    def cred_accepted_args(self, semantics, filter_args: frozenset = None) -> frozenset:
        """
        Return credulous accepted arguments according to semantics.
        Optionally use filter_args to restrict acceptence to extenions containing those args
        """
        credulous = frozenset()
        extensions = (
            self.extensions[semantics]
            if filter_args is None
            else self.extensions_containing_args(semantics, filter_args)
        )
        if len(extensions) > 0:
            credulous = frozenset.union(*extensions)
        return credulous

    def scept_accepted_args(
            self, semantics, filter_args: frozenset = None
    ) -> frozenset:
        """
        Return sceptically accepted arguments according to semantics.
        Optionally use filter_args to restrict acceptence to extenions containing those args
        Note: if an  AF has no extensions all arguments are sceptically accepted
        """
        sceptical = frozenset(self.arguments)
        extensions = (
            self.extensions[semantics]
            if filter_args is None
            else self.extensions_containing_args(semantics, filter_args)
        )
        if len(extensions) > 0:
            sceptical = frozenset.intersection(*extensions)
        return sceptical

    def attacked_by(self, arg_set: frozenset):
        """
        Returns the set of arguments attacked by the arg_set
        """
        attacked_args = set()
        for arg in arg_set:
            for attacked_arg in self.graph.successors(arg):
                attacked_args.add(attacked_arg)

        return attacked_args

    def flip_attack(self, edge: tuple) -> ArgumentationFramework:
        """
        Flipping an attack relation entails changing an incoming edge to an outgoing edge
        """
        graph = copy.deepcopy(self.graph)
        if graph.has_edge(*edge):
            graph.remove_edge(*edge)
        else:
            graph.add_edge(*edge)
        return ArgumentationFramework(graph)

    def graph_representation(self, name: str) -> DiGraph:
        name = name.lower()
        assert hasattr(self, f"{name}_representation")
        return getattr(self, f"{name}_representation")

    @property
    def base_representation(self) -> DiGraph:
        graph = copy.deepcopy(self.graph)
        set_node_attributes(graph, 0, "node_input")
        return graph

    @property
    def agnn_representation(self) -> DiGraph:
        """
        Basic graph representation with an empty node input and target variable
        """
        graph = self.base_representation
        set_node_attributes(graph, 0, "node_y")

        # add opposing edge for each existing edge
        set_edge_attributes(graph, 1, 'edge_exists')
        for u, v in graph.edges:
            if not graph.has_edge(v, u) and v != u:
                graph.add_edge(v, u, edge_exists=0)

        # add feature (exist or none exist) of the opposite edge
        set_edge_attributes(graph, None, "edge_opposite")
        for u, v in graph.edges:
            graph.edges[u, v]["edge_opposite"] = graph.edges[v, u]["edge_exists"]

        # self attacks
        set_edge_attributes(graph, 0, "edge_self")
        for n in graph.nodes:
            if graph.has_edge(n, n):
                graph.edges[n, n]["edge_self"] = 1

        return graph

    @property
    def gcn_representation(self) -> DiGraph:
        """
        Graph representation suitable for a graph convolutional network
        """
        graph = self.agnn_representation
        set_node_attributes(graph, 0, "node_y")
        set_node_attributes(graph, float(1), "node_x")
        return graph

    @property
    def fm2_representation(self) -> DiGraph:
        """
        Graph representation Used by Kuhlmann & Thimm (2019)
        each node gets a feature denoting the incoming and outgoing attacks
        """
        graph = self.agnn_representation
        set_node_attributes(graph, 0, "node_y")
        for node in graph.nodes:
            graph.nodes[node]["node_x_in"] = float(graph.in_degree(node))
            graph.nodes[node]["node_x_out"] = float(graph.out_degree(node))
        return graph

    @property
    def enforcement_representation(self) -> DiGraph:
        """
        Fully connected graph representation used for enforcement, where
        each edge contains a feature donoting whether it represents an existing attack,
        whether it represents a self attack and whether an opposing attack exists.
        """
        graph = self.base_representation

        # add input if edge exists
        set_edge_attributes(graph, 1, "edge_exists")
        for u, v in non_edges(graph):
            graph.add_edge(u, v, edge_exists=0)

        # self attacks
        set_edge_attributes(graph, 0, "edge_self")
        for n in graph.nodes:
            if not graph.has_edge(n, n):
                graph.add_edge(n, n, edge_exists=0)
            graph.edges[n, n]["edge_self"] = 1

        # add feature (exist or none exist) of the opposite edge
        set_edge_attributes(graph, None, "edge_opposite")
        for u, v in graph.edges:
            graph.edges[u, v]["edge_opposite"] = graph.edges[v, u]["edge_exists"]

        set_edge_attributes(graph, 0, "edge_y")
        set_edge_attributes(graph, 0, "edge_flipped")

        return graph

    @property
    def state_dict(self) -> dict:
        return self.__dict__.copy()

    @property
    def num_arguments(self) -> int:
        return len(self.graph.nodes)

    @property
    def num_attacks(self) -> int:
        return len(self.graph.edges)

    @property
    def arguments(self) -> set:
        return set(n for n in range(self.num_arguments))
