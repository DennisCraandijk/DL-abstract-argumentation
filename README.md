# Deep Learning for Abstract Argumentation Semantics
Code to reproduce the experiments, please email me for questions.

Published at IJCAI 2020: [https://www.ijcai.org/Proceedings/2020/231](https://www.ijcai.org/Proceedings/2020/231)


### Abstract
In this paper, we present a learning-based approach to determining acceptance of arguments under several abstract argumentation semantics. More specifically, we propose an argumentation graph neural network (AGNN) that learns a message-passing algorithm to predict the likelihood of an argument being accepted. The experimental results demonstrate that the AGNN can almost perfectly predict the acceptability under different semantics and scales well for larger argumentation frameworks. Furthermore, analysing the behaviour of the message-passing algorithm shows that the AGNN learns to adhere to basic principles of argument semantics as identified in the literature, and can thus be trained to predict extensions under the different semantics – we show how the latter can be done for multi-extension semantics by using AGNNs to guide a basic search.


# Installation
Create a virtual python 3 environment and install
* [pytorch](https://pytorch.org/) version 1.6 
* [pytorch_geometric](https://github.com/rusty1s/pytorch_geometric)

before installing the requirements
```
pip install -r requirements.txt
```

Compile the Argumentation Framework generators in the `src/data/generators/` directory with `./install.sh`. Compiling with this script requires Ant and Maven


Install pynauty in the `src/data/nauty` directory with `./install.sh`

# Data
To generate Argumentation Frameworks we use [AFBenchGen2](https://sourceforge.net/projects/afbenchgen/), [AFGen](http://argumentationcompetition.org/2019/papers/ICCMA19_paper_3.pdf) and [probo](https://sourceforge.net/projects/probo/) (thanks to the original authors!)

Various datasets can be generated with `generate_graphs.py` and `graphs2AFs.py`
For instance, in order to generate a dataset named 'test-ds' and consisting of 1000 graphs each containing 25 arguments, execute these commands
```
python -m src.data.generate_graphs --name test-25 --min_args 25 --max_args 25 --num 1000
python -m src.data.graphs2AFs --name test-25
```

To see more information about the parameters use the `--help` parameter

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
* `--tag` name this experiment
* `--help` to see more information about other parameters

To track the experiment I use [Weights and Biases](https://wandb.ai/)