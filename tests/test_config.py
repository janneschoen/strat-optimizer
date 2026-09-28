"""Tests for config loading and lookback derivation."""

import json

import pytest

from strat_optimizer.config import load_config, DEFAULT_CONFIG_FILE


def test_load_example_config():
    run = load_config("configs/example.json")
    assert run.strategy.name == "SMA Crossover"
    assert run.strategy.number_of_parameters == 3
    # slow SMA upper bound is the lookback
    assert run.lookback == 200
    assert run.asset.trading_days == 365
    assert run._config_path == "configs/example.json"


def test_lookback_from_fixed_parameter(tmp_path, sma_strategy):
    config = {
        "strategy_name": "SMA Crossover",
        "strategies_file": "configs/strategies.json",
        "parameter_ranges": [[1, 50], [30, 30], [0.2, 0.2]],
        "parameter_steps": [3, 0, 0],
        "backtest_length": 100,
        "test_size": 0.4,
        "asset": {"ticker": "X", "is_traded_all_year": False},
    }
    path = tmp_path / "config.json"
    path.write_text(json.dumps(config))

    run = load_config(str(path))
    assert run.lookback == 30          # fixed slow SMA value
    assert run.asset.trading_days == 252


def test_unknown_strategy_raises(tmp_path):
    config = {
        "strategy_name": "Nope",
        "strategies_file": "configs/strategies.json",
        "parameter_ranges": [[1, 2]],
        "parameter_steps": [1],
        "backtest_length": 100,
        "test_size": 0.4,
        "asset": {"ticker": "X", "is_traded_all_year": False},
    }
    path = tmp_path / "config.json"
    path.write_text(json.dumps(config))

    with pytest.raises(ValueError, match="not found"):
        load_config(str(path))


def test_default_config_path_constant():
    assert DEFAULT_CONFIG_FILE == "configs/config.json"
