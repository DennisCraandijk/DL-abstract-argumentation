import argparse
import pickle
import random
from pathlib import Path
from typing import Tuple

import pandas as pd
import torch
from torch.utils.data import Dataset
from torch_geometric.data.data import Data
from torch_geometric.data.in_memory_dataset import InMemoryDataset
from torch_geometric.utils import from_networkx

from src import config
from src.data.classes.ArgumentationFramework import ArgumentationFramework


class ArgumentationFrameworkDS(Dataset):
    """
    A dataset of Argumentation Frameworks from a dataset directory
    """

    def __init__(self, name, representation="base"):
        self.dir = config.dataset_dir / name / "AFs"
        self.AFs = list((self.dir).glob("*.pkl"))
        self.representation = representation

    def __len__(self) -> int:
        return len(self.AFs)

    def __getitem__(self, idx) -> Tuple[ArgumentationFramework, Data]:
        AF = ArgumentationFramework.from_pkl(self.AFs[idx])
        graph = AF.get_representation(self.representation)
        data = from_networkx(graph)
        data["idx"] = idx
        return AF, data


class AcceptanceProblemDS(Dataset):
    """
    Dataset of acceptance problems on Argumentation Frameworks from a dataset directory
    """

    def __init__(self, name, task, semantics, representation):

        self.AF_ds = ArgumentationFrameworkDS(name, representation)
        self.name = name
        self.representation = representation
        self.task = task
        self.semantics = semantics

    def __len__(self) -> int:
        return len(self.AF_ds)

    def __getitem__(self, idx: int) -> Data:
        AF, data = self.AF_ds[idx]
        S = set()
        if self.task == "enum":
            S = self.sample_set_of_arguments(AF)
            Y = AF.get_cred_accepted_args(self.semantics, S=S)
        elif self.task == "scept":
            Y = AF.get_scept_accepted_args(self.semantics)
        else:
            Y = AF.get_cred_accepted_args(self.semantics)

        data["node_input"][list(S)] = 1
        data["node_y"][list(Y)] = 1
        data["node_y"] = data["node_y"].float()

        return data

    def sample_set_of_arguments(self, AF: ArgumentationFramework) -> frozenset:
        """
        Generate a datapoint for the enumeration tree search with either a
        empty S, legal S or illegal S
        """
        S = frozenset()

        if len(AF.extensions[self.semantics]) == 0:
            return S

        random_extension = random.choice(list(AF.extensions[self.semantics]))

        if len(random_extension) == 0:
            return S

        # subset of the randomly chosen extension
        S = frozenset(
            random.sample(random_extension, random.randrange(0, len(random_extension)))
        )

        type = random.choice(["legal", "illegal"])
        if type == "legal":
            return S

        # an illegal set has none of the extensions as its superset
        if type == "illegal":
            possible_illegal_additions = AF.arguments - (
                S | AF.get_cred_accepted_args(self.semantics, S=S)
            )
            if len(possible_illegal_additions) != 0:
                S = S | frozenset(random.sample(possible_illegal_additions, 1))

        return S


class MemoryDS(InMemoryDataset):
    """Dataset base class for creating graph datasets which fit completely
    into memory. Saves processed files
    See `here <https://pytorch-geometric.readthedocs.io/en/latest/notes/
    create_dataset.html#creating-in-memory-datasets>`__ for the accompanying
    tutorial."""

    def __init__(self, ds, semantics, task, representation):
        self._processed_file_names = [f"{semantics}_{task}_{representation}_data.pt"]
        self.ds = ds

        root = config.dataset_dir / ds.name
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
        data, slices = self.collate([d for d in self.ds])
        torch.save((data, slices), self.processed_paths[0])
