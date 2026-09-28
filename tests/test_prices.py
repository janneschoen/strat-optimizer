"""Tests for the Yahoo Finance downloader's length guard."""

import pytest

from strat_optimizer import prices as prices_mod
from tests.helpers import build_run


class _Series(dict):
    def to_dict(self):
        return dict(self)


class _Raw:
    """Minimal stand-in for the object returned by yf.download()."""

    def __init__(self, series):
        self._series = series

    def __getitem__(self, key):
        assert key == "Open"
        return {"TEST": self._series}


def _make_run(sma_strategy):
    # backtest_length=8 + lookback=2 -> 10 prices requested
    return build_run(
        sma_strategy,
        ranges=[[1, 1], [2, 2], [0.5, 0.5]],
        steps=[0, 0, 0],
        backtest_length=8,
    )


def test_insufficient_history_raises(monkeypatch, sma_strategy):
    run = _make_run(sma_strategy)
    series = _Series({f"2024-01-{d:02d}": 100 + d for d in range(1, 6)})
    monkeypatch.setattr(prices_mod.yf, "download", lambda **kw: _Raw(series))

    with pytest.raises(RuntimeError, match="Only 5 prices available"):
        prices_mod.download_prices(run)


def test_empty_history_raises(monkeypatch, sma_strategy):
    run = _make_run(sma_strategy)
    monkeypatch.setattr(prices_mod.yf, "download",
                        lambda **kw: _Raw(_Series({})))

    with pytest.raises(RuntimeError, match="No price data"):
        prices_mod.download_prices(run)


def test_enough_history_trimmed_to_requested(monkeypatch, sma_strategy):
    run = _make_run(sma_strategy)
    series = _Series({f"2024-01-{d:02d}": 100 + d for d in range(1, 13)})
    monkeypatch.setattr(prices_mod.yf, "download", lambda **kw: _Raw(series))

    n, arr, first, last = prices_mod.download_prices(run)
    assert n == 10
    assert len(arr) == 10
    # keeps the most recent 10 prices
    assert list(arr) == [103 + i for i in range(10)]
