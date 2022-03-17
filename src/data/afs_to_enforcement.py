import argparse
import random

from tqdm import tqdm

from src import config
from src.constants import GRD, STB, COM, PRF, CRED, STRICT, NONSTRICT, SCEPT
from src.data.classes.argumentation_framework import ArgumentationFramework
from src.data.classes.problems.enforcement_problem import EnforcementProblem
from src.data.solvers.enforcement_solver import EnforcementSolver


def main(args: argparse.Namespace):
    """
    Generate and solve an enforcement problem for each AF in the dataset dir
    """
    dataset_dir = config.data_dir / "dataset" / args.name
    afs_dir = dataset_dir / "AFs"
    enforcement_dir = dataset_dir / "problems" / "enforcement"
    enforcement_dir.mkdir(parents=True, exist_ok=True)

    solver = EnforcementSolver()
    for task in args.tasks:
        task_dir = enforcement_dir / task
        task_dir.mkdir(parents=True, exist_ok=True)

        paths = list(afs_dir.glob("*.pkl"))
        random.shuffle(paths)
        for af_path in tqdm(paths):
            problem_path = task_dir / f"{af_path.stem}.pkl"
            if problem_path.exists():
                try:
                    problem = EnforcementProblem.from_pkl(problem_path)
                except Exception as exception:
                    print(exception)
                    continue
            else:
                af = ArgumentationFramework.from_pkl(af_path)
                problem = EnforcementProblem.from_af(af, task)

            for semantics in args.semantics:

                # check if solution for semantics exists,
                # and if exists but without optimal found than
                # check if now the time limit is highger than last try
                if (
                        solver.name in problem.solutions
                        and semantics in problem.solutions[solver.name]
                        and (
                        problem.solutions[solver.name][semantics] is not None
                        or problem.solve_times[solver.name][semantics]
                        >= args.time_limit
                )
                ):
                    continue

                problem.solve(solver, semantics, time_limit=args.time_limit)

                if task == CRED and semantics == PRF:
                    problem.solutions[solver.name][COM] = problem.solutions[
                        solver.name
                    ][PRF]
                    problem.solve_times[solver.name][COM] = problem.solve_times[
                        solver.name
                    ][PRF]
                    problem.edge_changes[solver.name][COM] = problem.edge_changes[
                        solver.name
                    ][PRF]

            problem.to_pkl(problem_path)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--semantics", nargs="+", default=[PRF, GRD, STB, COM], help="Semantics"
    )
    parser.add_argument(
        "--tasks", nargs="+", default=[STRICT, NONSTRICT, CRED, SCEPT]
    )
    parser.add_argument("--name", type=str, default="debug", help="Dataset name")
    parser.add_argument("--max_enforce_fraction", type=float, default=1)
    parser.add_argument("--time_limit", type=int, default=900)
    args = parser.parse_args()
    main(args)
