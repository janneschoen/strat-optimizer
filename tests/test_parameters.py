"""Tests for grid-search parameter generation."""

import pytest

from strat_optimizer.parameters import generate_parameter_combinations
from tests.helpers import build_run


def test_grid_size_and_filtering(sma_strategy):
    # fast in {1,3}, slow in {1,3}, alloc fixed -> 4 combos, constrained to
    # fast < slow -> (1,3) only
    run = build_run(
        sma_strategy,
        ranges=[[1, 4], [1, 4], [0.5, 0.5]],
        steps=[2, 2, 0],
    )
    n, combos, summary = generate_parameter_combinations(run)
    assert summary.total_combinations == 4
    assert summary.filtered_out == 3
    assert n == 1
    assert combos == [(1, 3, 0.5)]


def test_fixed_parameter_appears_in_summary(sma_strategy):
    run = build_run(
        sma_strategy,
        ranges=[[1, 3], [2, 5], [0.25, 0.25]],
        steps=[1, 1, 0],
    )
    _, _, summary = generate_parameter_combinations(run)
    assert summary.fixed_params == [("Position Sizing", 0.25)]
    assert [p[0] for p in summary.free_params] == [
        "Fast SMA Length", "Slow SMA Length"]


def test_parameter_count_mismatch_raises(sma_strategy):
    with pytest.raises(ValueError, match="parameters"):
        run = build_run(sma_strategy, ranges=[[1, 3]], steps=[1])
        generate_parameter_combinations(run)


def test_negative_step_raises(sma_strategy):
    run = build_run(sma_strategy,
                    ranges=[[1, 3], [2, 5], [0.5, 0.5]],
                    steps=[-1, 1, 0])
    with pytest.raises(ValueError, match="negative step"):
        generate_parameter_combinations(run)


def test_empty_arange_raises(sma_strategy):
    # half-open range where low == high yields no values
    run = build_run(sma_strategy,
                    ranges=[[5, 5], [6, 9], [0.5, 0.5]],
                    steps=[1, 1, 0])
    with pytest.raises(ValueError, match="no values"):
        generate_parameter_combinations(run)
