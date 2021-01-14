import copy
import pickle
from pathlib import Path
from typing import Set

from networkx.classes.digraph import DiGraph
from networkx.classes.function import (
    set_edge_attributes,
    set_node_attributes,
    non_edges,
)


from src.data.scripts.utils import apx2nxgraph, nxgraph2apx
from src.data.solvers.AcceptanceSolver import AcceptanceSolver


class ArgumentationFramework:
    """Argumentation Framework class to compute extensions, determine argument acceptance
    and get graph representations"""

    graph: DiGraph

    @classmethod
    def from_pkl(cls, pkl_path: Path):
        state_dict = pickle.load(open(pkl_path, "rb"))

        # legacy _id
        if "_id" in state_dict:
            state_dict["id"] = state_dict.pop("_id")

        return cls(**state_dict)

    @classmethod
    def from_apx(cls, apx: str, id=None):
        """
        Initalize AF object from apx file
        """

        graph = apx2nxgraph(apx)

        return cls(id, graph)

    def __init__(self, id, graph, extensions=None, **kwargs):

        self.extensions = extensions if extensions is not None else {}
        self.graph = graph
        self.representations = {}
        self.id = id

    def to_apx(self):
        return nxgraph2apx(self.graph)

    def edge_hamming_distance(self, AF: "ArgumentationFramework"):
        edges1 = set(self.graph.edges)
        edges2 = set(AF.graph.edges)
        return len(edges1.symmetric_difference(edges2))

    def get_extensions_containing_s(self, semantic, S: set) -> Set[frozenset]:
        extensions = set(
            [
                extension
                for extension in self.extensions[semantic]
                if S.issubset(extension)
            ]
        )
        return extensions

    def get_cred_accepted_args(self, semantic, S: frozenset = None) -> frozenset:
        credulous = frozenset()
        extensions = (
            self.extensions[semantic]
            if S is None
            else self.get_extensions_containing_s(semantic, S)
        )
        if len(extensions) > 0:
            credulous = frozenset.union(*extensions)
        return credulous

    def get_scept_accepted_args(self, semantic, S: frozenset = None) -> frozenset:
        sceptical = frozenset()
        extensions = (
            self.extensions[semantic]
            if S is None
            else self.get_extensions_containing_s(semantic, S)
        )
        if len(extensions) > 0:
            sceptical = frozenset.intersection(*extensions)
        return sceptical

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

    def get_representation(self, type) -> DiGraph:
        assert type in ["base", "AGNN", "enforcement", "FM2", "GCN"]
        if type not in self.representations:
            self.representations[type] = getattr(self, f"get_{type}_representation")()
        return self.representations[type]

    def get_base_representation(self) -> DiGraph:
        graph = copy.deepcopy(self.graph)
        set_node_attributes(graph, 0, "node_input")
        return graph

    def get_AGNN_representation(self) -> DiGraph:
        graph = self.get_base_representation()
        set_node_attributes(graph, 0, "node_y")
        return graph

    def get_GCN_representation(self) -> DiGraph:
        graph = self.get_AGNN_representation()
        set_node_attributes(graph, float(1), "node_x")
        return graph

    def get_FM2_representation(self) -> DiGraph:
        graph = self.get_AGNN_representation()
        for node in graph.nodes:
            graph.nodes[node]["node_x_in"] = float(graph.in_degree(node))
            graph.nodes[node]["node_x_out"] = float(graph.out_degree(node))
        return graph

    def get_enforcement_representation(self) -> DiGraph:
        graph = self.get_base_representation()
        set_edge_attributes(graph, 1, "edge_input")

        for u, v in non_edges(graph):
            graph.add_edge(u, v, edge_input=0)

        # self attacks
        for n in graph.nodes:
            if graph.has_edge(n, n):
                graph.edges[n, n]["edge_input"] = 3
            else:
                graph.add_edge(n, n, edge_input=2)

        set_edge_attributes(graph, 0, "edge_y")

        return graph

    def verify(self, S: frozenset, semantics, solver=None):
        if semantics == "ST":
            return self.verify_stable(S)
        elif semantics == "CO":
            return self.verify_complete(S)
        elif semantics in ["GR", "PR"]:
            return self.verify_solver(S, semantics, solver)
        else:
            raise Exception("Semantics not known")

    def verify_stable(self, S: frozenset):
        # "the set of arguments which are not attacked by S and then testing if this set is equal to S"
        not_attacked_by_S = self.arguments - self.attacked_by(S)
        return S == frozenset(not_attacked_by_S)

    def verify_complete(self, S: frozenset):
        # "Compute the set of arguments defended by S, the set of arguments not attacked by S and then to test if their intersection is equal to S."
        attacked_by_S = self.attacked_by(S)
        defended_by_S = set()
        for arg in self.arguments:
            attackers = set(self.graph.predecessors(arg))
            if attackers.issubset(attacked_by_S):
                defended_by_S.add(arg)

        not_attacked_by_S = self.arguments - attacked_by_S
        intersection = defended_by_S.intersection(not_attacked_by_S)
        return S == frozenset(intersection)

    def verify_solver(self, S: frozenset, semantics, solver: AcceptanceSolver):
        return S in solver.solve(self, semantics)

    def attacked_by(self, S: frozenset):
        attacked_args = set()
        for arg in S:
            for attacked_arg in self.graph.successors(arg):
                attacked_args.add(attacked_arg)

        return attacked_args
