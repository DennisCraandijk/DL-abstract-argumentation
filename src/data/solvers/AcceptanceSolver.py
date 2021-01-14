import ast
import subprocess
import tempfile
from pathlib import Path


class AcceptanceSolver:
    def __init__(self):
        self.mu_toksia = Path(__file__).parent / 'mu-toksia/mu-toksia_static'

    def solve(self, AF, semantics):
        task = "SE" if semantics in ["GR", "ID"] else "EE"
        cmd = [
            str(self.mu_toksia),
            "-fo",
            "apx",
            "-p",
            "{}-{}".format(task, semantics),
            "-f",
        ]
        apx = AF.to_apx()
        # write APX file to RAM
        with tempfile.NamedTemporaryFile(mode="w+") as tmp_input:
            tmp_input.write(apx)
            tmp_input.seek(0)
            cmd.append(tmp_input.name)

            # write output file to RAM and solve
            with tempfile.TemporaryFile(mode="w+") as tmp_output:
                p = subprocess.run(cmd, stdout=tmp_output, stderr=subprocess.PIPE)
                if p.stderr:
                    raise Exception(p.stderr)
                tmp_output.seek(0)

                extensions = self.parse(tmp_output, semantics)

        return extensions
        # AF.extensions[semantics] = extensions
        # return AF

    def parse(self, output, semantics):
        extensions = []
        for i, line in enumerate(output):
            if semantics == 'GR':
                extension = ast.literal_eval(''.join(line.split()))
                extensions = [extension]
                break

            elif i == 0 and line == '[]\n':
                extensions = []
                break

            elif line in ['[\n', ']\n']:
                continue

            extension = ast.literal_eval(''.join(line.split()))
            extensions.append(extension)
        extensions = set(frozenset(extension) for extension in extensions)
        return extensions