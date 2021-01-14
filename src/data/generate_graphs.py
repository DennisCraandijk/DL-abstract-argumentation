import argparse
import hashlib
import multiprocessing
import uuid
from multiprocessing.pool import Pool
from pathlib import Path

import networkx as nx
import pynauty
from networkx.classes.digraph import DiGraph
from tqdm import tqdm

from src.data.generators.GraphGenerator import GraphGenerator
from src.data.scripts.utils import limit_cpu, apx2nxgraph


# define directories
path = Path(__file__).parent
data_dir = path.parent.parent / 'data'
tmp_dir = data_dir / 'tmp/graphs'


def main(args):
    """
    Generate graphs from various AF generators used in ICCMA and save apx files in graphs_dir
    """
    graphs_dir = data_dir / f'dataset/{args.name}/graphs'
    graphs_dir.mkdir(parents=True, exist_ok=True)
    tmp_dir.mkdir(parents=True, exist_ok=True)
    empty_dir(tmp_dir)

    all_graph_ids = get_graph_ids_from_dir(graphs_dir.parent)
    n_graphs = len(get_graph_ids_from_dir(graphs_dir))

    # continue generating graph apx files until target dir contains args.num graphs
    graph_generator = GraphGenerator(tmp_dir, args.min_args, args.max_args)
    pbar = tqdm(total=args.num, initial=n_graphs, desc='Generate')
    while n_graphs < args.num:

        # next we create multiprocessing pool for generation of graph apx files, read results
        # and save graphs which meet the requirements
        processes = min(max(multiprocessing.cpu_count() - 1, 1), args.max_processes)
        pool = Pool(processes=processes, initializer=limit_cpu)
        tmp_ids = [i for i in range(args.num - n_graphs)]
        imap = pool.imap(graph_generator.generate, tmp_ids)
        for tmp_apx_path in imap:
            if tmp_apx_path is None: continue

            with open(tmp_apx_path, 'r') as f:
                graph = apx2nxgraph(f.read())

            if not satisfies_restrictions(graph): continue

            graph_id = get_isomorphic_signature(graph) if args.deduplication else get_random_id()

            if args.deduplication and graph_id in all_graph_ids: continue

            # append and move graph apx file to destination folder
            all_graph_ids.append(graph_id)
            tmp_apx_path.rename(graphs_dir / f'{graph_id}.apx')

            n_graphs += 1
            pbar.update()
    pbar.close()

    empty_dir(tmp_dir)


def get_graph_ids_from_dir(d: Path) -> list:
    return [file.stem for file in list(d.glob('**/*.apx'))]


def empty_dir(d: Path):
    for file in d.iterdir():
        file.unlink()


def satisfies_restrictions(graph: DiGraph) -> bool:
    # discard incorrectly generated graphs
    if not args.min_args <= len(graph.nodes) <= args.max_args:
        return False

    # discard graphs with isolated subgraphs
    if nx.number_connected_components(graph.to_undirected()) > 1:
        return False

    return True


def get_random_id() -> str:
    return uuid.uuid4().hex


def get_isomorphic_signature(graph: DiGraph) -> str:
    """
    Generate unique isomorphic id with pynauty
    """
    nauty_graph = pynauty.Graph(len(graph.nodes), directed=True, adjacency_dict=nx.to_dict_of_lists(graph))
    return hashlib.md5(pynauty.certificate(nauty_graph)).hexdigest()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--num', type=int, default=10, help='Number of argument frameworks to be generated')
    parser.add_argument('--min_args', type=int, default=5, help='Minimum number of arguments')
    parser.add_argument('--max_args', type=int, default=25, help='Maximum number of arguments')
    parser.add_argument('--name', type=str, default='debug', help='Dataset name')
    parser.add_argument('--no_deduplication', dest='deduplication', action='store_false', help='Stop detecting duplicates')
    parser.add_argument('--max_processes', type=int, default=16)
    parser.set_defaults(deduplication=True)
    args = parser.parse_args()
    main(args)
