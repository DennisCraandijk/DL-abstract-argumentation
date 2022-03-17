from typing import Optional

import torch
from torch import Tensor
from torch import nn
from torch_geometric.data import Batch
from torch_geometric.nn import MessagePassing, GCNConv
from torch_scatter import scatter

from src.models.MLP import MLP


class GNN(MessagePassing):
    def __init__(self, hidden_dim: int, mp_steps: int, aggregation: str, node_feature_dim,
                 edge_feature_dim):
        """
        Message passing graph neural network
        Args:
            hidden_dim: the dimensions of the vectors
            mp_steps: number of message passing steps
            aggregation: aggregation operator used to aggregate incoming messages
            node_feature_dim: dimensions of node input features
            edge_feature_dim: dimensions of edge input features
        """
        super().__init__()
        self.mp_steps = mp_steps
        self.node_feature_dim = node_feature_dim
        self.edge_feature_dim = edge_feature_dim

        # computes the messages between arguments
        self.message_mlp = MLP(
            input_size=(2 * hidden_dim) + self.edge_feature_dim,
            hidden_size=hidden_dim,
            output_size=hidden_dim,
        )

        # computes aggregation of incoming messages
        if aggregation == "multi":
            self.aggregators = ["std", "min", "max", "mean"]
        elif aggregation in ["sum", "std", "min", "max", "mean", "conv"]:
            self.aggregators = [aggregation]
        else:
            raise Exception("Aggregator not known")

        aggregators_output_size = len(self.aggregators) * hidden_dim
        if "conv" in self.aggregators:
            num_convs = self.edge_feature_dim + 1
            conv_output_size = round(hidden_dim / num_convs)
            self.conv_aggregators = nn.ModuleList([GCNConv(hidden_dim, conv_output_size) for _ in
                                                   range(num_convs)])
            aggregators_output_size += (num_convs * conv_output_size) - hidden_dim

        # computes node embeddings
        self.register_buffer("initial_node_embedding", torch.randn(hidden_dim))
        self.node_update_fn = nn.GRU(
            input_size=aggregators_output_size + self.node_feature_dim, hidden_size=hidden_dim
        )
        self.node_update_state = None

    def forward(self, batch: Batch, output_all_steps=True) -> Tensor:
        """
        Initialize node and edge input vectors,
        update node embeddings through message passing,
        and readout per edge
        """
        node_feature = batch["node_input"].unsqueeze(1).float()
        node_embedding = self.initial_node_embedding.unsqueeze(0).expand(
            batch.num_nodes, -1
        )
        self.node_update_state = None
        edge_feature = batch["edge_input"].float()

        # message passing and readout
        outputs = []
        for mp_step in range(self.mp_steps):
            node_embedding = self.propagate(
                batch.edge_index,
                node_feature=node_feature,
                node_embedding=node_embedding,
                edge_feature=edge_feature,
            )

            if output_all_steps or (mp_step + 1) == self.mp_steps:
                readout = self.readout(node_embedding, batch.edge_index, edge_feature)
                outputs.append(readout)

        return torch.stack(outputs, dim=0)

    # noinspection PyMethodOverriding
    def message(
            self,
            node_embedding_i: Tensor,
            node_embedding_j: Tensor,
            edge_feature: Tensor,
    ) -> Tensor:
        """ Computes message based on node embeddings and edge features """

        message = self.message_mlp(
            torch.cat((node_embedding_i, node_embedding_j, edge_feature), dim=1)
        )
        return message

    def aggregate(
            self,
            inputs: Tensor,
            index: Tensor,
            node_embedding: Tensor,
            edge_index: Tensor,
            edge_feature: Tensor,
            ptr: Optional[Tensor] = None,
            dim_size: Optional[int] = None,
    ) -> Tensor:
        """ Aggregates incoming messages into single fixed sized vector"""

        messages = []
        for aggregator in self.aggregators:
            if aggregator in ["sum", "min", "max", "mean"]:
                message = scatter(
                    inputs,
                    index,
                    dim=0,
                    dim_size=dim_size,
                    reduce=aggregator,
                )
            elif aggregator == "std":
                mean = scatter(inputs, index, 0, None, dim_size, reduce="mean")
                mean_squares = scatter(
                    inputs * inputs, index, 0, None, dim_size, reduce="mean"
                )
                message = mean_squares - mean * mean
                message = torch.sqrt(torch.relu(message) + 1e-5)
            elif aggregator == "conv":
                # use a seperate convolution for each edge feature
                conv_messages = []
                for i, conv_aggregator in enumerate(self.conv_aggregators):
                    edge_weight = edge_feature[:, i] if i < edge_feature.shape[1] else None
                    conv_message = conv_aggregator(node_embedding, edge_index, edge_weight)
                    conv_messages.append(conv_message)
                    message = torch.cat(conv_messages, dim=1)
            else:
                raise ValueError(f'Unknown aggregator "{aggregator}".')

            messages.append(message)

        messages = torch.cat(messages, dim=1)
        return messages

    # noinspection PyMethodOverriding
    def update(
            self,
            messages: Tensor,
            node_feature: Tensor,
            node_embedding: Tensor,
    ) -> Tensor:
        """ Compute updated node embeddings based on incoming messages and node features"""

        node_input = torch.cat((messages, node_feature), dim=1).unsqueeze(
            dim=0
        )  # [seq,batch,feature]
        node_embedding, self.node_update_state = self.node_update_fn(node_input, self.node_update_state)
        node_embedding = node_embedding.squeeze(0)

        return node_embedding

    def readout(self, *args, **kwargs) -> Tensor:
        """ Readout function """
        raise NotImplementedError("Readout not implemented")


class AGNN(GNN):
    """ AGNN used in IJCAI 2020 publication"""

    def __init__(self, hidden_dim: int, mp_steps: int):
        aggregation = 'sum'
        node_feature_dim = 1
        edge_feature_dim = 3
        super().__init__(hidden_dim, mp_steps, aggregation, node_feature_dim, edge_feature_dim)

        # computes readout
        self.node_readout = MLP(input_size=hidden_dim, hidden_size=hidden_dim, output_size=1)

    def readout(self, node_embedding: Tensor, *args) -> Tensor:
        """ Compute edge-level outputs based on two node embeddings and edge feature"""
        out = self.node_readout(node_embedding).squeeze()
        return out


class EGNN(GNN):
    """ EGNN used in AAAI 2022 publication """

    def __init__(self, hidden_dim: int, mp_steps: int, noisy: bool, aggregation='multi'):
        edge_feature_dim = 4
        node_feature_dim = 1
        super().__init__(hidden_dim, mp_steps, aggregation, node_feature_dim, edge_feature_dim)

        # computes readout of attack relation between two arguments
        self.edge_readout = MLP(
            input_size=(2 * hidden_dim) + self.edge_feature_dim,
            hidden_size=hidden_dim,
            output_size=1,
            noisy=noisy,
        )

    def readout(self, node_embedding: Tensor, edge_index: Tensor, edge_feature: Tensor) -> Tensor:
        """ Compute edge-level outputs based on two node embeddings and edge feature"""
        src, dest = node_embedding[edge_index[0]], node_embedding[edge_index[1]]
        out = self.edge_readout(torch.cat((src, dest, edge_feature), dim=1)).squeeze()
        return out
