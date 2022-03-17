from __future__ import annotations

from typing import TYPE_CHECKING

from src.constants import ENUM
from src.data.classes.problems.argumentation_problem import ArgumentationProblem
from src.data.solvers.enumeration_solver import EnumerationSolver

if TYPE_CHECKING:
    from src.data.classes.argumentation_framework import ArgumentationFramework


class EnumerationProblem(ArgumentationProblem):
    """ An enumeration problem where the goal is to enumerate all extensions"""

    @classmethod
    def from_af(cls, af: ArgumentationFramework, **kwargs):
        return super().from_af(af, task=ENUM)

    def solve(self, solver: EnumerationSolver, semantics):
        extensions, solve_time = solver.solve(self.af, semantics, return_times=True)
        self.solutions.setdefault(solver.name, {}).update({semantics: extensions})
        self.solve_times.setdefault(solver.name, {}).update({semantics: solve_time})
        if solver.exact:
            self.af.extensions[semantics] = extensions
            self.optimal_solver = solver.name
