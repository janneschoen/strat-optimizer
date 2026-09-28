"""Small helpers shared by the test modules."""

from strat_optimizer.config import RunConfig, Asset
from strat_optimizer.strategy import Strategy


def build_run(strategy: Strategy,
              ranges: list,
              steps: list,
              backtest_length: int = 8,
              test_size: float = 0.0,
              transaction_cost: float = 0.0,
              ticker: str = "TEST") -> RunConfig:
    """Construct a RunConfig directly, without touching the filesystem."""
    return RunConfig(
        strategy         = strategy,
        parameter_ranges = ranges,
        parameter_steps  = steps,
        backtest_length  = backtest_length,
        test_size        = test_size,
        transaction_cost = transaction_cost,
        show_plots       = False,
        asset            = Asset(ticker, 365),
    )
