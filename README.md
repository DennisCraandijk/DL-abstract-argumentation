# Deep Learning for Abstract Argumentation

**Neuro-symbolic approaches to reasoning problems from abstract argumentation**

This repository holds the code to produce data and experiments for using learning-based approaches to symbolic reasoning
problems of static and dynamic abstract argumentation. The work is described in the following publications:

- Deep Learning for Abstract Argumentation Semantics ([IJCAI 2020](https://www.ijcai.org/Proceedings/2020/231)) ([previous code](https://github.com/DennisCraandijk/DL-abstract-argumentation/tree/ijcai_2020))
- Enforcement Heuristics for Argumentation with Deep Reinforcement Learning ([AAAI 2022]())

Please email me for questions.

## Installation

Create a virtual python 3 environment (tested with 3.8) and install with pip or conda

#### Pip
First install

* [pytorch](https://pytorch.org/) (tested with version 1.11)
* [pytorch_geometric](https://github.com/rusty1s/pytorch_geometric) (tested with version 2.0.2)

seperately with the cuda/cpu settings for your system, then install the other requirements with
```
pip install -r requirements.txt
```

#### Conda
Alternatively use the `conda_environment.yaml` with conda.


### AF generators

Afterwards go to the `src/data/generators/vendor` directory and compile the Argumentation Framework generators
with `./install.sh` (compiling requires Java, Ant and Maven).

### Tests

Optionally test the installation by running the pytest unit tests in `tests/test_components.py`

## Project structure

```
├── data                generated datasets go here
├── models              trained models go here     
├── src
│   ├── data            scripts to generate datasets         
│   ├── experiment      scripts to run experiments
│   ├── models          machine learning models
├── tests               pytest unit tests
```

## Generate Data

To generate Argumentation Frameworks we use [AFBenchGen2](https://sourceforge.net/projects/afbenchgen/)
, [AFGen](http://argumentationcompetition.org/2019/papers/ICCMA19_paper_3.pdf)
and [probo](https://sourceforge.net/projects/probo/) (thanks to the original authors!)

The `src/data` folder contains data generation scripts:

- `generate_apx.py` generates AFs in apx format.
- `apx_to_afs.py` converts apx files to ArgumentationFramework objects and computes extensions and argument acceptance
- `afs_to_enforcement` generates and solves status and extensions enforcement problems for an AF The apx files are

If we want, for instance, to create a dataset for static and dynamic argumentation problems named 'test-25' and
consisting of 1000 graphs each containing 25 arguments, execute:

```
python -m src.data.generate_apxs --name test-25 --min_args 25 --max_args 25 --num 10000
python -m src.data.apxs_to_afs --name test-25
python -m src.data.afs_to_enforcement --name test-25
```

To see more information about the other parameters for each scriptuse the `--help` parameter

## Run experiments

To track the experiment I use [Weights and Biases](https://wandb.ai/). Every experiment is defined as an
[Pytorch Lightning](https://pytorchlightning.ai/) module. Thus you can use all the trainer flags as extra parameters to
control the experimental setting.

### Supervised learning

Experiments for *Deep Learning for Abstract Argumentation Semantics* (IJCAI 2020)

Train and evaluate model with `python -m src.experiment.supervised.sl_experiment` and set the parameters

* `--train_ds` use the name given to the training set
* `--val_ds` use the name given to the validation set
* `--test_ds` use the name given to the test set
* `--task` choose a task from `['cred','scept','enum']`
* `--semantics` choose a semantics from `['com','stb','prf','grd']`
* `--model` choose a model from `['AGNN','GCN','FM2']`
* `--tag` name this experiment
* `--help` to see more information about other parameters

Example

```python -m src.experiment.supervised.sl_experiment --tag test --train_ds train-10 --val_ds val-10 --test_ds test-10 --task enum --semantics grd --model AGNN```

### Reinforcement learning

Experiments for *Enforcement Heuristics for Argumentation with Deep Reinforcement Learning* (AAAI 2022)

Train and evaluate model with `python -m src.experiment.reinforcement.rl_experiment` and set the parameters

* `--train_ds` use the name given to the training dataset
* `--val_ds` use the name given to the validation dataset
* `--test_ds` use the name given to one or more the test dataset(s)
* `--task` choose a task from `['strict','non-strict','cred','scept']`
* `--semantics` choose a semantics from `['com','stb','prf','grd']`
* `--tag` name this experiment
* `--help` to see more information about other parameters

Example

```python -m src.experiment.reinforcement.rl_experiment --tag test --train_ds train-10 --val_ds val-10 --test_ds test-10 --task strict --semantics grd --aggregation multi```
