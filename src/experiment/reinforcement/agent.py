import numpy as np
import torch
from pl_bolts.models.rl.common.agents import Agent
from torch import nn
from torch_geometric.data import Data, Batch

from src.experiment.reinforcement.enforcement_env import EnforcementEnv


class ValueAgent(Agent):
    """Value based agent that returns an action based on the Q values from the network"""

    def __init__(
            self,
            net: nn.Module,
            eps_start: float,
            eps_end: float,
            eps_frames: float,
    ):
        super().__init__(net)
        self.eps_start = eps_start
        self.epsilon = eps_start
        self.eps_end = eps_end
        self.eps_frames = eps_frames

    @torch.no_grad()
    def __call__(self, state: Data, env: EnforcementEnv, device: str, *args, **kwargs) -> int:
        """
        Takes in the current state and returns the action based on the agents policy
        Args:
            state: current state of the environment
            device: the device used for the current batch
        Returns:
            action defined by policy
        """
        if np.random.random() < self.epsilon:
            action = env.action_space.sample()
        else:
            action = self.get_action(state, device)

        return action

    def get_action(self, state: Data, device: str) -> int:
        """
        Returns the best action based on the Q values of the network
        Args:
            state: current state of the environment
            device: the device used for the current batch
        Returns:
            action defined by Q values
        """
        batch = Batch.from_data_list([state])
        batch.to(device)

        q_values = self.net(batch, output_all_steps=False)
        _, action = torch.max(q_values, dim=1)
        return action.item()

    def update_epsilon(self, step: int):
        """
        Updates the epsilon value based on the current step
        Args:
            step: current global step
        """
        self.epsilon = max(self.eps_end, self.eps_start - (step + 1) / self.eps_frames)
