import torch
import torch.nn as nn
from torch_geometric.data import Batch
from torch_geometric.nn import GCNConv

from src.models.MLP import MLP


class GCN(nn.Module):
    def __init__(self, hidden_dim, mp_steps: int, node_classes=1):
        super().__init__()
        self.mp_steps = mp_steps
        self.node_classes = node_classes
        self.hidden_dim = hidden_dim

        self.register_buffer("X_v", torch.randn(hidden_dim))
        self.convolution = GCNConv(in_channels=(hidden_dim + self.node_classes), out_channels=hidden_dim)
        self.readout = MLP(hidden_dim, hidden_dim, 1)

    def message_passing(self, batch: Batch, output_all_steps):
        node_embedding = self.X_v.unsqueeze(0).expand(batch.num_nodes, -1)
        outputs = []
        for step in range(self.mp_steps):
            conv_input = torch.cat((node_embedding, batch["node_input"]), dim=1)
            node_embedding = self.convolution(conv_input, batch.edge_index, edge_weight=None)
            output = self.readout(node_embedding).squeeze()
            if output_all_steps or (step + 1) == self.mp_steps:
                outputs.append(output)

        outputs = torch.stack(outputs, dim=0)
        return outputs

    def forward(self, batch: Batch, output_all_steps=True):
        batch['node_input'] = batch['node_input'].unsqueeze(1)
        return self.message_passing(batch, output_all_steps)


class FM2(GCN):
    def __init__(self, hidden_dim, mp_steps: int):
        node_classes = 3
        super().__init__(hidden_dim, mp_steps, node_classes)

    def forward(self, batch: Batch, output_all_steps=True):
        batch['node_input'] = torch.stack((batch['node_input'], batch['node_x_in'], batch['node_x_out']), dim=1)
        return self.message_passing(batch, output_all_steps)
