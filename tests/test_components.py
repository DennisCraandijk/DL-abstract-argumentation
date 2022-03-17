import random
import shutil
import subprocess

from src import config
from src.constants import *
from src.data.classes.argumentation_framework import ArgumentationFramework
from src.data.classes.problems.enforcement_problem import EnforcementProblem
from src.data.solvers.acceptance_verifier import AcceptanceVerifier
from src.data.solvers.enforcement_solver import EnforcementSolver
from src.data.solvers.enumeration_solver import EnumerationSolver
from src.experiment.reinforcement.enforcement_env import EnforcementEnv

dataset_name = "pytest"
dataset_dir = config.dataset_dir / dataset_name
afs_dir = dataset_dir / "AFs"
num = 6
verifier = AcceptanceVerifier()


def test_af_generation():
    """ Test if we can generate apx files, convert them to an af object
     and if computed extensions are coherent as defined in the literature """

    shutil.rmtree(dataset_dir, ignore_errors=True)
    subprocess.call(["python", "-m", "src.data.generate_apxs", "--name", dataset_name, "--num", str(num)])
    subprocess.call(["python", "-m", "src.data.apxs_to_afs", "--name", dataset_name])
    af_pkl_paths = list(afs_dir.glob("*.pkl"))
    assert len(af_pkl_paths) == num


def test_extensions():
    for af_pkl_path in afs_dir.glob("*.pkl"):
        af = ArgumentationFramework.from_pkl(af_pkl_path)
        assert (
                len(af.extensions[PRF]) >= 1
                and len(af.extensions[GRD]) == 1
                and len(af.extensions[COM]) >= 1
        )
        for prf in af.extensions[PRF]:
            assert list(af.extensions[GRD])[0].issubset(prf)
            assert prf in af.extensions[COM]
        for stb in af.extensions[STB]:
            assert stb in af.extensions[PRF]
            assert list(af.extensions[GRD])[0] in af.extensions[COM]
            assert verifier.is_stable(af, stb)
        for com in af.extensions[COM]:
            assert verifier.is_complete(af, com)


def test_af_verification():
    for af_pkl_path in afs_dir.glob("*.pkl"):
        af = ArgumentationFramework.from_pkl(af_pkl_path)
        for semantics in [GRD, STB, PRF, COM]:

            # test conflict free verification
            for extension in af.extensions[semantics]:
                if len(extension) == 0: continue
                k = random.randint(1, len(extension))
                extension_subset = frozenset(random.sample(extension, k))
                assert verifier.is_conflict_free(af, extension_subset)

            # test is admissible verification
            argument = random.choice(list(af.arguments))
            if not verifier.is_in_admissible(af, argument):
                assert argument not in af.cred_accepted_args(semantics)
                assert argument not in af.scept_accepted_args(
                    semantics) or (len(af.extensions[semantics]) == 0)

        # test is complete verification
        for complete_extension in af.extensions[COM]:
            assert verifier.is_complete(af, complete_extension)

        for stable_extension in af.extensions[STB]:
            assert verifier.is_stable(af, stable_extension)


def test_enforcement_generation():
    enforcement_dir = dataset_dir / "problems" / "enforcement"
    shutil.rmtree(enforcement_dir, ignore_errors=True)
    enum_solver = EnumerationSolver()
    enf_solver = EnforcementSolver()
    subprocess.call(["python", "-m", "src.data.afs_to_enforcement", "--name", dataset_name, "--time_limit", "1"])
    for task in [STRICT, NONSTRICT, CRED, SCEPT]:
        task_dir = enforcement_dir / task
        problem_paths = list(task_dir.glob("*.pkl"))
        assert len(problem_paths) == num

        for problem_path in problem_paths:
            enf_problem = EnforcementProblem.from_pkl(problem_path)
            assert enf_solver.name in enf_problem.solutions
            assert enf_solver.name in enf_problem.solve_times
            assert enf_solver.name in enf_problem.edge_changes

            for semantics, num_changes in enf_problem.optimal_solution.items():
                if num_changes is None:
                    continue
                assert num_changes > 0
                enforcement_env = EnforcementEnv(semantics, task)
                enforcement_env.reset(enf_problem)

                done = False
                edge_changes = enf_problem.optimal_edge_changes[semantics]
                for i, edge_change in enumerate(edge_changes):
                    action = enforcement_env.actions.index(edge_change)
                    observation, reward, done, info = enforcement_env.step(action)
                    if i < len(edge_changes) - 1:
                        assert not done
                assert done

                af = enforcement_env.state
                af.extensions[semantics] = enum_solver.solve(af, semantics)

                if enf_problem.task == STRICT:
                    assert enf_problem.desired_extension in af.extensions[semantics]
                elif enf_problem.task == NONSTRICT:
                    assert any(enf_problem.desired_extension.issubset(extension) for extension in
                               af.extensions[semantics])
                elif enf_problem.task in [CRED, SCEPT]:
                    accepted = af.cred_accepted_args(semantics) if enf_problem.task == CRED else af.scept_accepted_args(
                        semantics)
                    assert enf_problem.positive.issubset(accepted)
                    assert len(enf_problem.negative.intersection(accepted)) == 0


def test_supervised_experiments():
    cmd = ["python", "-m", "src.experiment.supervised.sl_experiment", "--tag", "pytest", "--val_ds", "pytest",
           "--test_ds", "pytest", "--fast_dev_run", "1", "--batch_size", "2"]
    for task in [ENUM, CRED, SCEPT]:
        assert subprocess.run(cmd + ["--task", task]).returncode == 0

    for model in ['AGNN', 'FM2', 'GCN']:
        assert subprocess.run(cmd + ['--model', model]).returncode == 0


def test_reinforcement_experiments():
    cmd = ["python", "-m", "src.experiment.reinforcement.rl_experiment", "--tag", "pytest", "--val_ds", "pytest",
           "--test_ds", "pytest", "--fast_dev_run", "1", "--batch_size", "2"]
    for task in [STRICT, NONSTRICT, CRED, SCEPT]:
        assert subprocess.run(cmd + ["--task", task]).returncode == 0

    for aggregation in ['multi', 'sum', 'conv']:
        assert subprocess.run(cmd + ['--aggregation', aggregation]).returncode == 0
