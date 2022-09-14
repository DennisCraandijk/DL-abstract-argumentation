import argparse
import copy
from typing import List

import numpy as np
import pytorch_lightning as pl
import torch
import torchmetrics
from pytorch_lightning.callbacks import ModelCheckpoint, EarlyStopping
from pytorch_lightning.loggers import WandbLogger
from torch.nn.functional import binary_cross_entropy_with_logits
from torch_geometric.data import Batch
from torch_geometric.loader import DataLoader
from tqdm import tqdm

import src.config as config
from src.constants import GRD, STB, PRF, COM, SCEPT, CRED, ENUM
from src.experiment.supervised.dataset import (
    MemoryDS,
    AcceptanceProblemDS,
    ArgumentationFrameworkDS,
)
from src.experiment.supervised.metrics import f1_precision_recall, get_confusion_matrix
from src.models.GNN import AGNN
from src.models.baselines import GCN, FM2
from src.models.treesearch import search_breadth_batch, batch_solver


class SLExperiment(pl.LightningModule):
    """ Supervised learning experiment """

    def __init__(
            self,
            train_ds: str,
            val_ds: str,
            test_ds: str,
            semantics: str,
            task: str,
            model: str,
            oracle: bool,
            batch_size: int,
            lr: float,
            cyc_policy: bool,
            weight_decay: float,
            hidden_dim: int,
            mp_steps: int,
            tag: str = None,
            num_workers: int = 4,
            **kwargs,
    ):
        super().__init__()

        # Task
        self.semantics = semantics
        self.task = task

        # Dataset
        self.train_ds = train_ds
        self.val_ds = val_ds
        self.test_ds = test_ds

        # Model
        self.representation = model
        if model == "GCN":
            self.model = GCN(hidden_dim, mp_steps)
        elif model == "FM2":
            self.model = FM2(hidden_dim, mp_steps)
        else:
            self.model = AGNN(hidden_dim, mp_steps=mp_steps)

        # Training
        self.oracle = oracle
        self.batch_size = batch_size
        self.lr = lr
        self.cyc_policy = cyc_policy
        self.weight_decay = weight_decay
        self.num_workers = num_workers

        # Checkpointing
        if tag:
            self.model_dir = config.models_dir / "SL" / model / tag
            self.model_name = f"{self.semantics}_{self.task}"

        # Metrics
        self.best_val_mcc = None
        self.val_mcc = torchmetrics.classification.MatthewsCorrcoef(2)
        self.test_mcc = torchmetrics.classification.MatthewsCorrcoef(2)
        self.test_mae = torchmetrics.MeanAbsoluteError()

    def resume(self, name: str):
        """
        Load pretrained model
        Args:
            name: name or tag used in previous run
        """
        path = self.model_dir / f"{name}.ckpt"
        print(f"Resume {path}")
        assert path.exists()
        map_location = None if self.on_gpu else torch.device("cpu")
        state_dict = torch.load(path, map_location)["state_dict"]
        self.load_state_dict(state_dict)

    def train_dataloader(self) -> DataLoader:
        """ Return PyTorch dataloader for training"""
        dataset = AcceptanceProblemDS(
            self.train_ds, self.task, self.semantics, self.representation
        )
        memory_ds = MemoryDS(dataset, self.semantics, self.task, self.representation)
        if len(memory_ds) % self.batch_size != 0:
            raise Warning(
                "Dataset size is not devidable by batch size. This may cause issues with cyclical learning rate not syncing with the start of an epoch")

        loader = DataLoader(
            memory_ds,
            self.batch_size,
            shuffle=True,
            num_workers=self.num_workers,
            pin_memory=True,
        )
        return loader

    def eval_dataloader(self, ds_name) -> DataLoader:
        """ Return PyTorch dataloader for evaluation"""
        dataset = AcceptanceProblemDS(
            ds_name, self.task, self.semantics, self.representation
        )
        memory_ds = MemoryDS(dataset, self.semantics, self.task, self.representation)
        batch_size = self.batch_size * 4
        loader = DataLoader(
            memory_ds, batch_size, num_workers=self.num_workers, pin_memory=True
        )
        return loader

    def val_dataloader(self) -> DataLoader:
        return self.eval_dataloader(self.val_ds)

    def test_dataloader(self) -> DataLoader:
        return self.eval_dataloader(self.test_ds)

    def training_step(self, batch: Batch, *args, **kwargs):
        """
        Compute loss and log metrics
        """
        loss, _, _ = self.get_loss(batch)

        lr = self.trainer.optimizers[0].param_groups[0]["lr"]

        self.log("training/loss", loss)
        self.log("training/lr", lr)

        return loss

    def validation_step(self, batch: Batch, *args, **kwargs):
        """
        Operate on a single batch of data from the validation set
        """
        loss, preds, targets = self.get_loss(batch, all_steps=False)
        self.val_mcc(preds, targets.int())
        return loss

    def validation_epoch_end(self, outputs: List[torch.Tensor]):
        """
        Aggregate and log validation metrics
        """
        avg_loss = sum(outputs) / len(outputs)
        val_mcc = self.val_mcc.compute()
        val_mcc[torch.isnan(val_mcc)] = 0
        self.val_mcc.reset()

        self.best_val_mcc = (
            val_mcc
            if (self.best_val_mcc is None or val_mcc > self.best_val_mcc)
            else self.best_val_mcc
        )

        self.log("validation/mcc", val_mcc)
        self.log("validation/best_mcc", self.best_val_mcc)
        self.log("validation/loss", avg_loss)

    def test_step(self, batch: Batch, *args, **kwargs):
        """
        Operate on a single batch of data from the test set
        """
        loss, preds, targets = self.get_loss(batch, all_steps=False)
        self.test_mcc(preds, targets.int())
        self.test_mae(preds, targets)
        return loss

    def test_epoch_end(self, outputs: List[torch.Tensor]):
        """
        Aggregate and log test metrics
        """
        avg_loss = sum(outputs) / len(outputs)
        mcc = self.test_mcc.compute()
        mae = self.test_mae.compute()
        self.test_mcc.reset()
        self.test_mae.reset()

        self.log("test/loss", avg_loss)
        self.log("test/mcc", mcc)
        self.log("test/mae", mae)

        if self.task == ENUM:
            f1, precision, recall = self.test_enumeration()
            self.log("test/enum_f1", f1)
            self.log("test/enum_p", precision)
            self.log("test/enum_r", recall)

    def configure_optimizers(self):
        """ Choose what optimizers and learning-rate schedulers to use in your optimization. """
        optimizer = torch.optim.AdamW(
            self.model.parameters(), self.lr, weight_decay=self.weight_decay
        )
        optimizers = [optimizer]
        schedulers = []

        if self.cyc_policy:
            step_size_up = int(
                len(self.train_dataloader().dataset) / self.batch_size / 2
            )
            schedulers = [
                {
                    "interval": "step",
                    "frequency": 1,
                    "scheduler": torch.optim.lr_scheduler.CyclicLR(
                        optimizer,
                        self.lr / 1e4,
                        max_lr=self.lr,
                        step_size_up=step_size_up,
                        cycle_momentum=False,
                    ),
                }
            ]
        return optimizers, schedulers

    def test_enumeration(self):
        """
        Test enumeration by guiding a tree search on the test dataset
        """

        dataset = ArgumentationFrameworkDS(self.test_ds, representation=self.representation)
        cm = np.array([[0, 0], [0, 0]])
        with torch.no_grad():
            for af, data in tqdm(dataset, desc="Enumerate"):
                data.to(self.device)
                predictions = self.test_enumeration_step(af, data)

                cm += get_confusion_matrix(predictions, af.extensions[self.semantics])

                if self.trainer.fast_dev_run:
                    break
        f1, precision, recall = f1_precision_recall(cm)
        return f1, precision, recall

    def test_enumeration_step(self, af, data):
        """
        Test enumeration by guiding a tree search on the test dataset
        """
        predictions, search_log = search_breadth_batch(
            solver_batch=batch_solver,
            data=data,
            model=self.model,
            safeguard=True,
            oracle=self.oracle,
            AF=af,
        )
        if (
                self.semantics == STB
                and len(predictions) == 1
                and len(next(iter(predictions))) == 0
        ):
            predictions = set()
        return predictions

    def get_loss(self, batch: Batch, all_steps: bool = True):
        """
        Expand target to each message passing step and get BCE loss over each message passing step
        Args:
            batch:
            all_steps: consider all message passing steps or only the final step
        """
        logits = self.model(batch, output_all_steps=all_steps)
        targets = batch["node_y"].unsqueeze(0).expand_as(logits)
        loss = binary_cross_entropy_with_logits(logits, targets)
        predictions = logits.sigmoid()
        return loss, predictions, targets

    @staticmethod
    def add_model_specific_args(
            arg_parser: argparse.ArgumentParser,
    ) -> argparse.ArgumentParser:

        # parser = argparse.ArgumentParser(parents=[parent_parser], add_help=False)
        arg_parser.add_argument("--train_ds", type=str, help="Training dataset name")
        arg_parser.add_argument("--val_ds", type=str, help="Validation dataset name")
        arg_parser.add_argument("--test_ds", type=str, help="Test dataset name")

        arg_parser.add_argument(
            "--semantics",
            choices=[GRD, PRF, STB, COM],
            default=PRF,
            help="Semantics",
        )
        arg_parser.add_argument(
            "--task", choices=[SCEPT, CRED, ENUM], default=CRED, help="Task"
        )
        arg_parser.add_argument(
            "--model", choices=["AGNN", "GCN", "FM2"], default="AGNN"
        )
        arg_parser.add_argument(
            "--oracle",
            type=bool,
            default=False,
            help="use CO oracle",
        )

        arg_parser.add_argument("--resume", type=bool, default=False)
        arg_parser.add_argument(
            "--patience",
            type=int,
            default=None,
            help="Early stop after n epochs patience",
        )
        arg_parser.add_argument("--tag", type=str, default="debug")

        arg_parser.add_argument("--batch_size", type=int, default=50, help="Batch size")
        arg_parser.add_argument("--lr", type=float, default=1e-4)
        arg_parser.add_argument("--cyc_policy", type=bool, default=True)
        arg_parser.add_argument("--weight_decay", type=float, default=10e-10)

        arg_parser.add_argument("--hidden_dim", type=int, default=128)
        arg_parser.add_argument("--mp_steps", type=int, default=32)

        # pytorch lightning defaults
        arg_parser.set_defaults(gpus=1)
        arg_parser.set_defaults(gradient_clip_val=0.5)
        return arg_parser


