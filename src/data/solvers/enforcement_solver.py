from __future__ import annotations

import re
import resource
import subprocess
import tempfile
from pathlib import Path
from typing import Union

from src.constants import PRF, COM, STB, STRICT, NONSTRICT, CRED, SCEPT, GRD
from src.data.classes.argumentation_framework import ArgumentationFramework
from src.data.classes.problems.enforcement_problem import (
    StatusEnforcementProblem,
    ExtensionEnforcementProblem,
)


class EnforcementSolver:
    def __init__(self):
        self.name = "pakota"
        self.maadoita = (
                Path(__file__).parent / "vendor/maadoita_static/sources/maadoita"
        )
        self.pakota = Path(__file__).parent / "vendor/pakota2/pakota.py"
        self.semantics_conversion = {PRF: "prf", COM: "com", STB: "stb"}
        self.task_conversion = {
            self.pakota: {
                STRICT: "strict",
                NONSTRICT: "nonstrict",
                CRED: "cred",
                SCEPT: "skept",
            },
            self.maadoita: {
                STRICT: "strict",
                NONSTRICT: "non-strict",
            },
        }
        self.exact = True

    def solve(
            self,
            problem: Union[ExtensionEnforcementProblem, StatusEnforcementProblem],
            semantics,
            time_limit=None,
    ):
        if (isinstance(problem, StatusEnforcementProblem) and semantics == GRD) or (
                problem.task == SCEPT and semantics != STB
        ):
            return None, 0, None

        solver = self.maadoita if semantics == GRD else self.pakota

        with tempfile.NamedTemporaryFile(mode="w+") as tmp_input:
            tmp_input.write(problem.to_apx())
            tmp_input.seek(0)

            cmd = [str(solver), tmp_input.name, self.task_conversion[solver][problem.task]]
            if semantics != GRD:
                cmd.append(self.semantics_conversion[semantics])

            # write output file to RAM and solve
            with tempfile.TemporaryFile(mode="w+") as tmp_output:
                try:
                    usage_start = resource.getrusage(resource.RUSAGE_CHILDREN)
                    process = subprocess.run(
                        cmd,
                        stdout=tmp_output,
                        stderr=subprocess.PIPE,
                        timeout=time_limit,
                        check=False,
                    )
                    usage_end = resource.getrusage(resource.RUSAGE_CHILDREN)
                    solve_time = usage_end.ru_utime - usage_start.ru_utime
                except subprocess.TimeoutExpired:
                    problem.solutions.setdefault(self.name, {}).update(
                        {semantics: None}
                    )
                    problem.solve_times.setdefault(self.name, {}).update(
                        {semantics: time_limit}
                    )
                    problem.edge_changes.setdefault(self.name, {}).update(
                        {semantics: None}
                    )

                    return None, 0, None

                if process.stderr:
                    raise Exception(process.stderr)
                tmp_output.seek(0)
                output = tmp_output.read()
                tmp_output.seek(0)
                lines = tmp_output.readlines()

        if solver == self.maadoita:
            changes_readout_line = 3
            apx_readout_lines = slice(changes_readout_line + 1, None)
            assert "Number of changes:" in lines[changes_readout_line]
        else:
            changes_readout_line = 2 if "Number of iterations" in lines[0] else 1
            apx_readout_lines = slice(changes_readout_line + 1, None)
            assert (
                    "o " == lines[changes_readout_line][0:2]
            ), "readout problem for non GR"

        num_changes = int(re.search(r"\d+", lines[changes_readout_line]).group())

        apx_lines = lines[apx_readout_lines]
        modified_af = ArgumentationFramework.from_apx(apx_lines)
        edge_changes = modified_af.edge_difference(problem.af)

        return num_changes, solve_time, edge_changes
