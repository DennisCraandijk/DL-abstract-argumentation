import argparse
import copy

import numpy as np
import pytorch_lightning as pl
import torch
from pytorch_lightning.callbacks import ModelCheckpoint, EarlyStopping
from pytorch_lightning.loggers import WandbLogger
from torch.nn.functional import binary_cross_entropy
from torch_geometric.data import Batch
from torch_geometric.data.dataloader import DataLoader
from tqdm import tqdm

import src.config as config
from src.models.AGNN import AGNN
from src.models.FM2 import FM2
from src.models.GCN import GCN
from src.models.treesearch import search_breadth_batch, batch_solver
from src.sl.dataset import (
    MemoryDS,
    AcceptanceProblemDS,
    ArgumentationFrameworkDS,
)


class SLExperiment(pl.LightningModule):
    def __init__(
        self,
        train_ds,
        val_ds,
        test_ds,
        semantics,
        task,
        model,
        oracle,
        batch_size,
        lr,
        cyc_policy,
        wd,
        hidden_dim,
        mp_steps,
        tag=None,
        **kwargs,
    ):
        super().__init__()

        self.train_ds = train_ds
        self.val_ds = val_ds
        self.test_ds = test_ds
        self.semantics = semantics
        self.task = task
        self.representation = model
        if model is "GCN":
            self.model = GCN()
        elif model is "FM2":
            self.model = FM2()
        else:
            self.model = AGNN(hidden_dim, mp_steps=mp_steps)

        self.oracle = oracle
        self.batch_size = batch_size
        self.lr = lr
        self.cyc_policy = cyc_policy
        self.wd = wd
        self.hidden_dim = hidden_dim
        self.mp_steps = mp_steps

        if tag:
            self.model_dir = config.models_dir / "SL" / model / tag
            self.model_name = f"{self.semantics}_{self.task}"

        self.num_workers = 4
        self.best_val_mcc = None

        self.validation_confmat = pl.metrics.ConfusionMatrix(2)
        self.test_confmat = pl.metrics.ConfusionMatrix(2)
        self.test_acc_reject = pl.metrics.Accuracy()
        self.test_acc_accept = pl.metrics.Accuracy()
        self.test_mae = pl.metrics.MeanAbsoluteError()

    def forward(self, *args, **kwargs):
        pass

    def resume(self, name: str):
        """
        Load pretrained model
        """
        path = self.model_dir / f"{self.model_name}.ckpt"
        print(f"Resume {path}")
        assert path.exists()
        state_dict = torch.load(path)["state_dict"]
        self.load_state_dict(state_dict)

    def train_dataloader(self) -> DataLoader:
        """
        PyTorch dataloader for training
        """
        ds = AcceptanceProblemDS(
            self.train_ds, self.task, self.semantics, self.representation
        )
        memory_ds = MemoryDS(ds, self.semantics, self.task, self.representation)
        assert len(ds) % self.batch_size == 0

        loader = DataLoader(
            memory_ds,
            self.batch_size,
            shuffle=True,
            num_workers=self.num_workers,
        )
        return loader

    def eval_dataloader(self, ds_name) -> DataLoader:
        """
        PyTorch dataloader for evaluation
        """
        ds = AcceptanceProblemDS(
            ds_name, self.task, self.semantics, self.representation
        )
        memory_ds = MemoryDS(ds, self.semantics, self.task, self.representation)
        batch_size = self.batch_size * 4
        loader = DataLoader(memory_ds, batch_size, num_workers=self.num_workers)
        return loader

    def val_dataloader(self) -> DataLoader:
        return self.eval_dataloader(self.val_ds)

    def test_dataloader(self) -> DataLoader:
        return self.eval_dataloader(self.test_ds)

    def training_step(self, batch: Batch, *args):
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
        loss, preds, targets = self.get_loss(batch)
        self.validation_confmat(preds, targets)
        return loss

    def validation_epoch_end(self, losses):
        """
        Aggregate and log validation metrics
        """
        avg_loss = sum(losses) / len(losses)
        confmat = self.validation_confmat.compute().cpu()
        val_mcc = self.matthews_corrcoef(confmat)

        self.best_val_mcc = (
            val_mcc
            if (self.best_val_mcc is None or val_mcc > self.best_val_mcc)
            else self.best_val_mcc
        )

        self.log("validation/mcc", val_mcc)
        self.log("validation/best_mcc", self.best_val_mcc)
        self.log("validation/loss", avg_loss)

    def test_step(self, batch, *args, **kwargs):
        """
        Operate on a single batch of data from the test set
        """
        loss, preds, targets = self.get_loss(batch)
        self.test_confmat(preds, targets)
        self.test_mae(preds, targets)
        return loss

    def test_epoch_end(self, losses):
        """
        Aggregate and log test metrics
        """
        avg_loss = sum(losses) / len(losses)
        confmat = self.test_confmat.compute().cpu()
        mcc = self.matthews_corrcoef(confmat)
        class_accuracy = self.class_accuracy(confmat)
        mae = self.test_mae.compute().cpu()

        self.log("test/loss", avg_loss)
        self.log("test/mcc", mcc)
        self.log("test/acc_reject", class_accuracy[0])
        self.log("test/acc_accept", class_accuracy[1])
        self.log("test/mae", mae)

        if self.task == "enum":
            f1, p, r = self.test_enumeration()
            self.log("test/enum_f1", f1)
            self.log("test/enum_p", p)
            self.log("test/enum_r", r)

    def configure_optimizers(self):
        optimizer = torch.optim.AdamW(
            self.model.parameters(), lr=self.lr, weight_decay=self.wd
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
        '''
        Test enumeration by guiding a tree search on the test dataset
        '''

        ds = ArgumentationFrameworkDS(self.test_ds)
        cm = np.array([[0, 0], [0, 0]])
        with torch.no_grad():
            for AF, data in tqdm(ds, desc="Enumerate"):
                data.to(self.device)
                predictions = self.test_enumeration_step(AF, data)

                cm += get_confusion_matrix(predictions, AF.extensions[self.semantics])

                if self.trainer.fast_dev_run:
                    break
        f1, p, r = f1_precision_recall(cm)
        return f1, p, r

    def test_enumeration_step(self, AF, data):
        '''
        Test enumeration by guiding a tree search on the test dataset
        '''
        predictions, search_log = search_breadth_batch(
            solver_batch=batch_solver,
            data=data,
            model=self.model,
            safeguard=True,
            oracle=self.oracle,
            AF=AF,
        )
        if (
                self.semantics == "ST"
                and len(predictions) == 1
                and len(next(iter(predictions))) == 0
        ):
            predictions = set()
        return predictions

    def get_loss(self, batch: Batch):
        """
        Expand target to each message passing step and get BCE loss over each message passing step
        """
        predictions = self.model(batch).sigmoid()
        targets = batch["node_y"].unsqueeze(0).expand(*predictions.shape)
        loss = binary_cross_entropy(predictions, targets)
        return loss, predictions, targets

    @staticmethod
    def class_accuracy(cm):
        return cm.diagonal() / cm.sum(axis=1)

    @staticmethod
    def matthews_corrcoef(C):
        # from https://scikit-learn.org/stable/modules/generated/sklearn.metrics.matthews_corrcoef.html source code
        t_sum = C.sum(axis=1)
        p_sum = C.sum(axis=0)
        n_correct = np.trace(C)
        n_samples = p_sum.sum()
        cov_ytyp = n_correct * n_samples - np.dot(t_sum, p_sum)
        cov_ypyp = n_samples ** 2 - np.dot(p_sum, p_sum)
        cov_ytyt = n_samples ** 2 - np.dot(t_sum, t_sum)
        mcc = cov_ytyp / np.sqrt(cov_ytyt * cov_ypyp)

        if np.isnan(mcc):
            mcc = 0

        return mcc

    @staticmethod
    def add_model_specific_args(
        arg_parser: argparse.ArgumentParser,
    ) -> argparse.ArgumentParser:

        # parser = argparse.ArgumentParser(parents=[parent_parser], add_help=False)
        arg_parser.add_argument(
            "--train_ds", type=str, help="Training dataset name"
        )
        arg_parser.add_argument(
            "--val_ds", type=str, help="Validation dataset name"
        )
        arg_parser.add_argument(
            "--test_ds", type=str, help="Test dataset name"
        )

        arg_parser.add_argument(
            "--semantics",
            choices=["GR", "PR", "ST", "CO"],
            default="PR",
            help="Semantics",
        )
        arg_parser.add_argument(
            "--task", choices=["scept", "cred", "enum"], default="cred", help="Task"
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

        arg_parser.add_argument("--resume", type=str, default=None)
        arg_parser.add_argument(
            "--patience",
            type=int,
            default=None,
            help="Early stop after n epochs patience",
        )
        arg_parser.add_argument("--tag", type=str, default="debug")

        arg_parser.add_argument("--batch_size", type=int, default=50, help="Batch size")
        arg_parser.add_argument("--lr", type=float, default=2e-4)
        arg_parser.add_argument("--cyc_policy", type=bool, default=True)
        arg_parser.add_argument("--wd", type=float, default=10e-10)

        arg_parser.add_argument("--hidden_dim", type=int, default=128)
        arg_parser.add_argument("--mp_steps", type=int, default=32)

        # pytorch lightning defaults
        arg_parser.set_defaults(gpus=1)
        arg_parser.set_defaults(gradient_clip_val=0.5)
        return arg_parser


def cli_main():

    parser = argparse.ArgumentParser(add_help=False)

    # trainer args
    parser = pl.Trainer.add_argparse_args(parser)

    # model args
    parser = SLExperiment.add_model_specific_args(parser)
    args = parser.parse_args()

    model = SLExperiment(**args.__dict__)

    if args.resume:
        model.resume(args.resume)

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
        save_last=True,
    )
    callbacks = [checkpoint_callback]

    if args.tag:
        tag_checkpoint_callback = copy.deepcopy(checkpoint_callback)
        tag_checkpoint_callback.dirpath = model.model_dir
        tag_checkpoint_callback.filename = model.model_name
        callbacks.append(tag_checkpoint_callback)

    # early stopping
    if args.patience:
        early_stop_callback = EarlyStopping(
            monitor="validation/loss", patience=args.patience, mode="min"
        )
        callbacks.append(early_stop_callback)

    pl.seed_everything(123)
    trainer = pl.Trainer.from_argparse_args(
        args,
        deterministic=True,
        logger=logger,
        callbacks=callbacks,
        track_grad_norm=2,
    )

    if args.train_ds and args.val_ds:
        trainer.fit(model)
    if args.test_ds:
        trainer.test(model)


def f1_precision_recall(cm):
    tp, fp, fn = cm[0, 0], cm[0, 1], cm[1, 0]
    p = tp / (tp + fp)
    r = tp / (tp + fn)
    f1 = 2 * (p * r) / (p + r)
    return f1, p, r


def get_confusion_matrix(predictions, targets):
    tp = len(set.intersection(predictions, targets))  # overlap
    fp = len(predictions) - tp
    fn = len(targets) - tp
    tn = int(len(predictions) == 0 and len(targets) == 0)

    cm = np.array([[tp, fp], [fn, tn]])

    return cm


if __name__ == "__main__":
    cli_main()
