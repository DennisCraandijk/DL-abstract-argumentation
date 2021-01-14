from torch_geometric.nn import GCNConv


class GCN(GCNConv):
    def __init__(self, **kwargs):
        super().__init__(in_channels=1, out_channels=1)
        self.elements = ['node']

    def forward(self, batch, **kwargs):
        x = batch.node_x.unsqueeze(dim=1)
        out = super().forward(x, batch.edge_index, edge_weight=None).squeeze()
        outputs = {'node': [out]}
        return outputs