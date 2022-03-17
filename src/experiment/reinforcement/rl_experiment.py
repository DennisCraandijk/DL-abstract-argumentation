import argparse
import copy
import math
from timeit import default_timer as timer
from typing import Tuple, List, Union, Optional

import networkx
import pytorch_lightning as pl
import torch
from pl_bolts.datamodules import ExperienceSourceDataset
from pl_bolts.datamodules.experience_source import Experience
from pytorch_lightning.callbacks import ModelCheckpoint, EarlyStopping
from pytorch_lightning.loggers import WandbLogger
from torch import optim
from torch.nn.functional import mse_loss
from torch.utils.data import DataLoader
from torch.utils.hipify.hipify_python import str2bool
from torch_geometric.data import DataLoader as GraphDataLoader, Batch
from tqdm import tqdm

from src import config
from src.constants import GRD, STB, COM, PRF, STRICT, NONSTRICT, CRED, SCEPT
from src.data.classes.problems.enforcement_problem import ExtensionEnforcementProblem, StatusEnforcementProblem
from src.data.solvers.enumeration_solver import EnumerationSolver
from src.experiment.reinforcement.agent import ValueAgent
from src.experiment.reinforcement.dataset import EnforcementProblemDS
from src.experiment.reinforcement.enforcement_env import EnforcementEnv
from src.experiment.reinforcement.memory import MultiStepBuffer
from src.models.GNN import EGNN


