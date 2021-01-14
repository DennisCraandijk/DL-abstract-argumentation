import argparse
import multiprocessing
import pickle
from multiprocessing.pool import Pool
from pathlib import Path

from tqdm import tqdm

from src.data.classes.ArgumentationFramework import ArgumentationFramework
from src.data.scripts.utils import limit_cpu
from src.data.solvers.AcceptanceSolver import AcceptanceSolver

path = Path(__file__).parent
data_dir = path.parent.parent / 'data'
solver = path / 'solvers/mu-toksia/mu-toksia_static'


def main(args):
    """
    Take apx files in graphs_dir, build AF object, compute the extensions and save as pickle in dataset_dir
    """

    # make multiprocessing pool to compute AF semantics
    apx_paths = list(graphs_dir.glob('*.apx'))
    processes = min(max(multiprocessing.cpu_count() - 1, 1), args.max_processes)
    pool = Pool(processes=processes, initializer=limit_cpu)
    for _ in tqdm(pool.imap(graph2AF, apx_paths), total=len(apx_paths), desc='Solve'):
        pass


def graph2AF(apx_file_path: Path):
    solver = AcceptanceSolver()
    id = apx_file_path.stem
    # return AF from state_dict pickle if already exists
    AF_pkl_path = AFs_dir / f'{id}.pkl'
    if AF_pkl_path.exists():
        return

    # otherwise build AF from graph file and compute extensions

    with open(apx_file_path, 'r') as f:
        AF = ArgumentationFramework.from_apx(f.read(), id=id)

    for semantics in args.semantics:
        AF.extensions[semantics] = solver.solve(AF, semantics)

    if args.validation:
        validate_extensions(AF)

    pickle.dump(AF.state_dict, open(AF_pkl_path, 'wb+'))


def validate_extensions(AF: ArgumentationFramework) -> bool:
    assert len(AF.extensions['PR']) >= 1 and len(AF.extensions['GR']) == 1 and len(AF.extensions['CO']) >= 1
    for PR in AF.extensions['PR']:
        assert list(AF.extensions['GR'])[0].issubset(PR)
        assert PR in AF.extensions['CO']
    for ST in AF.extensions['ST']:
        assert ST in AF.extensions['PR']
        assert list(AF.extensions['GR'])[0] in AF.extensions['CO']
        assert AF.verify_stable(ST)
    for CO in AF.extensions['CO']:
        assert AF.verify_complete(CO)

    return True


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--semantics', nargs='+', default=['PR', 'GR', 'ST', 'CO'], help='Semantics')
    parser.add_argument('--name', type=str, default='debug', help='Dataset name')
    parser.add_argument('--max_processes', type=int, default=16)
    parser.add_argument('--validation', action='store_true')
    parser.set_defaults(multiprocessing=True)
    args = parser.parse_args()

    graphs_dir = data_dir / f'dataset/{args.name}/graphs'
    AFs_dir = data_dir / f'dataset/{args.name}/AFs'
    AFs_dir.mkdir(parents=True, exist_ok=True)
    main(args)
