import torch
from torch_geometric.nn import GCNConv


class FM2(GCNConv):
    def __init__(self, **kwargs):
        super().__init__(in_channels=2, out_channels=1)
        self.elements = ['node']

    def forward(self, batch):
        x = torch.cat((batch.node_x_in.unsqueeze(dim=1), batch.node_x_out.unsqueeze(dim=1)), dim=1)
        return super().forward(x, batch.edge_index, edge_weight=None).squeeze().unsqueeze(0)
