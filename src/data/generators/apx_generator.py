import random
import subprocess
from pathlib import Path
from typing import Optional, Tuple


class ApxGenerator:
    """
    A generator that calls random AF generators from the ICCMA competition,
    generates apx files and return the apx paths
    """

    def __init__(self, directory, min_nodes, max_nodes, timeout):
        self.min_nodes = min_nodes
        self.max_nodes = max_nodes
        self.dir = directory
        self.timeout = timeout

        self.path = Path(__file__).parent
        self.jaf_benchgen = self.path / "vendor/AFBenchGen2/target/jAFBenchGen-2.jar"
        self.af_gen_bench = f"{self.path}/vendor/AFGenBenchmarkGenerator/target:"
        self.probo_cp = (
            f"{self.path}/vendor/probo/target/:"
            f"{self.path}/vendor/probo/lib/*"
        )

    def generate(self, file_name: str) -> Optional[Path]:
        """
        Generate an APX file with an AF generator and return its path
        """

        name, cmd = self.generate_random_generator_cmd(file_name)
        cmd = ["timeout", str(self.timeout)] + cmd
        with subprocess.Popen(
                cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=self.path
        ) as process:
            output, stderr = process.communicate()

            if process.returncode != 0:
                print("Error")
                print(" ".join(cmd))
                print(stderr)
                return None

        file_path = self.dir / f"{file_name}.apx"

        # save output to file when using jAFBench
        if name in ["BarabasiAlbert", "WattsStrogatz", "ErdosRenyi"]:
            with open(file_path, "wb") as file:
                file.write(output)

        assert file_path.exists()

        return file_path

    def generate_random_generator_cmd(self, file_name: str) -> Tuple[str, list]:
        """
        Generate a command list for a random ICCMA generator with corresponding parameters
        """
        num_nodes = random.randint(self.min_nodes, self.max_nodes)

        generators = {
            "AFGen": [
                "-cp",
                self.af_gen_bench,
                "AFGen.AFGen",
                str(self.dir / file_name),
                str(num_nodes),
                str(random.randrange(1, 20) / 100),
                str(random.randrange(1, 40) / 100),
            ],
            "BarabasiAlbert": [
                "-jar",
                str(self.jaf_benchgen),
                "-numargs",
                str(num_nodes - 1),
                "-type",
                "BarabasiAlbert",
                "-BA_WS_probCycles",
                str(random.randrange(1, 25) / 100),
            ],
            "WattsStrogatz": [
                "-jar",
                str(self.jaf_benchgen),
                "-numargs",
                str(num_nodes),
                "-type",
                "WattsStrogatz",
                "-BA_WS_probCycles",
                str(random.randrange(1, 25) / 100),
                "-WS_baseDegree",
                str(random.randrange(2, num_nodes - 1, 2)),
                "-WS_beta",
                str(random.randrange(1, 25) / 100),
            ],
            "ErdosRenyi": [
                "-jar",
                str(self.jaf_benchgen),
                "-numargs",
                str(num_nodes - 1),
                "-type",
                "ErdosRenyi",
                "-ER_probAttacks",
                str(random.randrange(25, 50) / 100),
            ],
            "Grounded": [
                "-cp",
                f"{self.probo_cp}",
                "net.sf.probo.generators.GroundedGenerator",
                str(self.dir / file_name),
                str(self.max_nodes),
            ],
            "Scc": [
                "-cp",
                f"{self.probo_cp}",
                "net.sf.probo.generators.SccGenerator",
                str(self.dir / file_name),
                str(self.max_nodes),
            ],
            "Stable": [
                "-cp",
                f"{self.probo_cp}",
                "net.sf.probo.generators.StableGenerator",
                str(self.dir / file_name),
                str(self.max_nodes),
            ],
        }

        name, cmd = random.choice(list(generators.items()))
        cmd = ["java"] + cmd
        return name, cmd
