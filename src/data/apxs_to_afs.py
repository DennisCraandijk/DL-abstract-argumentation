import argparse
import pickle
import random

from tqdm import tqdm

from src import config
from src.constants import PRF, GRD, COM, STB
from src.data.classes.argumentation_framework import ArgumentationFramework
from src.data.classes.problems.enumeration_problem import EnumerationProblem
from src.data.solvers.enumeration_solver import EnumerationSolver


def main(args: argparse.Namespace):
    """ Iterate the directory with apx files and save as argumentation frameworks"""

    # create required directories
    graphs_dir = config.dataset_dir / args.name / "graphs"
    afs_dir = config.dataset_dir / args.name / "AFs"
    enumeration_dir = config.dataset_dir / args.name / "problems" / "enumeration"

    afs_dir.mkdir(parents=True, exist_ok=True)
    enumeration_dir.mkdir(parents=True, exist_ok=True)

    # process APX files and skip if already exists
    # files are iterated randomly to allow parallelization
    apx_paths = list(graphs_dir.glob("*.apx"))
    random.shuffle(apx_paths)
    for apx_path in tqdm(apx_paths, desc="Solve"):
        graph_id = apx_path.stem
        af_pkl_path = afs_dir / f"{graph_id}.pkl"
        enum_pkl_path = enumeration_dir / f"{graph_id}.pkl"

        # only continue to the next instance if we can actually
        # load the argumentation framework from the pickle
        if af_pkl_path.exists() and enum_pkl_path.exists():
            try:
                ArgumentationFramework.from_pkl(af_pkl_path)
                EnumerationProblem.from_pkl(enum_pkl_path)
                continue
            except (Exception,):
                pass

        # otherwise build AF from graph file and compute extensions
        with open(apx_path, "r", encoding="utf-8") as file:
            af = ArgumentationFramework.from_apx(file, graph_id=graph_id)

        enumeration_solver = EnumerationSolver()
        problem = EnumerationProblem.from_af(af)
        for semantics in args.semantics:
            problem.solve(enumeration_solver, semantics)
            assert af == problem.af

        with open(enum_pkl_path, "wb+") as file:
            pickle.dump(problem.state_dict, file)

        with open(af_pkl_path, "wb+") as file:
            pickle.dump(af.state_dict, file)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--semantics", nargs="+", default=[PRF, GRD, STB, COM])
    parser.add_argument("--name", type=str, default="debug", help="Dataset name")
    parser.set_defaults(multiprocessing=True)
    args = parser.parse_args()
    main(args)
