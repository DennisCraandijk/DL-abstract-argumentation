import ast
import os
import re

import networkx as nx
import psutil
from networkx.classes.digraph import DiGraph


def limit_cpu():
    "is called at every process start"
    p = psutil.Process(os.getpid())
    # windows p.nice(psutil.BELOW_NORMAL_PRIORITY_CLASS)
    # unix  ps.nice(19)
    # set to lowest priority
    p.nice(19)


def nxgraph2apx(graph: DiGraph) -> str:
    apx = ''
    for argument in graph.nodes:
        apx += f'arg({argument}).\n'

    for attack in graph.edges:
        apx += f'att({attack[0]},{attack[1]}).\n'

    return apx


def apx2nxgraph(apx: str) -> DiGraph:
    conversion, i = {}, 0

    graph = nx.DiGraph()
    for line in apx.splitlines():
        assert line[:3] in ['arg', 'att']
        if line[:3] == 'arg':
            argument = re.findall(r'(?<=\().+?(?=\))', line)[0]  # everything between brackets
            if argument not in conversion:
                conversion[argument] = i
                i += 1
            graph.add_node(conversion[argument])

        else:
            attack = re.findall(r'(?<=\().+?(?=\))', line)[0].split(',')  # everything between brackets
            graph.add_edge(conversion[attack[0]], conversion[attack[1]])

    return graph

# def is_admissible(graph, extension):
#     for u, v in list(graph.edges):
#         # should be conflict free
#         if u in extension and v in extension:
#             return False
#
#         # if attacked argument is IN
#         if v in extension:
#             # for all possible defenders
#             possible_defendeces = graph.in_edges(u)
#             for possible_defender, _ in possible_defendeces:
#                 if possible_defender in extension:
#                     break
#             else:
#                 return False
#     return True
def parse_solver_output(semantic, tmp_output):
    extensions = []
    for i, line in enumerate(tmp_output):
        if semantic == 'GR':
            extension = ast.literal_eval(''.join(line.split()))
            extensions = [extension]
            break

        elif i == 0 and line == '[]\n':
            extensions = []
            break

        elif line in ['[\n', ']\n']:
            continue

        extension = ast.literal_eval(''.join(line.split()))
        extensions.append(extension)
    extensions = set(frozenset(extension) for extension in extensions)
    return extensions