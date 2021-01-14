import random
import subprocess
from pathlib import Path


class GraphGenerator:
    def __init__(self, dir, min_nodes, max_nodes):
        """
        Generator since it does not accept variables in imap
        """
        self.min_nodes = min_nodes
        self.max_nodes = max_nodes
        self.dir = dir

        self.path = Path(__file__).parent.parent
        self.jAFBench = self.path / 'generators/AFBenchGen2/target/jAFBenchGen-2.jar'
        self.AFGen_cp = f'{self.path}/generators/AFGenBenchmarkGenerator/target:'
        self.probo_cp = f'{self.path}/generators/probo/target/:' \
                        f'{self.path}/generators/probo/lib/*'

    def generate(self, id) -> Path:
        timeout = 30 * 60
        name, generator = self.random_graph_generator_cmd(id)
        cmd = ['timeout', str(timeout)] + generator
        p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=self.path)
        output, stderr = p.communicate()

        if p.returncode != 0:
            print(' '.join(cmd))
            print(stderr)
            return None

        file_path = self.dir / '{}.apx'.format(id)
        # save output to file when using jAFBench
        if name in ['BarabasiAlbert', 'WattsStrogatz', 'ErdosRenyi']:
            with open(file_path, 'wb') as f:
                f.write(output)

        assert file_path.exists()

        return file_path

    def random_graph_generator_cmd(self, id):
        num_nodes = random.randint(self.min_nodes, self.max_nodes)

        generators = {
            'AFGen': [
                '-cp',
                self.AFGen_cp,
                'AFGen.AFGen',
                str(self.dir / str(id)),
                str(num_nodes),
                str(random.randrange(1, 20) / 100),
                str(random.randrange(1, 40) / 100)
            ],
            'BarabasiAlbert': [
                '-jar',
                str(self.jAFBench),
                '-numargs', str(num_nodes - 1),
                '-type', 'BarabasiAlbert',
                '-BA_WS_probCycles', str(random.randrange(1, 25) / 100)
            ],
            'WattsStrogatz': [
                '-jar',
                str(self.jAFBench),
                '-numargs', str(num_nodes),
                '-type', 'WattsStrogatz',
                '-BA_WS_probCycles', str(random.randrange(1, 25) / 100),
                '-WS_baseDegree', str(random.randrange(2, num_nodes - 1, 2)),
                '-WS_beta', str(random.randrange(1, 25) / 100)
            ],
            'ErdosRenyi': [
                '-jar',
                str(self.jAFBench),
                '-numargs', str(num_nodes - 1),
                '-type', 'ErdosRenyi',
                '-ER_probAttacks', str(random.randrange(25, 50) / 100),
            ],
            'Grounded': [
                '-cp',
                f'{self.probo_cp}',
                'net.sf.probo.generators.GroundedGenerator',
                str(self.dir / str(id)),
                str(self.max_nodes),
            ],
            'Scc': [
                '-cp',
                f'{self.probo_cp}',
                'net.sf.probo.generators.SccGenerator',
                str(self.dir / str(id)),
                str(self.max_nodes),
            ],
            'Stable': [
                '-cp',
                f'{self.probo_cp}',
                'net.sf.probo.generators.StableGenerator',
                str(self.dir / str(id)),
                str(self.max_nodes),
            ]
        }

        name, generator = random.choice(list(generators.items()))

        return name, ['java'] + generator
