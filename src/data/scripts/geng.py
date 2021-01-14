import argparse
import pickle
import subprocess
import tempfile
from pathlib import Path

import networkx as nx
from tqdm import tqdm

from src.data.classes.ArgumentationFramework import ArgumentationFramework
from src.data.generate_graphs import get_isomorphic_signature
from src.data.graphs2AFs import validate_extensions
from src.data.scripts.utils import nxgraph2apx

parser = argparse.ArgumentParser()
parser.add_argument('--num_args', type=int, default=3, help='Number of arguments')
parser.add_argument('--name', type=str, default='enf-3', help='Dataset name')
parser.add_argument('--semantics', nargs='+', default=['PR', 'GR', 'ST', 'CO'], help='Semantics')
args = parser.parse_args()

# define directories
path = Path(__file__).parent.parent
data_dir = path.parent.parent / 'data'
AFs_dir = data_dir / f'dataset/{args.name}/AFs'
AFs_dir.mkdir(parents=True, exist_ok=True)
graphs_dir = data_dir / f'dataset/{args.name}/graphs'
graphs_dir.mkdir(parents=True, exist_ok=True)

solver = path / 'solvers/mu-toksia/mu-toksia_static'


graphs = []

is_edge_line = False
edge_list = []


#TODO loop -l

# write output file to RAM and solve
with tempfile.TemporaryFile(mode='w+') as tmp_output:
    cmd = f'nauty-geng {args.num_args} -c -g -l 1 | nauty-directg | nauty-listg -e'
    p = subprocess.run(cmd, stdout=tmp_output, shell=True)
    # p.communicate()
    tmp_output.seek(0)
    output = tmp_output.read()

    for i, line in tqdm(enumerate(output.split('\n\n'))):

        if not line or line[0] == '>': continue

        else:
            edge_line = line.strip().splitlines()[2].split('  ')
            graph: nx.DiGraph = nx.parse_edgelist(edge_line, nodetype=int, create_using=nx.DiGraph)
            apx = nxgraph2apx(graph)
            try:
                graph_id = get_isomorphic_signature(graph)
            except:
                continue
            AF = ArgumentationFramework.from_apx(apx, id=graph_id)
            assert AF.num_arguments == args.num_args
            AF_pkl_path = AFs_dir / f'{graph_id}.pkl'
            if AF_pkl_path.exists():
                continue

            for semantic in args.semantics:
                AF.compute_extensions(semantic, solver)
            validate_extensions(AF)
            pickle.dump(AF.state_dict, open(AF_pkl_path, 'wb+'))
            apx = nxgraph2apx(AF.graph)
            with open((graphs_dir / f'{graph_id}.apx'), 'w+') as f:
                f.write(apx)
