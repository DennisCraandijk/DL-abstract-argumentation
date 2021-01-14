import copy
from typing import Callable

import torch
from torch_geometric.data.batch import Batch

from src.data.classes.ArgumentationFramework import ArgumentationFramework


def input2set(input):
    if type(input) != list:
        input = [input]
    return set(input)


def set_features(data, features):
    # fill data with new feature
    for node in range(len(data.node_embedding)):
        data.node_embedding[node] = 0
        if node in features:
            data.node_embedding[node] = 1

    return data



def not_in_log(log, output, level):
    for i in range(level, len(log)):
        if output in log[i]:
            return False

    return True


def search_breadth_batch(solver_batch: Callable, AF: ArgumentationFramework, input=set(), output=None, level=0,
                         safeguard=True, oracle=False, **kwargs):
    global search_log, output_log, input_log, results

    # keep logs of seen features and outputs
    if level == 0:
        search_log, output_log, input_log = [], {}, {}
        results = set()
    output_log.setdefault(level, []), input_log.setdefault(level, [])

    # if output has not been provided by a parent (only in root)
    if output is None:
        output = solver_batch(input_list=[input], **kwargs)[0]
        search_log.append((input, output))

    # if input equals output, we are in a leaf and return results
    if input == output:
        results.add(frozenset(output))
        return results, search_log
    # TODO oracle
    if oracle and frozenset(input) in AF.extensions['CO']:
        results.add(frozenset(input))

    additions = output - input

    input_list = []
    for addition in additions:
        child_input = input | {addition}

        # skip this child if already seen
        if child_input in input_log[level]:
            continue

        # queue to get output of this child
        input_log[level].append(child_input)
        input_list.append(child_input.copy())

    if len(input_list) == 0:
        return results, search_log

    output_list = solver_batch(input_list=input_list, **kwargs)

    children_input, children_output = [], []
    for i in range(len(output_list)):
        child_input, child_output = input_list[i], output_list[i]
        if safeguard and not child_input.issubset(child_output): continue
        search_log.append((child_input, child_output))

        # mark for further exploration if output is unique
        if child_output not in children_output and not_in_log(output_log, child_output, level):
            children_input.append(child_input)
            children_output.append(child_output)
            output_log[level].append(child_output)

    # explore marked children nodes
    for i in range(len(children_output)):
        search_breadth_batch(solver_batch, input=children_input[i], output=children_output[i],
                             level=(level + 1), safeguard=safeguard, oracle=oracle, AF=AF, **kwargs)

    return results, search_log


# TODO incorporate in search_breadth_batch?
def batch_solver(data, input_list, model):
    # data = data.to_data_list()[0]
    data_list, predictions = [], []
    for input in input_list:
        data_list.append(copy.deepcopy(set_features(data, input)))
    batch = Batch.from_data_list(data_list)
    outputs = model(batch)
    sections = torch.bincount(batch.batch).tolist()
    node_outputs = outputs[-1,:].split(sections)

    for i, node_output in enumerate(node_outputs):
        node_predictions = (node_output.sigmoid() > 0.5).byte()
        yes = input2set((node_predictions == 1).nonzero().squeeze().tolist())
        predictions.append(yes)
    return predictions
