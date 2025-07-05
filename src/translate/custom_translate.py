#! /usr/bin/env python3


import os
import sys
import traceback
from typing import Dict, List, Optional, Tuple, Union

VarValPair = Tuple[int, int]

def python_version_supported():
    return sys.version_info >= (3, 6)

if not python_version_supported():
    sys.exit("Error: Translator only supports Python >= 3.6.")


from collections import defaultdict
from copy import deepcopy
from itertools import product


import fact_groups
import instantiate
import normalize
import options
import pddl
import pddl_parser
import sas_tasks
import signal
import simplify
import timers
import tools
import variable_order
import translate


def main():
    timer = timers.Timer()
    with timers.timing("Parsing", True):
        task = pddl_parser.open(
            domain_filename=options.domain, task_filename=options.task)

    with timers.timing("Normalizing task"):
        normalize.normalize(task)

    if options.generate_relaxed_task:
        # Remove delete effects.
        for action in task.actions:
            for index, effect in reversed(list(enumerate(action.effects))):
                if effect.literal.negated:
                    del action.effects[index]

    sas_task = translate.pddl_to_sas(task)
    goal_prev = []
    for var, val in sas_task.goal.pairs:
        goal_prev.append((var, val))
    goal_action = sas_tasks.SASOperator('(goal_action)', goal_prev, [], 1)

    init_effects = []
    for i, value in enumerate(sas_task.init.values):
        init_effects.append((i, -1, value, []))
    init_action = sas_tasks.SASOperator('(init_action)', [], init_effects, 1)
    sas_task.operators.append(init_action)
    sas_task.operators.append(goal_action)

    translate.dump_statistics(sas_task)

    with timers.timing("Writing output"):
        with open(options.sas_file, "w") as output_file:
            sas_task.output(output_file)
    print("Done! %s" % timer)


def handle_sigxcpu(signum, stackframe):
    print()
    print("Translator hit the time limit")
    # sys.exit() is not safe to be called from within signal handlers, but
    # os._exit() is.
    os._exit(TRANSLATE_OUT_OF_TIME)


if __name__ == "__main__":
    try:
        signal.signal(signal.SIGXCPU, handle_sigxcpu)
    except AttributeError:
        print("Warning! SIGXCPU is not available on your platform. "
              "This means that the planner cannot be gracefully terminated "
              "when using a time limit, which, however, is probably "
              "supported on your platform anyway.")
    try:
        # Reserve about 10 MB of emergency memory.
        # https://stackoverflow.com/questions/19469608/
        emergency_memory = b"x" * 10**7
        main()
    except MemoryError:
        del emergency_memory
        print()
        print("Translator ran out of memory, traceback:")
        print("=" * 79)
        traceback.print_exc(file=sys.stdout)
        print("=" * 79)
        sys.exit(TRANSLATE_OUT_OF_MEMORY)
    except pddl_parser.ParseError as e:
        print(e)
        sys.exit(TRANSLATE_INPUT_ERROR)
