from collections import OrderedDict

import torch
import torch.nn as nn
from torch_geometric.data.batch import Batch


class GNN(nn.Module):
    def __init__(self, mp_steps, **config):
        super().__init__()
        self.mp_steps = mp_steps
        self.update_fns = self.assign_update_fns()
        self.readout_fns = self.assign_readout_fns()

    def assign_update_fns(self) -> OrderedDict:
        raise NotImplementedError

    def assign_readout_fns(self) -> dict:
        raise NotImplementedError

    def forward(self, batch: Batch, output_all_steps=False):

        edge_index = batch.edge_index
        sections = (
            torch.bincount(batch.batch).tolist() if hasattr(batch, "batch") else None
        )

        hiddens = self.initialize(batch)

        del batch

        # update attributes with update and aggregation step
        outputs = {element: [] for element in self.readout_fns.keys()}

        for step in range(self.mp_steps):
            hiddens = self.step(edge_index=edge_index, sections=sections, **hiddens)

            if not output_all_steps and (step + 1) != self.mp_steps:
                continue

            for element, readout_fn in self.readout_fns.items():
                outputs[element].append(readout_fn(**hiddens))

        return outputs

    def initialize(self, batch):
        hiddens = {}
        # initialize attributes trough embeddings and intialize lstm states to None
        for element in self.embeddings.keys():
            embedding = self.embeddings[element](batch[f"{element}_input"])
            hiddens.update(
                {
                    f"{element}_input": embedding,
                    f"{element}_embedding": embedding.clone(),
                    f"{element}_lstm": None,
                }
            )
        return hiddens

    def step(self, edge_index, sections, **hiddens):
        """
        Perform a message passing step by propagating information and updating each element
        """
        for element, update_fn in self.update_fns.items():
            hiddens[f"{element}_embedding"], hiddens[f"{element}_lstm"] = update_fn(
                edge_index=edge_index, sections=sections, element=element, **hiddens
            )

        return hiddens
