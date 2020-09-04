import copy
import pickle
import subprocess
import tempfile
from pathlib import Path
from typing import Set

from networkx.classes.digraph import DiGraph
from networkx.classes.function import set_edge_attributes, set_node_attributes, non_edges

from src.data.generate_graphs import get_isomorphic_signature
from src.data.scripts.utils import apx2nxgraph, nxgraph2apx, parse_solver_output


class ArgumentationFramework:
    """Argumentation Framework class to compute extensions, determine argument acceptance
    and get graph representations """

    graph: DiGraph

    @classmethod
    def from_pkl(cls, pkl_path: Path):
        state_dict = pickle.load(open(pkl_path, 'rb'))

        # legacy _id
        if '_id' in state_dict:
            state_dict['id'] = state_dict.pop('_id')

        return cls(**state_dict)

    @classmethod
    def from_apx(cls, apx_file_path: Path, id=None):
        """
        Initalize AF object from apx file
        """
        if not id: id = apx_file_path.stem
        with open(apx_file_path, 'r') as f:
            graph = apx2nxgraph(f.read())

        return cls(id, graph)

    def __init__(self, id, graph, extensions=None, **kwargs):

        self.extensions = extensions if extensions is not None else {}
        self.graph = graph
        self.representations = {}
        self.id = id

    def compute_extensions(self, semantic, solver_path) -> Set[frozenset]:
        apx = nxgraph2apx(self.graph)

        # construct ICCMA solver command
        task = 'SE' if semantic in ['GR', 'ID'] else 'EE'
        cmd = [str(solver_path), '-fo', 'apx', '-p', '{}-{}'.format(task, semantic), '-f']

        # write APX file to RAM
        with tempfile.NamedTemporaryFile(mode='w+') as tmp_input:
            tmp_input.write(apx)
            tmp_input.seek(0)
            cmd.append(tmp_input.name)

            # write output file to RAM and solve
            with tempfile.TemporaryFile(mode='w+') as tmp_output:
                p = subprocess.run(cmd, stdout=tmp_output, stderr=subprocess.PIPE)
                if p.stderr: raise Exception(p.stderr)
                tmp_output.seek(0)

                extensions = parse_solver_output(semantic, tmp_output)

        self.extensions[semantic] = extensions

        return extensions

    def compute_id(self):
        self.id = get_isomorphic_signature(self.graph)
        return self.id

    def get_extensions_containing_s(self, semantic, S: set) -> Set[frozenset]:
        extensions = set([extension for extension in self.extensions[semantic] if
                          S.issubset(extension)])
        return extensions

    def get_cred_accepted_args(self, semantic, S: frozenset = None) -> frozenset:
        credulous = frozenset()
        extensions = self.extensions[semantic] if S is None else self.get_extensions_containing_s(semantic, S)
        if len(extensions) > 0: credulous = frozenset.union(*extensions)
        return credulous

    def get_scept_accepted_args(self, semantic, S: frozenset = None) -> frozenset:
        sceptical = frozenset()
        extensions = self.extensions[semantic] if S is None else self.get_extensions_containing_s(semantic, S)
        if len(extensions) > 0: sceptical = frozenset.intersection(*extensions)
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
        assert type in ['base', 'AGNN', 'enforcement', 'FM2', 'GCN']
        if type not in self.representations:
            self.representations[type] = getattr(self, f'get_{type}_representation')()
        return self.representations[type]

    def get_base_representation(self) -> DiGraph:
        graph = copy.deepcopy(self.graph)
        set_node_attributes(graph, 0, 'node_attr')
        return graph

    def get_AGNN_representation(self) -> DiGraph:
        graph = self.get_base_representation()
        set_node_attributes(graph, 0, 'node_y')
        return graph

    def get_GCN_representation(self) -> DiGraph:
        graph = self.get_AGNN_representation()
        set_node_attributes(graph, float(1), 'node_x')
        return graph

    def get_FM2_representation(self) -> DiGraph:
        graph = self.get_AGNN_representation()
        for node in graph.nodes:
            graph.nodes[node]['node_x_in'] = float(graph.in_degree(node))
            graph.nodes[node]['node_x_out'] = float(graph.out_degree(node))
        return graph

    def get_enforcement_representation(self) -> DiGraph:
        graph = self.get_base_representation()
        set_edge_attributes(graph, 1, 'edge_attr')

        for u, v in non_edges(graph):
            graph.add_edge(u, v, edge_attr=0)

        # self attacks
        for n in graph.nodes:
            if graph.has_edge(n, n):
                graph.edges[n, n]['edge_attr'] = 3
            else:
                graph.add_edge(n, n, edge_attr=2)

        # # traverse all edge combinations and add if edge does not exist
        # for u, v in combinations(graph.nodes, 2):
        #     if not graph.has_edge(u, v):
        #         graph.add_edge(u, v, edge_attr=0)
        #     if not graph.has_edge(v, u):
        #         graph.add_edge(v, u, edge_attr=0)

        set_edge_attributes(graph, 0, 'edge_y')

        return graph
