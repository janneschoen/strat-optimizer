"""Tests for strategy metadata and parameter validation."""

import pytest

from strat_optimizer.strategy import Strategy, ParameterConfig, MAX_PARAMS
from strat_optimizer.backtesting import _lib


def test_upper_param_constraint(sma_strategy):
    assert sma_strategy.is_valid([1, 5, 0.5])
    assert not sma_strategy.is_valid([5, 5, 0.5])   # fast must be < slow
    assert not sma_strategy.is_valid([6, 5, 0.5])


def test_wrong_parameter_count_is_invalid(sma_strategy):
    assert not sma_strategy.is_valid([1, 5])
    assert not sma_strategy.is_valid([1, 5, 0.5, 9])


def test_too_many_parameters_rejected():
    params = [ParameterConfig(f"p{i}") for i in range(MAX_PARAMS + 1)]
    with pytest.raises(ValueError, match="MAX_PARAMS"):
        Strategy("Too Big", params)


def test_max_params_matches_c_engine():
    assert MAX_PARAMS == int(_lib.engine_max_params())