class RLExperiment(pl.LightningModule):
    """ Basic DQN Model """

    def __init__(
            self,
            semantics: str,
            task: str,
            task_full: Optional[str],
            hidden_dim: int,
            train_ds: str,
            val_ds: str,
            test_ds: str,
            aggregation: str,
            mp_steps: int,
            noisy: bool,
            eps_start: float,
            eps_end: float,
            eps_last_frame: int,
            sync_rate: int,
            gamma: float,
            loss_all_steps: bool,
            lr: float,
            cyc_policy: bool,
            batch_size: int,
            replay_size: int,
            warm_start_size: int,
            batches_per_epoch: int,
            n_steps: int,
            reward_for_achieving_goal: int,
            reward_for_step: int,
            avg_reward_len: int,
            seed: int = 123,
            fast_dev_run=False,
            tag=None,
            **kwargs,
    ):
        """

        Args:
            semantics: argumentation semantics
            task: enforcement task
            task_full: both semantics and task in single string
            hidden_dim: the number of dimensions of the embeddings and neural networks
            train_ds: train dataset
            val_ds: validation dataset
            test_ds: test dataset(s)
            aggregation: choose [sum, conv, multi]
            mp_steps: number of message passing steps
            noisy: whether to use noisy exploration
            eps_start: starting value of epsilon for the epsilon-greedy exploration
            eps_end: final value of epsilon for the epsilon-greedy exploration
            eps_last_frame: the final frame in for the decrease of epsilon. At this frame espilon = eps_end
            sync_rate: the number of iterations between syncing up the target network with the train network
            gamma: discount factor
            lr: learning rate
            loss_all_steps: calculate loss over all message passing steps or only final step
            cyc_policy: cyclic learning rate
            batch_size: size of minibatch pulled from the DataLoader
            replay_size: total capacity of the replay buffer
            warm_start_size: how many random steps through the environment to be carried out at the start of
                training to fill the buffer with a starting point
            avg_reward_len: how many episodes to take into account when calculating the avg reward
            min_episode_reward: the minimum score that can be achieved in an episode. Used for filling the avg buffer
                before training begins
            seed: seed value for all RNG used
            batches_per_epoch: number of batches per epoch
            n_steps: size of n step look ahead
            reward_for_achieving_goal:
            reward_for_step:
            avg_reward_len:
            seed:
            fast_dev_run:
            tag:
            **kwargs:
        """
        super().__init__()

        # Task
        if task_full:
            semantics, task = task_full.split("_")
        assert semantics in [COM, STB, PRF, GRD]
        assert task in [STRICT, NONSTRICT, CRED, SCEPT]
        self.semantics = semantics
        self.task = task

        # Environment
        self.acceptance_solver = EnumerationSolver()
        self.reward_for_achieving_goal = reward_for_achieving_goal
        self.reward_for_step = reward_for_step

        # Dataset
        self.train_ds_name = train_ds
        self.val_ds_name = val_ds
        self.test_ds_names = test_ds.split(" ")
        self.buffer = None
        self.dataset = None

        # Model
        self.aggregation = aggregation
        self.net = EGNN(
            hidden_dim=hidden_dim,
            mp_steps=mp_steps,
            noisy=noisy,
            aggregation=aggregation,
        )
        self.target_net = EGNN(
            hidden_dim=hidden_dim,
            mp_steps=mp_steps,
            noisy=noisy,
            aggregation=aggregation,
        )

        # Agent
        self.eps_start = eps_start if not noisy else 0
        self.eps_end = eps_end if not noisy else 0
        self.eps_last_frame = eps_last_frame if not noisy else 1
        self.agent = ValueAgent(
            self.net,
            eps_start=self.eps_start,
            eps_end=self.eps_end,
            eps_frames=self.eps_last_frame,
        )

        # Checkpointing
        if tag:
            self.model_dir = config.models_dir / "RL" / tag
            self.model_name = f"{self.train_ds_name}-mp{mp_steps}_{self.semantics}_{self.task}"
        self.resumed = False

        # Hyperparameters
        self.sync_rate = sync_rate
        self.gamma = gamma
        self.loss_all_steps = loss_all_steps
        self.lr = lr
        self.cyc_policy = cyc_policy
        self.batch_size = batch_size
        self.replay_size = replay_size
        self.warm_start_size = warm_start_size if not fast_dev_run else 100
        self.batches_per_epoch = batches_per_epoch
        self.n_steps = n_steps

        self.save_hyperparameters()

        # Metrics
        self.total_at_goal = [0]
        self.total_rewards = [0]
        self.total_step_count = [0]
        self.done_episodes = 0
        self.total_steps = 0
        self.avg_reward_len = avg_reward_len
        self.avg_rewards = 0
        self.avg_at_goal = 0
        self.avg_steps = 0

        # Make environment
        self.val_env = self.make_environment(seed=seed)
        if self.train_ds_name:
            self.train_env_dl = self.env_ds(train_ds, fixed_goals=False)
            self.env = self.make_environment(self.train_env_dl, seed)
            self.state = self.env.reset()

    def resume(self, name: str):
        """
        Load pretrained model
        Args:
            name: name or tag used in previous run
        """
        path = self.model_dir / f"{name}_{self.semantics}_{self.task}.ckpt"
        print(f"Resume {path}")
        assert path.exists()
        map_location = None if self.on_gpu else torch.device("cpu")
        state_dict = torch.load(path, map_location)["state_dict"]
        self.load_state_dict(state_dict)
        self.resumed = True

    def run_episode(
            self,
            problem: Union[ExtensionEnforcementProblem, StatusEnforcementProblem],
            epsilon: float = 0.0,
    ) -> dict:
        """
        Carries out an of the environment with the current agent and returns after the final step
        Args:
            problem: Enforcement problem
            epsilon: epsilon value for DQN agent
        """

        episode_state = self.val_env.reset(problem)
        episode_reward = 0
        info = None

        done = False
        while not done:
            self.agent.epsilon = epsilon
            action = self.agent(episode_state, self.val_env, self.device)
            next_state, reward, done, info = self.val_env.step(action)
            episode_state = next_state
            episode_reward += reward

        return info

    def populate(self, warm_start: int) -> None:
        """Populates the buffer with initial experiences"""
        if warm_start > 0:
            self.state = self.env.reset()

            for _ in tqdm(range(warm_start), desc="Populate replay buffer"):
                if not self.resumed:
                    self.agent.epsilon = 1.0
                action = self.agent(self.state, self.env, self.device)
                next_state, reward, done, _ = self.env.step(action)
                exp = Experience(
                    state=self.state,
                    action=action,
                    reward=reward,
                    done=done,
                    new_state=next_state,
                )
                self.buffer.append(exp)
                self.state = next_state

                if done:
                    self.state = self.env.reset()

    def train_batch(
            self,
    ) -> Tuple[torch.Tensor, torch.Tensor, torch.Tensor, torch.Tensor, torch.Tensor]:
        """
        Contains the logic for generating a new batch of data to be passed to the DataLoader
        Returns:
            yields a Experience tuple containing the state, action, reward, done and next_state.
        """
        episode_reward = 0
        episode_steps = 0

        while True:
            self.total_steps += 1
            action = self.agent(self.state, self.env, self.device)

            next_state, reward, is_done, info = self.env.step(action)

            episode_reward += reward
            episode_steps += 1

            exp = Experience(
                state=self.state,
                action=action,
                reward=reward,
                done=is_done,
                new_state=next_state,
            )

            self.agent.update_epsilon(self.global_step)
            self.buffer.append(exp)
            self.state = next_state

            if is_done:
                self.done_episodes += 1

                self.total_rewards.append(episode_reward)
                self.total_step_count.append(episode_steps)
                self.total_at_goal.append(info["at_goal"])

                self.state = self.env.reset()
                episode_steps = 0
                episode_reward = 0

            states, actions, rewards, dones, new_states = self.buffer.sample(
                self.batch_size
            )

            for i, _ in enumerate(dones):
                yield states[i], actions[i], rewards[i], dones[i], new_states[i]

            # Simulates epochs
            if self.total_steps % self.batches_per_epoch == 0:
                break

    def training_step(
            self, batch: Tuple[Batch, torch.Tensor, torch.Tensor, torch.Tensor, Batch], *args, **kwargs
    ) -> torch.Tensor:
        """
        Carries out a single step through the environment to update the replay buffer.
        Then calculates loss based on the minibatch recieved
        Args:
            batch: current mini batch of replay data
            _: batch number, not used
        Returns:
            Training loss and log metrics
        """

        # calculates training loss
        loss = self.dqn_loss(batch)

        # Soft update of target network
        if self.global_step % self.sync_rate == 0:
            self.target_net.load_state_dict(self.net.state_dict())

        self.avg_rewards = sum(self.total_rewards[-self.avg_reward_len:]) / min(
            len(self.total_rewards), self.avg_reward_len
        )
        self.avg_steps = sum(self.total_step_count[-self.avg_reward_len:]) / min(
            len(self.total_step_count), self.avg_reward_len
        )
        self.avg_at_goal = sum(self.total_at_goal[-self.avg_reward_len:]) / min(
            len(self.total_at_goal), self.avg_reward_len
        )

        self.log("agent/avg_reward", self.avg_rewards)
        self.log("agent/avg_steps", self.avg_steps)
        self.log("agent/avg_at_goal", self.avg_at_goal)
        self.log("agent/loss", loss)
        self.log("training/episodes", self.done_episodes, prog_bar=True)
        self.log("training/steps", self.global_step, prog_bar=True)
        self.log("training/epsilon", self.agent.epsilon, prog_bar=True)
        self.log("training/lr", self.trainer.optimizers[0].param_groups[0]["lr"])

        return loss

    def dqn_loss(
            self, batch: Tuple[Batch, torch.Tensor, torch.Tensor, torch.Tensor, Batch]
    ) -> torch.Tensor:
        """
        Calculates the mse loss using a mini batch from the replay buffer
        Returns:
            loss
        """
        states, actions, rewards, dones, next_states = batch
        # actions = actions.long().squeeze(-1)

        # seperate sections by fully connected edges
        sections = (torch.bincount(states.batch) ** 2).tolist()

        # loop over each AF and select de q_value corresponding to the action taken in that graph
        out = self.net(states, output_all_steps=self.loss_all_steps)
        state_action_values = torch.stack(
            [
                values_per_step[:, actions[idx]]
                for idx, values_per_step in enumerate(out.split(sections, dim=1))
            ]
        )
        self.log("agent/q_average", state_action_values.mean())
        self.log("agent/q_min", state_action_values.min())
        self.log("agent/q_max", state_action_values.max())

        with torch.no_grad():
            target_output = self.target_net(next_states, output_all_steps=False)
            next_state_values = torch.stack(
                [
                    values_per_step.max(dim=1)[0]
                    for values_per_step in target_output.split(sections, dim=1)
                ]
            ).squeeze()
            next_state_values[dones] = 0.0
            next_state_values = next_state_values.detach()

        expected_state_action_values = next_state_values * self.gamma + rewards
        self.log("agent/y_average", expected_state_action_values.mean())

        expected_state_action_values = expected_state_action_values.unsqueeze(
            dim=1
        ).expand_as(state_action_values)
        loss = mse_loss(state_action_values, expected_state_action_values)
        return loss

    def _eval_dataloader(self, ds_name: str) -> DataLoader:
        """ Returns enforcement evaluation dataloader """
        dataset = EnforcementProblemDS(
            (config.dataset_dir / ds_name), self.task, fixed_goals=True
        )
        dataloader = DataLoader(dataset, batch_size=None)
        return dataloader

    def val_dataloader(self) -> DataLoader:
        return self._eval_dataloader(self.val_ds_name)

    def validation_step(
            self,
            problem: Union[ExtensionEnforcementProblem, StatusEnforcementProblem],
            *args,
            **kwargs,
    ) -> dict:
        """
        Operates on a single batch of data from the validation set and returns validation metrics
        """
        info = self.run_episode(problem)

        optimality = None
        if (
                self.val_env.semantics in problem.optimal_solution
                and problem.optimal_solution[self.val_env.semantics]
        ):
            optimality = (
                    info["step_count"] / problem.optimal_solution[self.val_env.semantics]
            )

        return {
            "at_goal": info["at_goal"],
            "step_count": info["step_count"],
            "max_steps": info["max_steps"],
            "optimality": optimality,
        }

    def validation_epoch_end(self, outputs: list):
        """ Aggregate and log the outputs of all validation steps """

        outputs = {k: [dic[k] for dic in outputs] for k in outputs[0]}

        avg_accuracy = sum(outputs["at_goal"]) / len(outputs["at_goal"])
        avg_step_count = sum(outputs["step_count"]) / len(outputs["step_count"])
        avg_max_steps = sum(outputs["max_steps"]) / len(outputs["max_steps"])

        avg_optimality = None
        outputs["optimality"] = list(filter(None, outputs["optimality"]))
        if len(outputs["optimality"]) > 0:
            avg_optimality = sum(outputs["optimality"]) / len(outputs["optimality"])

        # every not 100% accurate gets negative score
        score = math.floor(avg_accuracy) - (avg_step_count / avg_max_steps)

        self.log("validation/accuracy", avg_accuracy)
        self.log("validation/step_count", avg_step_count)
        self.log("validation/score", score)
        self.log("validation/optimality", avg_optimality)

    def test_step(
            self,
            problem: Union[ExtensionEnforcementProblem, StatusEnforcementProblem],
            *args,
            **kwargs,
    ) -> dict:
        """Operates on a single batch of data from the test set."""

        start = timer()
        info = self.run_episode(problem)
        end = timer()

        optimality = None
        optimal_num = None
        optimal_time = None
        density = None
        optimal_known = False

        step_count = info["step_count"] if info["at_goal"] else None

        if (
                self.val_env.semantics in problem.optimal_solution
                and problem.optimal_solution[self.val_env.semantics]
                and info["at_goal"]
        ):
            optimality = (
                    step_count / problem.optimal_solution[self.val_env.semantics]
            )
            optimal_num = problem.optimal_solution[self.val_env.semantics]
            optimal_time = problem.solve_times[self.val_env.semantics]
            density = networkx.density(problem.af.graph)

            optimal_known = (
                    problem.optimal_solution[self.val_env.semantics] is not None
            )

        return {
            "at_goal": info["at_goal"],
            "step_count": step_count,
            "max_steps": info["max_steps"],
            "optimality": optimality,
            "optimal_known": optimal_known,
            "optimal_num": optimal_num,
            "optimal_time": optimal_time,
            "density": density,
            "time": (end - start),
        }

    def test_epoch_end(self, outputs):
        """ Aggregate and log the outputs of all validation steps """

        # if only one dataloader is provided, still make output nested as if multiple were provided
        if len(self.test_ds_names) == 1 and not isinstance(outputs[0], list):
            outputs = [outputs]

        for i, ds_name in enumerate(self.test_ds_names):
            output = {k: [dic[k] for dic in outputs[i]] for k in outputs[i][0]}  # list-dict to dict-list
            step_count = list(filter(None, output["step_count"]))
            avg_step_count = (
                sum(step_count) / len(step_count) if len(step_count) else 0
            )
            avg_accuracy = sum(output["at_goal"]) / len(output["at_goal"])
            avg_optimality = 0
            optimal_num = 0

            optimality_gaps = list(filter(None, output["optimality"]))
            if len(optimality_gaps) > 0:
                avg_optimality = sum(optimality_gaps) / len(optimality_gaps)
                optimal_num = sum(filter(None, output["optimal_num"])) / len(
                    list(filter(None, output["optimal_num"]))
                )
            optimal_known = sum(output["optimal_known"]) / len(output["optimal_known"])
            avg_time = sum(output["time"]) / len(output["time"])

            self.log(f"{ds_name}/step_count", avg_step_count)
            self.log(f"{ds_name}/accuracy", avg_accuracy)
            self.log(f"{ds_name}/optimality", avg_optimality)
            self.log(f"{ds_name}/optimal_known", optimal_known)
            self.log(f"{ds_name}/optimal_num", optimal_num)
            self.log(f"{ds_name}/avg_time", avg_time)

    def configure_optimizers(self):
        """ Initialize Adam optimizer and cyclical learning rate"""
        optimizer = optim.AdamW(self.net.parameters(), lr=self.lr)
        optimizers = [optimizer]
        schedulers = []

        if self.cyc_policy:
            step_size_up = int(self.batches_per_epoch / 2)
            schedulers = [
                {
                    "interval": "step",
                    "frequency": 1,
                    "scheduler": optim.lr_scheduler.CyclicLR(
                        optimizer,
                        self.lr / 1e4,
                        max_lr=self.lr,
                        step_size_up=step_size_up,
                        cycle_momentum=False,
                    ),
                }
            ]
        return optimizers, schedulers

    def train_dataloader(self) -> DataLoader:
        """Initialize the Replay Buffer dataset used for retrieving experiences"""
        self.buffer = MultiStepBuffer(self.replay_size, self.n_steps)
        self.populate(self.warm_start_size)

        self.dataset = ExperienceSourceDataset(self.train_batch)
        dataloader = GraphDataLoader(dataset=self.dataset, batch_size=self.batch_size)
        return dataloader

    def env_ds(self, dataset: str, fixed_goals: bool = True) -> EnforcementProblemDS:
        """ Returns enforcement problem dataset based on the dataset name and the desired task"""
        dataset = EnforcementProblemDS((config.dataset_dir / dataset), self.task, fixed_goals)
        return dataset

    def test_dataloader(self) -> List[DataLoader]:
        """Get test loader"""
        dataloaders = [self._eval_dataloader(name) for name in self.test_ds_names]
        return dataloaders

    def make_environment(self, env_dl=None, seed: int = None) -> EnforcementEnv:
        """
        Initialise gym  environment
        Returns:
            gym environment
        """

        env = EnforcementEnv(
            self.semantics,
            self.task,
            env_dl,
            reward_for_achieving_goal=self.reward_for_achieving_goal,
            reward_for_step=self.reward_for_step,
        )

        if seed:
            env.seed(seed)

        return env

    @staticmethod
    def add_model_specific_args(
            arg_parser: argparse.ArgumentParser,
    ) -> argparse.ArgumentParser:
        """
        Adds arguments for DQN model
        Args:
            arg_parser: parent parser
        """
        arg_parser.add_argument(
            "--sync_rate",
            type=int,
            default=1000,
            help="how many frames do we update the target network",
        )
        arg_parser.add_argument(
            "--replay_size",
            type=int,
            default=100000,
            help="capacity of the replay buffer",
        )
        arg_parser.add_argument(
            "--warm_start_size",
            type=int,
            default=10000,
            help="how many samples do we use to fill our buffer at the start of training",
        )
        arg_parser.add_argument(
            "--eps_last_frame",
            type=int,
            default=0,
            help="what frame should epsilon stop decaying",
        )
        arg_parser.add_argument(
            "--eps_start", type=float, default=0, help="starting value of epsilon"
        )
        arg_parser.add_argument(
            "--eps_end", type=float, default=0, help="final value of epsilon"
        )
        arg_parser.add_argument(
            "--batches_per_epoch",
            type=int,
            default=100000,
            help="number of batches in an epoch",
        )
        arg_parser.add_argument(
            "--batch_size", type=int, default=30, help="size of the batches"
        )
        arg_parser.add_argument("--lr", type=float, default=2e-5, help="learning rate")
        arg_parser.add_argument("--cyc_policy", type=bool, default=True)
        arg_parser.add_argument("--loss_all_steps", type=str2bool, default=False)

        arg_parser.add_argument(
            "--gamma", type=float, default=1, help="discount factor"
        )

        arg_parser.add_argument("--reward_for_step", type=int, default=-1)
        arg_parser.add_argument("--reward_for_achieving_goal", type=int, default=0)

        arg_parser.add_argument(
            "--avg_reward_len",
            type=int,
            default=100,
            help="how many episodes to include in avg reward",
        )
        arg_parser.add_argument(
            "--n_steps",
            type=int,
            default=1,
            help="how many frames do we update the target network",
        )

        # model args
        arg_parser.add_argument("--hidden_dim", type=int, default=128)
        arg_parser.add_argument(
            "--aggregation", choices=["multi", "sum", "conv"], default="multi"
        )
        arg_parser.add_argument("--mp_steps", type=int, default=5)
        arg_parser.add_argument("--noisy", type=str2bool, default=True)

        # data args
        arg_parser.add_argument("--resume", type=str, default=None)
        arg_parser.add_argument("--tag", type=str, default="")
        arg_parser.add_argument(
            "--semantics",
            choices=[GRD, PRF, STB, COM],
            default=COM,
            help="Semantics",
        )
        arg_parser.add_argument(
            "--task",
            choices=[STRICT, NONSTRICT, CRED, SCEPT],
            default=STRICT,
        )
        arg_parser.add_argument("--task_full", type=str, default=None)
        arg_parser.add_argument(
            "--train_ds", type=str, default=None, help="Training dataset name"
        )
        arg_parser.add_argument(
            "--val_ds", type=str, default=None, help="Validation dataset name"
        )
        arg_parser.add_argument(
            "--test_ds", type=str, default=None, help="Test dataset name"
        )
        arg_parser.add_argument(
            "--patience", type=int, default=None, help="Early stopping patience"
        )

        # default pl to GPU
        arg_parser.set_defaults(gpus=1)
        arg_parser.set_defaults(gradient_clip_val=1)
        arg_parser.set_defaults(limit_val_batches=1000)
        return arg_parser


