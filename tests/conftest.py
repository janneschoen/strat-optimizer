"""Shared pytest fixtures for the test suite."""

import sys
from pathlib import Path

import numpy as np
import pytest

# make the package importable when pytest is run from anywhere
REPO_ROOT = Path(__file__).resolve().parent.parent
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

from strat_optimizer.strategy import Strategy, ParameterConfig  # noqa: E402


@pytest.fixture
def sma_strategy():
    return Strategy("SMA Crossover", [
        ParameterConfig("Fast SMA Length", upper_param=1),
        ParameterConfig("Slow SMA Length", defines_lookback=True),
        ParameterConfig("Position Sizing"),
    ])


@pytest.fixture
def rsi_strategy():
    return Strategy("RSI", [
        ParameterConfig("Buying Threshold", upper_param=1),
        ParameterConfig("Selling Threshold"),
        ParameterConfig("Window Size", defines_lookback=True),
    ])


@pytest.fixture
def rising_prices():
    return np.arange(100, 110, dtype=np.float32)
