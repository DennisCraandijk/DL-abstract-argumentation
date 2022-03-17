from pathlib import Path

from torch.utils.data import Dataset

from src.constants import STRICT, NONSTRICT
from src.data.classes.argumentation_framework import ArgumentationFramework
from src.data.classes.problems.enforcement_problem import ExtensionEnforcementProblem, EnforcementProblem, \
    StatusEnforcementProblem


class EnforcementProblemDS(Dataset):
    """ Pytorch dataset for enforcement problems """

    def __init__(self, directory: Path, task: str, fixed_goals: bool):
        self.fixed_goals = fixed_goals
        self.task = task
        self.directory = directory / (f"problems/enforcement/{task}" if fixed_goals else "AFs")
        self.items = list(self.directory.glob("*.pkl"))

    def __len__(self) -> int:
        return len(self.items)

    def __getitem__(self, idx: int) -> EnforcementProblem:
        """ Returns the ith enforcement problem from the dataset.
        When fixed goals is set, the problem is loaded from a pickle with a preset enforcement goal
        Otherwise the AF is loaded and a new enforcement goal is randomly created"""
        problem_class = (
            ExtensionEnforcementProblem
            if self.task in [STRICT, NONSTRICT]
            else StatusEnforcementProblem
        )
        if self.fixed_goals:
            problem = problem_class.from_pkl(self.items[idx])
        else:
            af = ArgumentationFramework.from_pkl(self.items[idx])
            problem = problem_class.from_af(af, self.task)
        return problem
