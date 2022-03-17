from __future__ import annotations

from typing import Optional, TYPE_CHECKING

import networkx as nx
from networkx import DiGraph

from src.constants import MUST_OUT, BLANK, OPP, OUT, UNDEC, PRO

if TYPE_CHECKING:
    from src.data.classes.argumentation_framework import ArgumentationFramework


class AcceptanceVerifier:
    """
    A solver wiht algorithms for verifying acceptance status of arguments
    (without computing all extensions)
    """

    @staticmethod
    def is_conflict_free(af: ArgumentationFramework, arg_set: frozenset) -> bool:
        """
        Verify whether arg_set is a conflict free set
        """
        for arg in arg_set:
            for attacked_arg in af.graph.successors(arg):
                if attacked_arg in arg_set:
                    return False
        return True

    @staticmethod
    def is_stable(af: ArgumentationFramework, arg_set: frozenset) -> bool:
        """
        Verify whether arg_set is a stable extension by
        taking the set of arguments which are not attacked by arg_set
        and then testing if this set is equal to arg_set
        Besnard & Doutre (2004) Checking the acceptability of a set of arguments.
        """
        # "the set of arguments which are not attacked by S and then testing if this set is equal to S"
        not_attacked_by_arg_set = af.arguments - af.attacked_by(arg_set)
        return arg_set == frozenset(not_attacked_by_arg_set)

    @staticmethod
    def is_complete(af: ArgumentationFramework, arg_set: frozenset) -> bool:
        """
        Verify whether the arg_set is a complete extension by
        Compute the set of arguments defended by arg_set,
        the set of arguments not attacked by S and then
        to test if their intersection is equal to arg_set.
        Besnard & Doutre (2004) Checking the acceptability of a set of arguments.
        """
        attacked_by_arg_set = af.attacked_by(arg_set)
        defended_by_arg_set = set()
        for arg in af.arguments:
            attackers = set(af.graph.predecessors(arg))
            if attackers.issubset(attacked_by_arg_set):
                defended_by_arg_set.add(arg)

        not_attacked_by_arg_set = af.arguments - attacked_by_arg_set
        intersection = defended_by_arg_set.intersection(not_attacked_by_arg_set)
        return arg_set == frozenset(intersection)

    @staticmethod
    def is_in_admissible(af: ArgumentationFramework, arg_x: int) -> bool:
        """
        Verify whether an argument x is in an admissable set.
        Implementation of algorithm 3 described in
        Nofal et al (2014) Algorithms for decision problems in argument systems under preferred semantics
        """

        # Pseudo code {x}+ is attacked by x. graph.successors
        # . {x}- is attacking x. graph.predecessors

        def is_accepted(graph: DiGraph) -> bool:
            """ Check if current labelling is accepted """
            # line 14
            for arg_y in graph.nodes:
                # change argumentsʼ labels iteratively until
                # there is no argument remaining that is labelled MUST_OUT
                if graph.nodes[arg_y]["label"] != MUST_OUT:
                    continue

                # line 15. Updated at end of while loop
                blank_predecessors = [
                    arg_z
                    for arg_z in graph.predecessors(arg_y)
                    if graph.nodes[arg_z]["label"] == BLANK
                ]
                while blank_predecessors:
                    # line 16
                    for arg_z in blank_predecessors:
                        if all(
                                graph.nodes[w]["label"] in [OPP, OUT, MUST_OUT]
                                for w in graph.predecessors(arg_z)
                        ):
                            break
                    else:
                        for arg_z in blank_predecessors:
                            if all(
                                    graph.nodes[w]["label"] == BLANK
                                    and graph.out_degree(arg_z) >= graph.out_degree(w)
                                    for w in graph.predecessors(arg_y)
                            ):
                                break

                    # line 17-29 in a function
                    graph2 = label_pro(graph, arg_z)

                    if not graph2:
                        # line 27. Finish this while loop
                        graph.nodes[arg_z]["label"] = UNDEC
                    else:
                        if is_accepted(graph2):
                            return True

                        graph.nodes[arg_z]["label"] = UNDEC

                    # update for line 15
                    blank_predecessors = [
                        arg_z
                        for arg_z in graph.predecessors(arg_y)
                        if graph.nodes[arg_z]["label"] == BLANK
                    ]
                return False
            return True

        def label_pro(graph: DiGraph, arg_z: int) -> Optional[DiGraph]:
            """ Label arg_z PRO and propagate labels correspondingly """
            graph2 = graph.copy()
            graph2.nodes[arg_z]["label"] = PRO
            # line 18-22
            for u in graph2.successors(arg_z):
                if graph2.nodes[u]["label"] == MUST_OUT:
                    graph2.nodes[u]["label"] = OPP
                elif graph2.nodes[u]["label"] != OPP:
                    graph2.nodes[u]["label"] = OUT
            # line 23-29
            for v in graph2.predecessors(arg_z):
                if graph2.nodes[v]["label"] in [BLANK, UNDEC]:
                    graph2.nodes[v]["label"] = MUST_OUT
                    if all(
                            graph2.nodes[w]["label"] != BLANK
                            for w in graph2.predecessors(v)
                    ):
                        # line 27 implemented as return. Set z to undecided is done outside function
                        return None
                elif graph2.nodes[v]["label"] == OUT:
                    graph2.nodes[v]["label"] = OPP
            return graph2

        graph = af.graph.copy()
        nx.set_node_attributes(graph, BLANK, "label")
        graph.nodes[arg_x]["label"] = PRO

        # Line 3
        # An argument y is labelled OUT if and only if y is attacked by a PRO argument.
        for arg_y in graph.successors(arg_x):
            graph.nodes[arg_y]["label"] = OUT

        # Line 4-10
        for arg_z in graph.predecessors(arg_x):
            if graph.nodes[arg_z]["label"] == BLANK:
                # The MUST_OUT label identifies arguments that attack PRO arguments.
                graph.nodes[arg_z]["label"] = MUST_OUT
                if not any(
                        graph.nodes[w]["label"] == BLANK for w in graph.predecessors(arg_z)
                ):
                    # If no argument can possibly defeat a MUST_OUT argument, arg is not accepted
                    return False
            elif graph.nodes[arg_z]["label"] == OUT:
                # An argument y is labelled OPP if and only if
                # y is attacked by a PRO argument and y attacks a PRO argument
                graph.nodes[arg_z]["label"] = OPP

        return is_accepted(graph)
