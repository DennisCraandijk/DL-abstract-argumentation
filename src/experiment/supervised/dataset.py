import random
from typing import Tuple

import torch
from torch.utils.data import Dataset
from torch_geometric.data.data import Data
from torch_geometric.data.in_memory_dataset import InMemoryDataset
from torch_geometric.utils import from_networkx

from src import config
from src.constants import SCEPT, ENUM
from src.data.classes.argumentation_framework import ArgumentationFramework


class ArgumentationFrameworkDS(Dataset):
    """
    A dataset of Argumentation Frameworks from a dataset directory
    """

    def __init__(self, name: str, representation: str = "base"):
        self.dir = config.dataset_dir / name / "AFs"
        self.afs = list(self.dir.glob("*.pkl"))
        self.representation = representation

    def __len__(self) -> int:
        return len(self.afs)

    def __getitem__(self, idx) -> Tuple[ArgumentationFramework, Data]:
        """ Return AF and data representaiton of idx-th AF"""
        af = self.get_af(idx)
        graph = af.graph_representation(self.representation)
        data = from_networkx(graph)
        data["idx"] = idx
        data["edge_input"] = torch.cat(
            [
                data[key].unsqueeze(1)
                for key in ["edge_exists", "edge_opposite", "edge_self"]
            ],
            dim=1,
        )
        return af, data

    def get_af(self, idx: int) -> ArgumentationFramework:
        """ Load af from pickle on disk based on idx in dataset"""
        return ArgumentationFramework.from_pkl(self.afs[idx])


class AcceptanceProblemDS(Dataset):
    """
    Dataset of acceptance problems on Argumentation Frameworks from a dataset directory
    """

    def __init__(self, name: str, task: str, semantics: str, representation: str):

        self.af_dataset = ArgumentationFrameworkDS(name, representation)
        self.name = name
        self.representation = representation
        self.task = task
        self.semantics = semantics

    def __len__(self) -> int:
        return len(self.af_dataset)

    def __getitem__(self, idx: int) -> Data:
        """ Return data representation of an AF with the corresponding node input features and labels"""
        af, data = self.af_dataset[idx]
        arg_set = set()
        if self.task == ENUM:
            arg_set = self.sample_set_of_arguments(af)
            accepted_args = af.cred_accepted_args(self.semantics, filter_args=arg_set)
        elif self.task == SCEPT:
            accepted_args = af.scept_accepted_args(self.semantics)
        else:
            accepted_args = af.cred_accepted_args(self.semantics)

        data["node_input"][list(arg_set)] = 1
        data["node_y"][list(accepted_args)] = 1
        data["node_y"] = data["node_y"].float()

        return data

    def sample_set_of_arguments(self, af: ArgumentationFramework) -> frozenset:
        """
        Generate a datapoint for the enumeration tree search with either an
        empty S, legal S or illegal S
        """
        arg_set = frozenset()

        if len(af.extensions[self.semantics]) == 0:
            return arg_set

        random_extension = random.choice(list(af.extensions[self.semantics]))

        if len(random_extension) == 0:
            return arg_set

        # subset of the randomly chosen extension
        arg_set = frozenset(
            random.sample(random_extension, random.randrange(0, len(random_extension)))
        )

        sample_type = random.choice(["legal", "illegal"])
        if sample_type == "legal":
            return arg_set

        # an illegal set has none of the extensions as its superset
        if sample_type == "illegal":
            possible_illegal_additions = af.arguments - (
                    arg_set | af.cred_accepted_args(self.semantics, filter_args=arg_set)
            )
            if len(possible_illegal_additions) != 0:
                arg_set = arg_set | frozenset(random.sample(possible_illegal_additions, 1))

        return arg_set


class MemoryDS(InMemoryDataset):
    """Dataset base class for creating graph datasets which fit completely
    into memory. Saves processed files
    See `here <https://pytorch-geometric.readthedocs.io/en/latest/notes/
    create_dataset.html#creating-in-memory-datasets>`__ for the accompanying
    tutorial."""

    def __init__(self, dataset, semantics: str, task: str, representation: str):
        self._processed_file_names = [f"{semantics}_{task}_{representation}_data.pt"]
        self.dataset = dataset

        root = config.dataset_dir / dataset.name
        super().__init__(root=root, transform=None, pre_transform=None)
        self.data, self.slices = torch.load(self.processed_paths[0])

    @property
    def raw_file_names(self):
        return ["AFs.pkl"]

    @property
    def processed_file_names(self):
        return self._processed_file_names

    def download(self):
        pass

    def process(self):
        """Processes the dataset to the :obj:`self.processed_dir` folder."""
        data, slices = self.collate(list(self.dataset))
        torch.save((data, slices), self.processed_paths[0])
