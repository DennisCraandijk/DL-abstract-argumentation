from __future__ import annotations

import math
import pickle
import random
from pathlib import Path
from typing import Optional, Dict, List, TYPE_CHECKING

from src.constants import STRICT, NONSTRICT, SCEPT, CRED
from src.data.classes.argumentation_framework import ArgumentationFramework
from src.data.classes.problems.argumentation_problem import ArgumentationProblem

if TYPE_CHECKING:
    from src.data.solvers.enforcement_solver import EnforcementSolver


class EnforcementProblem(ArgumentationProblem):
    """
    Base class for extension and status enforcement problems
    """

    def __init__(
            self,
            af: ArgumentationFramework,
            task: str,
            edge_changes: Optional[Dict[str, Dict[str, List[tuple]]]] = None,
            **kwargs,
    ):
        super().__init__(af, task, **kwargs)
        self.edge_changes = {} if edge_changes is None else edge_changes

    @classmethod
    def from_af(cls, af, task):
        if task in [STRICT, NONSTRICT]:
            return ExtensionEnforcementProblem.from_af(af, task)
        if task in [CRED, SCEPT]:
            return StatusEnforcementProblem.from_af(af, task)
        raise Exception

    @classmethod
    def from_pkl(cls, pkl_path: Path):
        with open(pkl_path, "rb") as file:
            state_dict = pickle.load(file)

        if "desired_extension" in state_dict:
            return ExtensionEnforcementProblem.from_state_dict(state_dict)
        if "positive" in state_dict:
            return StatusEnforcementProblem.from_state_dict(state_dict)

        raise Exception

    @property
    def optimal_edge_changes(self) -> dict:
        if not self.optimal_solver or self.optimal_solver not in self.edge_changes:
            raise ValueError

        return self.edge_changes[self.optimal_solver]

    def solve(self, solver: EnforcementSolver, semantics: str, time_limit=None):
        solution, solve_time, edge_changes = solver.solve(self, semantics, time_limit)
        self.solutions.setdefault(solver.name, {}).update({semantics: solution})
        self.solve_times.setdefault(solver.name, {}).update({semantics: solve_time})
        self.edge_changes.setdefault(solver.name, {}).update({semantics: edge_changes})
        if solver.exact:
            self.optimal_solver = solver.name


class ExtensionEnforcementProblem(EnforcementProblem):
    """
    An extension enforcement problem where
    desired_extension is a set of arguments that should become an extension
    """

    def __init__(self, desired_extension: frozenset, **kwargs):
        super().__init__(**kwargs)
        self.desired_extension = desired_extension

    @classmethod
    def from_af(
            cls, af: ArgumentationFramework, task, max_enforce_fraction=1, seed=None
    ):
        # get arg fraction with min 1 and max |A|-1
        max_args = min(
            max(math.floor(af.num_arguments * max_enforce_fraction), 1),
            af.num_arguments - 1,
        )

        # get a random set of arguments as desired extension which isn't an extension right now
        desired_extension = None
        all_extensions = [
            extension
            for extensions in af.extensions.values()
            for extension in extensions
        ]
        while (
                desired_extension is None
                or (task == STRICT and desired_extension in all_extensions)
                or (
                        task == NONSTRICT
                        and any(
                    desired_extension.issubset(extension)
                    for extension in all_extensions
                )
                )
        ):

            random.seed(seed)
            k = random.randint(1, max_args)
            random.seed(seed)
            desired_extension = frozenset(random.sample(af.arguments, k))
            if seed:
                seed += 1

        return cls(
            af=af,
            task=task,
            solutions=None,
            solve_times=None,
            desired_extension=desired_extension,
        )

    def to_apx(self):
        apx = super().to_apx()
        for argument in self.desired_extension:
            apx += f"enf({argument}).\n"
        return apx

    def __str__(self):
        return f"Enforce desired extension: {self.desired_extension}"


class StatusEnforcementProblem(EnforcementProblem):
    """
    A status enforcement problem where
    positive and negative arguments should be accepted and rejected resp.
    """

    def __init__(self, negative: frozenset, positive: frozenset, **kwargs):
        super().__init__(**kwargs)
        self.negative = negative
        self.positive = positive

    @classmethod
    def from_af(cls, af: ArgumentationFramework, task, max_enforce_fraction=1):
        # get arg fraction with min 1 and max |A|-1
        max_args = min(
            max(math.floor(af.num_arguments * max_enforce_fraction), 1),
            af.num_arguments - 1,
        )

        # accepted_args = set()
        # for semantics in af.extensions.keys():
        #     accepted_args = (
        #         accepted_args.union(af.cred_accepted_args(semantics))
        #         if task == CRED
        #         else af.scept_accepted_args(semantics)
        #     )
        positive: Optional[frozenset] = None
        negative: Optional[frozenset] = None
        accepted_args_per_semanitcs = [
            af.cred_accepted_args(semantics)
            if task == CRED
            else af.scept_accepted_args(semantics)
            for semantics in af.extensions.keys()
        ]
        while positive is None or any(
                positive.issubset(accepted_args) and negative.isdisjoint(accepted_args)
                for accepted_args in accepted_args_per_semanitcs
        ):
            k = random.randint(1, max_args)
            enforce_arguments = frozenset(random.sample(af.arguments, k))
            p = random.randint(0, len(enforce_arguments))
            positive = frozenset(random.sample(enforce_arguments, p))
            negative = enforce_arguments - positive

        return cls(
            af=af,
            task=task,
            solutions=None,
            solve_times=None,
            negative=negative,
            positive=positive,
        )

    def to_apx(self):
        apx = super().to_apx()
        for argument in self.negative:
            apx += f"neg({argument}).\n"
        for argument in self.positive:
            apx += f"pos({argument}).\n"
        return apx

    def __str__(self):
        return f"Enforce desired status. Negative: {self.negative}. Positive: {self.positive}"