def cli_main():
    parser = argparse.ArgumentParser(add_help=False)

    # trainer args
    parser = pl.Trainer.add_argparse_args(parser)

    # model args
    parser = RLExperiment.add_model_specific_args(parser)
    args = parser.parse_args()

    model = RLExperiment(**args.__dict__)

    if args.resume:
        model.resume(args.resume)
    logger = None
    callbacks = []
    if not args.fast_dev_run:
        logger = WandbLogger(
            project="enforcement", save_dir=str(config.root_dir), tags=[args.tag]
        )
        logger.log_hyperparams(args)

        # save checkpoints based on avg_reward
        checkpoint_callback = ModelCheckpoint(
            dirpath=logger.experiment.dir,
            save_top_k=1,
            monitor="validation/score",
            mode="max",
            verbose=True,
            save_weights_only=True,
        )
        callbacks.append(checkpoint_callback)

        if args.tag:
            tag_checkpoint_callback = copy.deepcopy(checkpoint_callback)
            tag_checkpoint_callback.dirpath = model.model_dir
            tag_checkpoint_callback.filename = model.model_name
            callbacks.append(tag_checkpoint_callback)

    if args.patience:
        early_stopping_callback = EarlyStopping(
            monitor="validation/score", patience=args.patience, verbose=True, mode="max", min_delta=0.001
        )
        callbacks.append(early_stopping_callback)

    pl.seed_everything(123)
    trainer = pl.Trainer.from_argparse_args(
        args,
        logger=logger,
        callbacks=callbacks,
        track_grad_norm=2,
    )

    if args.train_ds and args.val_ds:
        trainer.fit(model)
    if args.test_ds:
        trainer.test(model)


if __name__ == "__main__":
    cli_main()
