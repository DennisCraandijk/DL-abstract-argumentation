from typing import Optional, Union, List, Tuple

import torch
from gym import spaces
from gym.core import Env
from torch.utils.data.sampler import RandomSampler
from torch_geometric.data import Data, Dataset
from torch_geometric.utils import from_networkx
from torch_sparse import coalesce

from src.constants import CRED, STB, COM, GRD, PRF, SCEPT, STRICT, NONSTRICT
from src.data.classes.argumentation_framework import ArgumentationFramework
from src.data.classes.problems.enforcement_problem import (
    ExtensionEnforcementProblem,
    StatusEnforcementProblem,
)
from src.data.solvers.acceptance_verifier import AcceptanceVerifier
from src.data.solvers.enumeration_solver import EnumerationSolver


class EnforcementEnv(Env):
    """ Openai gym environment for enforcement"""

    actions: List[tuple]
    state: Optional[ArgumentationFramework]

    environment_name = "Enforcement"

    def __init__(
            self,
            semantics: str,
            task: str,
            dataset: Dataset = None,
            reward_for_achieving_goal=0,
            reward_for_step=-1,
    ):
        self.semantics = semantics
        self.task = task
        self.verifier = AcceptanceVerifier()
        self.enumeration_solver = EnumerationSolver()
        self.dataset = dataset
        self.sampler = RandomSampler(self.dataset, replacement=True, num_samples=1)

        self.reward_for_step = reward_for_step
        self.reward_for_achieving_goal = reward_for_achieving_goal

        self.state = None
        self.next_state = None
        self.problem = None
        self.actions = []
        self.action_history = None
        self.flipped_edges = None
        self.max_episode_steps = None
        self.step_count = None
        self.reward = None
        self.done = None
        self.at_goal = None

    def reset(
            self,
            problem: Optional[
                Union[ExtensionEnforcementProblem, StatusEnforcementProblem]
            ] = None,
    ):
        """Resets the state of the environment and returns an initial observation.

        Returns:
            observation (object): the initial observation.
        """
        self.problem = problem or self.sample_problem()
        self.state = self.problem.af
        self.actions = list(sorted(self.state.enforcement_representation.edges))
        self.action_space = spaces.Discrete(len(self.actions))
        self.action_history = []
        self.flipped_edges = set()
        self.max_episode_steps = len(self.actions)

        self.next_state = None
        self.step_count = 0
        self.reward = None
        self.done = False
        self.at_goal = False

        return self.observation

    def sample_problem(self):
        """ Sample a new enforcement problem from the sampler"""
        idx = next(iter(self.sampler))
        problem = self.dataset[idx]
        return problem

    def step(self, action: int) -> Tuple[Data, float, bool, dict]:
        """Run one timestep of the environment. W

        Accepts an action and returns a tuple (observation, reward, done, info).

        Args:
            action (int): an action provided by the agent

        Returns:
            observation (object): agent's observation of the current environment
            reward (float) : amount of reward returned after previous action
            done (bool): whether the episode has ended
            info (dict): contains auxiliary information (i.e. for debugging)
        """

        self.next_state = self.take_action(action)
        self.step_count += 1

        if self.user_at_goal(self.next_state):
            self.reward = self.reward_for_achieving_goal
            self.done = True
            self.at_goal = True
        else:
            # normalize reward so total stays in [-1,1]
            self.reward = self.reward_for_step / self.max_episode_steps
            if self.step_count >= self.max_episode_steps:
                self.done = True

        self.state = self.next_state

        return (
            self.observation,
            self.reward,
            self.done,
            {
                "at_goal": self.at_goal,
                "step_count": self.step_count,
                "max_steps": self.max_episode_steps,
            },
        )

    def render(self, mode="human"):
        pass

    def take_action(self, action: int) -> ArgumentationFramework:
        """
        Flip an edge relation and add action to history and flipped edges
        """
        edge = self.actions[action]
        af = self.state.flip_attack(edge)
        self.action_history.append(action)
        self.flipped_edges.add(edge)
        return af

    def verify_strict(self, af: ArgumentationFramework) -> bool:
        """
        Returns whether strict extension enforcement has been achieved
        """
        if self.semantics == STB:
            return self.verifier.is_stable(af, self.problem.desired_extension)
        if self.semantics == COM:
            return self.verifier.is_complete(af, self.problem.desired_extension)
        if self.semantics in [GRD, PRF]:
            # fast precheck
            if not self.verifier.is_complete(af, self.problem.desired_extension):
                return False

            extensions = self.enumeration_solver.solve(af, self.semantics)
            return self.problem.desired_extension in extensions

        raise Exception("Semantics not known")

    def verify_non_strict(self, af: ArgumentationFramework) -> bool:
        """
        Returns whether non-strict extension enforcement has been achieved
        """
        # fast precheck
        if not self.verifier.is_conflict_free(af, self.problem.desired_extension):
            return False

        extensions = self.enumeration_solver.solve(af, self.semantics)
        for extension in extensions:
            if self.problem.desired_extension.issubset(extension):
                return True

        return False

    def quick_reject_status(self, af: ArgumentationFramework, arg) -> bool:
        """
        Fast method to check if an argument is rejected by if an argument is in an admissible extension
        """
        if self.semantics == STB:
            return False

        return not self.verifier.is_in_admissible(af, arg)

    def verify_status(self, af: ArgumentationFramework) -> bool:
        """
        Returns whether status enforcement has been achieved for either CRED and SCEPT
        """

        if any(self.quick_reject_status(af, arg) for arg in self.problem.positive):
            return False

        af.extensions[self.semantics] = self.enumeration_solver.solve(af, self.semantics)
        accepted_args = (
            af.cred_accepted_args(self.semantics)
            if self.task == CRED
            else af.scept_accepted_args(self.semantics)
        )

        return self.problem.positive.issubset(
            accepted_args
        ) and self.problem.negative.isdisjoint(accepted_args)

    def user_at_goal(self, state: ArgumentationFramework) -> bool:
        """Returns boolean indicating whether user at goal location"""

        if self.task == STRICT:
            return self.verify_strict(state)
        if self.task == NONSTRICT:
            return self.verify_non_strict(state)
        if self.task in [CRED, SCEPT]:
            return self.verify_status(state)

        raise Exception("Task not known")

    @property
    def observation(self) -> Data:
        """
        Returns a pytorch geometric grpah object as observation of the AF
        where edge_input contains features wheter the attack exists,
        whether the oppsite attack exists and a self attack exists, and whether an edge have been flipped.
        Node features are set to indicate which arguments should be enforced
        """

        graph = self.state.enforcement_representation
        if self.flipped_edges:
            for u, v in self.flipped_edges:
                graph.edges[u, v]["edge_flipped"] = 1

        data: Data = from_networkx(graph)
        data["edge_input"] = torch.cat(
            [
                data[key].unsqueeze(1)
                for key in ["edge_exists", "edge_opposite", "edge_self", "edge_flipped"]
            ],
            dim=1,
        )

        data.edge_index, data["edge_input"] = coalesce(
            data.edge_index, data["edge_input"], data.num_nodes, data.num_nodes
        )

        if self.task in [STRICT, NONSTRICT]:
            data["node_input"][list(self.problem.desired_extension)] = 1
        elif self.task in [CRED, SCEPT]:
            data["node_input"][list(self.problem.positive)] = 1
            data["node_input"][list(self.problem.negative)] = -1
        return data
