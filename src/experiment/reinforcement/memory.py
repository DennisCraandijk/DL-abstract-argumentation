from typing import Tuple

import numpy as np
from pl_bolts.models.rl.common.memory import MultiStepBuffer as plMultiStepBuffer


class MultiStepBuffer(plMultiStepBuffer):
    def sample(self, batch_size: int) -> Tuple:
        """
        Takes a sample of the buffer
        Args:
            batch_size: current batch_size
        Returns:
            a batch of tuple np arrays of state, action, reward, done, next_state
        """

        indices = list(
            np.random.choice(len(self.buffer), batch_size - 1, replace=False)
        )
        indices.append(len(self.buffer) - 1)  # append most recent
        states, actions, rewards, dones, next_states = zip(
            *[self.buffer[idx] for idx in indices]
        )

        return (
            states,
            actions,
            rewards,
            dones,
            next_states,
        )