def cli_main():
    parser = argparse.ArgumentParser()

    # trainer args
    parser = pl.Trainer.add_argparse_args(parser)

    # model args
    parser = SLExperiment.add_model_specific_args(parser)
    args = parser.parse_args()

    model = SLExperiment(**args.__dict__)

    if args.resume:
        model.resume(args.resume)

    logger = None
    callbacks = []
    if not args.fast_dev_run:
        logger = WandbLogger(
            project="argumentation", save_dir=str(config.root_dir), tags=[args.tag]
        )
        logger.log_hyperparams(args)

        # save checkpoints based on avg_reward
        checkpoint_callback = ModelCheckpoint(
            dirpath=logger.experiment.dir,
            save_top_k=1,
            monitor="validation/loss",
            mode="min",
            save_weights_only=True,
            verbose=True,
        )
        callbacks.append(checkpoint_callback)

        if args.tag:
            tag_checkpoint_callback = copy.deepcopy(checkpoint_callback)
            tag_checkpoint_callback.dirpath = model.model_dir
            tag_checkpoint_callback.filename = model.model_name
            callbacks.append(tag_checkpoint_callback)

    # early stopping
    if args.patience:
        early_stop_callback = EarlyStopping(
            monitor="validation/loss", patience=args.patience, mode="min", verbose=True
        )
        callbacks.append(early_stop_callback)

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
