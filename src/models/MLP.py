import torch.nn as nn
from pl_bolts.models.rl.common.networks import NoisyLinear


class MLP(nn.Module):
    def __init__(
        self, input_size, hidden_size, output_size, num_layers=1, noisy=False, **kwargs
    ):
        super().__init__()
        self.linear_fn = nn.Linear if not noisy else NoisyLinear
        self.activation_fn = nn.ReLU

        self.mlp_modules = self.construct_mlp(
            input_size, hidden_size, output_size, num_layers
        )

    def construct_mlp(
        self, input_size: int, hidden_size: int, output_size: int, num_layers: int
    ):
        """ Stack linear layers with activation"""
        modules = nn.ModuleList([])

        for _ in range(num_layers):
            modules.append(self.linear_fn(in_features=input_size, out_features=hidden_size))
            modules.append(self.activation_fn())
            input_size = hidden_size

        modules.append(self.linear_fn(in_features=hidden_size, out_features=output_size))

        return modules

    def forward(self, x):
        ''' Feedforward through the MLP '''
        for module in self.mlp_modules:
            x = module(x)
        return x
