from collections import OrderedDict

import torch
import torch.nn as nn
from torch.nn.functional import one_hot
from torch_scatter.scatter import scatter_sum

from src.models.GNN import GNN
from src.models.MLP import MLP


class AGNN(GNN):
    def __init__(self, hidden_dim, **kwargs):
        super().__init__(**kwargs)

        self.node_classes = 2
        self.register_buffer("X_v", torch.randn(hidden_dim))
        self.hidden_dim = hidden_dim

        self.M_t = MLP(
            input_size=2 * hidden_dim, hidden_size=hidden_dim, output_size=hidden_dim
        )
        self.M_s = MLP(
            input_size=2 * hidden_dim, hidden_size=hidden_dim, output_size=hidden_dim
        )
        self.U = nn.LSTM(
            input_size=hidden_dim + self.node_classes, hidden_size=hidden_dim
        )
        self.R = MLP(input_size=hidden_dim, hidden_size=hidden_dim, output_size=1)

    def initialize(self, batch):
        """
        Assign embeddings to each node
        """

        initial_node_embedding = self.X_v.unsqueeze(0).expand(batch.num_nodes, -1)
        node_input = one_hot(batch["node_input"], num_classes=self.node_classes).float()

        hiddens = {
            "node_input": node_input,
            "node_embedding": initial_node_embedding,
            "node_lstm": None,
        }

        return hiddens

    def assign_update_fns(self):
        """
        Assign update functions to elements in order
        """
        return OrderedDict(
            {
                "edge_s": self.edge_update_fn,
                "edge_t": self.edge_update_fn,
                "node": self.node_update_fn,
            }
        )

    def edge_update_fn(self, edge_index, node_embedding, element, **kwargs):
        """
        Takes the connected nodes and updates the edge embedding
        """
        src, dest = node_embedding[edge_index[0]], node_embedding[edge_index[1]]
        edge_embedding = torch.cat((src, dest), dim=1)
        edge_embedding = (
            self.M_s(edge_embedding)
            if element == "edge_s"
            else self.M_t(edge_embedding)
        )
        return edge_embedding, None

    def node_update_fn(
        self,
        edge_index,
        node_embedding,
        node_input,
        node_lstm,
        edge_s_embedding,
        edge_t_embedding,
        **kwargs
    ):
        """
        Aggregates the connected edges from source to target and vice versa and updates the node embedding
        """
        messages = scatter_sum(
            src=edge_s_embedding,
            index=edge_index[1],
            dim=0,
            dim_size=node_embedding.shape[0],
        ) + scatter_sum(
            src=edge_t_embedding,
            index=edge_index[0],
            dim=0,
            dim_size=node_embedding.shape[0],
        )
        node_embedding = torch.cat((node_input, messages), dim=1)

        node_embedding = node_embedding.unsqueeze(dim=0)  # [seq,batch,feature]
        node_embedding, node_lstm = self.U(node_embedding, node_lstm)
        node_embedding = node_embedding.squeeze(dim=0)

        return node_embedding, node_lstm

    def assign_readout_fns(self):
        """ Assign readout functions to elements"""
        return {"node": self.node_readout_fn}

    def node_readout_fn(self, node_embedding, **kwargs):
        """ Readout node embeddings """
        return self.R(node_embedding).squeeze()

    def forward(self, batch, **kwargs):
        """ Forward pass a batch """
        output = super().forward(batch, **kwargs)
        return torch.stack(output["node"], dim=0)
