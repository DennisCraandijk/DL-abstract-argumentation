import re
from typing import TextIO, Union

import networkx as nx
from networkx.classes.digraph import DiGraph


def nxgraph2apx(graph: DiGraph) -> str:
    apx = ''
    for argument in graph.nodes:
        apx += f'arg({argument}).\n'

    for attack in graph.edges:
        apx += f'att({attack[0]},{attack[1]}).\n'

    return apx


def apx2nxgraph(apx: Union[list, TextIO]) -> DiGraph:
    conversion, i = {}, 0

    graph = nx.DiGraph()
    for line in apx:
        if line[:3] == 'arg':
            argument = re.findall(r'(?<=\().+?(?=\))', line)[0]  # everything between brackets
            if argument not in conversion:
                conversion[argument] = i
                i += 1
            graph.add_node(conversion[argument])

        elif line[:3] == 'att':
            attack = re.findall(r'(?<=\().+?(?=\))', line)[0].split(',')  # everything between brackets
            graph.add_edge(conversion[attack[0]], conversion[attack[1]])

    return graph
