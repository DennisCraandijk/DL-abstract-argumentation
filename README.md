# Deep Learning for Abstract Argumentation Semantics
Code to reproduce the experiments, please email me for questions.

Published at IJCAI 2020: [https://www.ijcai.org/Proceedings/2020/231](https://www.ijcai.org/Proceedings/2020/231)


## Abstract
In this paper, we present a learning-based approach to determining acceptance of arguments under several abstract argumentation semantics. More specifically, we propose an argumentation graph neural network (AGNN) that learns a message-passing algorithm to predict the likelihood of an argument being accepted. The experimental results demonstrate that the AGNN can almost perfectly predict the acceptability under different semantics and scales well for larger argumentation frameworks. Furthermore, analysing the behaviour of the message-passing algorithm shows that the AGNN learns to adhere to basic principles of argument semantics as identified in the literature, and can thus be trained to predict extensions under the different semantics – we show how the latter can be done for multi-extension semantics by using AGNNs to guide a basic search.

# AGNN

## Installation
Create a virtual python environment and manually install
* [pytorch](https://pytorch.org/)
* [pytorch_geometric](https://github.com/rusty1s/pytorch_geometric)

before installing the requirements.txt

Compile the java generators in src/data/generators/

Install pynauty in the src/data/nauty directory with `./install.sh`

## Data
Example: in order to generate a dataset named 'test-25' and consisting of 1000 graphs each containing 25 arguments, execute these commands
```
python -m src.data.generate_graphs --name test-25 --min_args 25 --max_args 25 --num 10000
python -m src.data.graphs2AFs --name test-25
```
