"""
parameters.py — Grid-search parameter combination generator

Builds a Cartesian product of the parameter ranges, then filters
through the strategy's constraint validator (is_valid) to discard
impossible combinations (e.g. Fast SMA ≥ Slow SMA).

A step of 0 means the parameter is fixed at its range[0] value.
"""

from .config import RunConfig
from dataclasses import dataclass
import itertools
import numpy as np


@dataclass
class GridSummary:
    """Metadata about the generated parameter grid."""
    total_combinations:   int
    valid_combinations:   int
    filtered_out:         int
    free_params:          list   # [(name, range_lo, range_hi, step), ...]
    fixed_params:         list   # [(name, value), ...]


def generate_parameter_combinations(run: RunConfig):
    """
    Returns (number_of_combinations, list_of_tuples, GridSummary).

    Each tuple is one valid parameter combination to backtest.
    """

    if len(run.parameter_steps) != len(run.parameter_ranges):
        raise ValueError(
            f"Got {len(run.parameter_ranges)} parameter ranges "
            f"but {len(run.parameter_steps)} steps."
        )

    # build per-parameter candidate lists
    param_lists = []
    free_params = []
    fixed_params = []

    for idx, rng in enumerate(run.parameter_ranges):
        name = run.strategy.parameters[idx].name
        step = run.parameter_steps[idx]
        if step == 0:
            param_lists.append([rng[0]])          # fixed value
            fixed_params.append((name, rng[0]))
        else:
            # np.arange is half-open: [rng[0], rng[1])
            param_lists.append(
                np.arange(rng[0], rng[1], step).tolist()
            )
            free_params.append((name, rng[0], rng[1], step))

    all_combos = list(itertools.product(*param_lists))

    # filter through strategy constraints
    valid = [c for c in all_combos if run.strategy.is_valid(c)]

    if not valid:
        raise ValueError(
            "No valid parameter combinations after applying "
            "strategy constraints."
        )

    summary = GridSummary(
        total_combinations=len(all_combos),
        valid_combinations=len(valid),
        filtered_out=len(all_combos) - len(valid),
        free_params=free_params,
        fixed_params=fixed_params,
    )

    return len(valid), valid, summary
