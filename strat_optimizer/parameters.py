"""
parameters.py — Grid-search parameter combination generator

Builds a Cartesian product of the parameter ranges, then filters
through the strategy's constraint validator (is_valid) to discard
impossible combinations (e.g. Fast SMA ≥ Slow SMA).

A step of 0 means the parameter is fixed at its range[0] value.
"""

from .config import RunConfig
import itertools
import numpy as np


def generate_parameter_combinations(run: RunConfig):
    """
    Returns (number_of_combinations, list_of_tuples).

    Each tuple is one valid parameter combination to backtest.
    """

    if len(run.parameter_steps) != len(run.parameter_ranges):
        raise ValueError(
            f"Got {len(run.parameter_ranges)} parameter ranges "
            f"but {len(run.parameter_steps)} steps."
        )

    # build per-parameter candidate lists
    param_lists = []

    for idx, rng in enumerate(run.parameter_ranges):
        step = run.parameter_steps[idx]
        if step == 0:
            param_lists.append([rng[0]])          # fixed value
        else:
            # np.arange is half-open: [rng[0], rng[1])
            param_lists.append(
                np.arange(rng[0], rng[1], step).tolist()
            )

    all_combos = list(itertools.product(*param_lists))

    # filter through strategy constraints
    valid = [c for c in all_combos if run.strategy.is_valid(c)]

    if not valid:
        raise ValueError(
            "No valid parameter combinations after applying "
            "strategy constraints."
        )

    return len(valid), valid
