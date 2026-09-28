"""Tests for the C engine bridge: ABI, metrics, and signal semantics."""

import ctypes

import numpy as np
import pytest

from strat_optimizer import backtesting as bt
from strat_optimizer.backtesting import (
    EngineArgs, run_backtesting_engine, NUM_METRICS,
)
from tests.helpers import build_run


def _sma_run(sma_strategy, alloc=0.5, tc=0.0):
    # fixed fast=1, slow=2, alloc -> single combination, lookback=2
    return build_run(
        sma_strategy,
        ranges=[[1, 1], [2, 2], [alloc, alloc]],
        steps=[0, 0, 0],
        transaction_cost=tc,
    )


def test_abi_contract():
    assert NUM_METRICS == 7
    assert ctypes.sizeof(EngineArgs) == int(bt._lib.engine_args_sizeof())


def test_long_hold_matches_closed_form(sma_strategy, rising_prices):
    alloc = 0.5
    run = _sma_run(sma_strategy, alloc=alloc)
    perfs, equity = run_backtesting_engine(
        run, len(rising_prices), rising_prices, [(1, 2, alloc)])

    p = perfs[0]
    entry_price = rising_prices[run.lookback]
    expected = alloc * (rising_prices[-1] / entry_price - 1.0)

    assert p.total_return == pytest.approx(expected, rel=1e-6)
    # equity starts at the initial budget
    assert equity[0] == pytest.approx(10000.0)


def test_transaction_cost_reduces_return(sma_strategy, rising_prices):
    alloc = 0.5
    free = _sma_run(sma_strategy, alloc=alloc, tc=0.0)
    costly = _sma_run(sma_strategy, alloc=alloc, tc=0.01)

    p_free, _ = run_backtesting_engine(
        free, len(rising_prices), rising_prices, [(1, 2, alloc)])
    p_cost, _ = run_backtesting_engine(
        costly, len(rising_prices), rising_prices, [(1, 2, alloc)])

    assert p_cost[0].total_return < p_free[0].total_return


def test_rsi_neutral_zone_is_flat(rsi_strategy, rising_prices):
    # buy_thresh=0 / sell_thresh=100 can never be crossed, so the
    # strategy returns SIGNAL_FLAT every day and stays in cash.
    run = build_run(
        rsi_strategy,
        ranges=[[0, 0], [100, 100], [2, 2]],
        steps=[0, 0, 0],
    )
    perfs, equity = run_backtesting_engine(
        run, len(rising_prices), rising_prices, [(0, 100, 2)])

    assert perfs[0].total_return == pytest.approx(0.0)
    assert np.allclose(equity, 10000.0)


def test_multiple_combinations_returns_all_results(sma_strategy,
                                                   rising_prices):
    run = _sma_run(sma_strategy)
    combos = [(1, 2, 0.25), (1, 3, 0.5)]
    perfs, _ = run_backtesting_engine(
        run, len(rising_prices), rising_prices, combos)
    assert len(perfs) == 2
    # higher exposure -> higher return on a rising series
    assert perfs[1].total_return > perfs[0].total_return


# ---- engine-level guards (called through ctypes directly) -----------

def _raw_args(prices, perf, params, name):
    args = EngineArgs()
    args.prices = prices.ctypes.data_as(ctypes.POINTER(ctypes.c_float))
    args.param_grid = params.ctypes.data_as(ctypes.POINTER(ctypes.c_float))
    args.performances = perf.ctypes.data_as(ctypes.POINTER(ctypes.c_float))
    args.equity_curve = None
    args.n_prices = len(prices)
    args.n_combos = 1
    args.n_params = len(params)
    args.strategy_name = name
    args.start = 2
    args.end = len(prices)
    args.trading_days = 365
    args.transaction_cost = 0.0
    return args


def test_unknown_strategy_is_rejected(capfd):
    prices = np.arange(100, 106, dtype=np.float32)
    perf = np.zeros(NUM_METRICS, dtype=np.float32)
    params = np.array([1.0], dtype=np.float32)

    args = _raw_args(prices, perf, params, b"does not exist")
    bt._lib.engine_run(ctypes.byref(args))

    assert np.all(perf == 0.0)
    assert "unknown strategy" in capfd.readouterr().err


def test_too_many_parameters_is_rejected(capfd):
    prices = np.arange(100, 106, dtype=np.float32)
    perf = np.zeros(NUM_METRICS, dtype=np.float32)
    params = np.array([1.0, 2.0, 3.0, 4.0], dtype=np.float32)

    args = _raw_args(prices, perf, params, b"SMA Crossover")
    bt._lib.engine_run(ctypes.byref(args))

    assert np.all(perf == 0.0)
    assert "MAX_PARAMS" in capfd.readouterr().err
