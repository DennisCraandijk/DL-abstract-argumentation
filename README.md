# Deep Learning for Abstract Argumentation Semantics
Code to reproduce the experiments, please email me for questions.

Published at IJCAI 2020: [https://www.ijcai.org/Proceedings/2020/231](https://www.ijcai.org/Proceedings/2020/231)


### Abstract
In this paper, we present a learning-based approach to determining acceptance of arguments under several abstract argumentation semantics. More specifically, we propose an argumentation graph neural network (AGNN) that learns a message-passing algorithm to predict the likelihood of an argument being accepted. The experimental results demonstrate that the AGNN can almost perfectly predict the acceptability under different semantics and scales well for larger argumentation frameworks. Furthermore, analysing the behaviour of the message-passing algorithm shows that the AGNN learns to adhere to basic principles of argument semantics as identified in the literature, and can thus be trained to predict extensions under the different semantics – we show how the latter can be done for multi-extension semantics by using AGNNs to guide a basic search.


# Installation
Create a virtual python 3 environment and install
* [pytorch](https://pytorch.org/) version 1.6 
* [pytorch_geometric](https://github.com/rusty1s/pytorch_geometric)

seperately before installing the `requirements.txt`

Go to the `src/data/generators/` directory to compile the Argumentation Framework generators with `./install.sh` (this requires Java, Ant and Maven)

Got to the `src/data/nauty` directory to install [pynauty](https://web.cs.dal.ca/~peter/software/pynauty/html/index.html) with `./install.sh`

# Data
To generate Argumentation Frameworks we use the [AFBenchGen2](https://sourceforge.net/projects/afbenchgen/), [AFGen](http://argumentationcompetition.org/2019/papers/ICCMA19_paper_3.pdf) and [probo](https://sourceforge.net/projects/probo/) generators (thanks to the original authors!)

A dataset can be generated with `generate_graphs.py` (which generates .apx files) and `graphs2AFs.py` (which converts the .apx files to ArgumentationFramework objects with enumerated extensions). For instance, in order to generate a dataset named `test-ds` consisting of 1000 graphs each containing 25 arguments, execute these commands
```
python -m src.data.generate_graphs --name test-25 --min_args 25 --max_args 25 --num 1000
python -m src.data.graphs2AFs --name test-25
```
The generated files are stored in `data/dataset/[name]`. For more information about the parameters of these scripts use the `--help` parameter

# Experiment

Train and evaluate model

```
python -m src.sl.experiment --train_ds train_ds --val_ds val-ds --test test-ds
```

with the parameters
* `--train_ds` use the name given to the training set
* `--val_ds` use the name given to the validation set
* `--test_ds` use the name given to the test set
* `--task` choose a task from `['cred','scept','enum']`
* `--semantics` choose a semantics from `['CO','ST','PR','GR']`
* `--model` choose a GNN model from `['AGNN','FM2','GCN']
* `--tag` name this experiment
* `--help` to see more information about other parameters

The trained model checkpoints are saved in `models/sl/[model]/[tag]/`

To track the experiment I use [Weights and Biases](https://wandb.ai/)

# Cite

```
@inproceedings{ijcai2020-231,
  title     = {Deep Learning for Abstract Argumentation Semantics},
  author    = {Craandijk, Dennis and Bex, Floris},
  booktitle = {Proceedings of the Twenty-Ninth International Joint Conference on
               Artificial Intelligence, {IJCAI-20}},
  publisher = {International Joint Conferences on Artificial Intelligence Organization},             
  editor    = {Christian Bessiere},	
  pages     = {1667--1673},
  year      = {2020},
  month     = {7},
  note      = {Main track}
  doi       = {10.24963/ijcai.2020/231},
  url       = {https://doi.org/10.24963/ijcai.2020/231},
}
```
