from typing import Tuple

import numpy as np
from numpy import ndarray


def f1_precision_recall(cm: ndarray) -> Tuple[float, float, float]:
    """ Return metrics from confusion matrix"""
    tp, fp, fn = cm[0, 0], cm[0, 1], cm[1, 0]
    precision = tp / (tp + fp)
    recall = tp / (tp + fn)
    f1 = 2 * (precision * recall) / (precision + recall)
    return f1, precision, recall


def get_confusion_matrix(predictions: frozenset, targets: frozenset) -> ndarray:
    """ Return confusion matrix from metrics"""
    tp = len(set.intersection(predictions, targets))  # overlap
    fp = len(predictions) - tp
    fn = len(targets) - tp
    tn = int(len(predictions) == 0 and len(targets) == 0)

    cm = np.array([[tp, fp], [fn, tn]])

    return cm
