import pickle
from pathlib import Path
from typing import Optional, Dict

from src.data.classes.argumentation_framework import ArgumentationFramework


class ArgumentationProblem:
    """
    Base class for computational argumentation prolems
    """

    def __init__(
        self,
        af: ArgumentationFramework,
        task: str,
        solutions: Optional[Dict[str, dict]] = None,
        solve_times: Optional[Dict[str, Dict[str, float]]] = None,
        optimal_solver: Optional[str] = None,
    ):
        self.af = af
        self.task = task
        self.solutions = {} if solutions is None else solutions
        self.solve_times = {} if solve_times is None else solve_times
        self.optimal_solver = optimal_solver

    @classmethod
    def from_af(cls, af: ArgumentationFramework, task: str):
        """ (Randomly) construct object from Argumentation Framework"""
        return cls(af, task)

    @classmethod
    def from_state_dict(cls, state_dict: dict):
        """ Construct object from state dict """
        state_dict["af"] = ArgumentationFramework(**state_dict["af"])
        return cls(**state_dict)

    @classmethod
    def from_pkl(cls, pkl_path: Path):
        """ Construct object from state dict pickle """
        with open(pkl_path, "rb") as file:
            state_dict = pickle.load(file)
        return ArgumentationProblem.from_state_dict(state_dict)

    def to_pkl(self, pkl_path: Path):
        """ Save state dict to a pickle """
        with open(pkl_path, "wb+") as file:
            pickle.dump(self.state_dict, file)

    @property
    def state_dict(self):
        state_dict = self.__dict__.copy()
        state_dict["af"] = state_dict["af"].state_dict
        return state_dict

    def to_apx(self):
        """ Convert problem to APX format """
        apx = self.af.to_apx()
        return apx

    @property
    def optimal_solution(self):
        if not self.optimal_solver or self.optimal_solver not in self.solutions:
            raise ValueError

        return self.solutions[self.optimal_solver]

    @property
    def optimal_solve_time(self):
        if not self.optimal_solver or self.optimal_solver not in self.solve_times:
            raise ValueError

        return self.solve_times[self.optimal_solver]

    def solve(self, **kwargs):
        raise NotImplementedError
