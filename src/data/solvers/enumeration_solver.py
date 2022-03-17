from __future__ import annotations

import ast
import subprocess
import tempfile
from collections import Set
from io import TextIOWrapper
from pathlib import Path
from timeit import default_timer as timer
from typing import Tuple, Union

from src.constants import GRD, IDE, PRF, STB, COM
from src.data.classes.argumentation_framework import ArgumentationFramework


class EnumerationSolver:
    """
    A wrapper around the mu-toksia solver for computing extensions
    """

    def __init__(self):
        self.name = "mu-toksia"
        self.mu_toksia = Path(__file__).parent / "vendor/mu-toksia/mu-toksia_static"
        self.semantics_conversion = {GRD: "GR", PRF: "PR", COM: "CO", STB: "ST"}
        self.exact = True

    def solve(
            self, af: ArgumentationFramework, semantics: str, return_times=False
    ) -> Union[Set[frozenset], Tuple[Set[frozenset], float]]:
        """
        Compute extensions with mu-toksia,
        write to a tempfile and parse
        """

        task = "SE" if semantics in [GRD, IDE] else "EE"
        cmd = [
            str(self.mu_toksia),
            "-fo",
            "apx",
            "-p",
            f"{task}-{self.semantics_conversion[semantics]}",
            "-f",
        ]
        apx = af.to_apx()
        # write APX file to RAM
        with tempfile.NamedTemporaryFile(mode="w+") as tmp_input:
            tmp_input.write(apx)
            tmp_input.seek(0)
            cmd.append(tmp_input.name)

            # write output file to RAM and solve
            with tempfile.TemporaryFile(mode="w+") as tmp_output:
                start = timer()
                subprocess.run(
                    cmd, stdout=tmp_output, stderr=subprocess.PIPE, check=True
                )
                end = timer()
                solve_time = end - start

                tmp_output.seek(0)
                extensions = self.parse(tmp_output, semantics)

        if return_times:
            return extensions, solve_time
        return extensions

    @staticmethod
    def parse(output: TextIOWrapper, semantics: str) -> Set[frozenset]:
        """
        Parse the extensions from the output of mu-toksia
        """
        extensions = []
        for i, line in enumerate(output):
            if semantics == GRD:
                extension = ast.literal_eval("".join(line.split()))
                extensions = [extension]
                break

            if i == 0 and line == "[]\n":
                extensions = []
                break

            if line in ["[\n", "]\n"]:
                continue

            extension = ast.literal_eval("".join(line.split()))
            extensions.append(extension)
        extensions = set(frozenset(extension) for extension in extensions)
        return extensions
