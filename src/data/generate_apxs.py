import argparse
import hashlib
import multiprocessing
import os
import uuid
from multiprocessing.pool import Pool
from pathlib import Path

import networkx as nx
import psutil
import pynauty
from networkx.classes.digraph import DiGraph
from tqdm import tqdm

from src import config
from src.data.generators.apx_generator import ApxGenerator
from src.data.utils import apx2nxgraph


def main(args: argparse.Namespace):
    """
    Generate graphs from various AF generators used in ICCMA and save apx files in graphs_dir
    """

    # define and generate necessary directories
    graphs_dir = config.dataset_dir / args.name / "graphs"
    tmp_dir = config.data_dir / "tmp" / args.name
    graphs_dir.mkdir(parents=True, exist_ok=True)
    tmp_dir.mkdir(parents=True, exist_ok=True)

    # gather graph ids from other datasets to avoid duplicates (in test and train set for instance)
    existing_graph_ids = (
        [] if args.allow_duplicates else get_graph_ids_from_dir(config.dataset_dir)
    )

    # continue generating graph apx files until target dir contains args.num graphs
    graph_generator = ApxGenerator(
        tmp_dir, args.min_args, args.max_args, args.timeout
    )
    n_generated_graphs = len(get_graph_ids_from_dir(graphs_dir))
    pbar = tqdm(total=args.num, initial=n_generated_graphs, desc="Generate")
    while n_generated_graphs < args.num:

        # next we create multiprocessing pool for generation of graph apx files, read results
        # and save graphs which meet the requirements
        processes = min(max(multiprocessing.cpu_count() - 1, 1), args.max_processes)
        pool = Pool(processes=processes, initializer=limit_cpu)
        file_names = [str(i) for i in range(args.num - n_generated_graphs)]
        imap = pool.imap(graph_generator.generate, file_names)

        for tmp_apx_path in imap:
            if tmp_apx_path is None:
                continue

            with open(tmp_apx_path, "r", encoding='utf-8') as file:
                graph = apx2nxgraph(file)

            if not is_correctly_generated(graph, args.min_args, args.max_args):
                continue

            graph_id = (
                generate_random_id()
                if args.allow_duplicates
                else get_isomorphic_signature(graph)
            )

            if not args.allow_duplicates and graph_id in existing_graph_ids:
                continue

            # append and move graph apx file to destination folder
            existing_graph_ids.append(graph_id)
            tmp_apx_path.rename(graphs_dir / f"{graph_id}.apx")

            n_generated_graphs += 1
            pbar.update()

    pbar.close()


def get_graph_ids_from_dir(path: Path) -> list:
    """
    Recusrsively scans a directory for APX graphs and returns their ids
    """
    return [file.stem for file in list(path.glob("**/*.apx"))]


def is_correctly_generated(graph: DiGraph, min_args, max_args) -> bool:
    """
    Check if a graph is correctly generated
    """
    # no empty graphs
    if graph.number_of_nodes() == 0 or graph.number_of_edges() == 0:
        return False

    # no incorrectly generated graphs
    if not min_args <= len(graph.nodes) <= max_args:
        return False

    # no graphs with isolated subgraphs
    if nx.number_connected_components(graph.to_undirected()) > 1:
        return False

    return True


def generate_random_id() -> str:
    return uuid.uuid4().hex


def get_isomorphic_signature(graph: DiGraph) -> str:
    """
    Generate unique isomorphic id with pynauty
    """
    nauty_graph = pynauty.Graph(
        len(graph.nodes), directed=True, adjacency_dict=nx.to_dict_of_lists(graph)
    )
    return hashlib.md5(pynauty.certificate(nauty_graph)).hexdigest()


def limit_cpu():
    "is called at every process start"
    p = psutil.Process(os.getpid())
    # windows p.nice(psutil.BELOW_NORMAL_PRIORITY_CLASS)
    # unix  ps.nice(19)
    # set to lowest priority
    p.nice(19)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--num",
        type=int,
        default=10,
        help="Number of argument frameworks to be generated",
    )
    parser.add_argument(
        "--min_args", type=int, default=5, help="Minimum number of arguments"
    )
    parser.add_argument(
        "--max_args", type=int, default=25, help="Maximum number of arguments"
    )
    parser.add_argument("--name", type=str, default="debug", help="Dataset name")
    parser.add_argument(
        "--allow_duplicates",
        dest="allow_duplicates",
        action="store_true",
        help="Stop detecting duplicates",
    )
    parser.add_argument("--max_processes", type=int, default=1)  # use -pe smp <n_slots> in job
    parser.add_argument("--timeout", type=int, default=60)
    args = parser.parse_args()
    main(args)
