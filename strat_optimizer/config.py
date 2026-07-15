"""
config.py — Load and validate a run configuration from JSON

Reads two JSON files:
  1. The run config   (default: config.json)
     - strategy name, parameter ranges & steps, backtest length,
       test split, asset ticker
  2. The strategy definitions file  (default: strategies.json)
     - metadata for each strategy: parameter names, constraints

Produces a RunConfig dataclass that the rest of the pipeline
consumes.  Also derives the lookback — the minimum number of
historical prices the C engine needs before it can start trading.
"""

from dataclasses import dataclass
from .strategies import Strategy, ParameterConfig
import sys, json


DEFAULT_CONFIG_FILE = "configs/config.json"


@dataclass
class Asset:
    ticker:       str
    trading_days: int    # 252 (equities) or 365 (crypto / forex)


@dataclass
class RunConfig:
    """Central configuration for one optimisation run."""

    strategy: Strategy

    # grid-search parameters
    parameter_ranges: list   # [[lo, hi], ...]  one per parameter
    parameter_steps:  list   # step size; 0 = fixed at range[0]

    # backtest settings
    backtest_length: int     # number of trading days to test
    asset:           Asset
    test_size:       float   # fraction of data held out [0, 1]

    def __post_init__(self):
        self.lookback = self._calculate_lookback()

    def _calculate_lookback(self) -> int:
        """
        The lookback is the maximum value of the parameter flagged
        'defines_lookback' in the strategy definition.  This tells
        us how many historical prices the signal function needs
        before day one.

        If the step for that parameter is 0 (fixed), the lookback
        is the fixed value itself; otherwise it is the upper bound
        of its range.
        """
        for i, param in enumerate(self.strategy.parameters):
            if param.defines_lookback:
                rng = self.parameter_ranges[i]
                if len(rng) < 2 or self.parameter_steps[i] == 0:
                    val = rng[0]
                else:
                    val = rng[1]
                if val > 0:
                    return int(val)

        raise ValueError(
            "Could not derive the required lookback from parameters."
        )


def load_config() -> RunConfig:
    """
    Read the config JSON (CLI argument or config.json) and the
    strategies definition file.  Build and return a RunConfig.
    """
    config_file = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_CONFIG_FILE
    print(f"Config file: '{config_file}'")

    with open(config_file) as f:
        config = json.load(f)

    strategies_file = config.get("strategies_file", "configs/strategies.json")

    with open(strategies_file) as f:
        strategies = json.load(f)

    # match the strategy name to its definition
    strategy = None
    for i, s in enumerate(strategies):
        if s["name"] == config["strategy_name"]:
            params = []
            for p in s["parameters"]:
                params.append(ParameterConfig(
                    name             = p["name"],
                    min              = p.get("min"),
                    max              = p.get("max"),
                    upper_param      = p.get("upper_param"),
                    defines_lookback = p.get("defines_lookback", False),
                ))
            strategy = Strategy(
                name       = s["name"],
                index      = i,
                parameters = params,
            )
            break

    if strategy is None:
        raise ValueError(
            f"Strategy '{config['strategy_name']}' not found in "
            f"'{strategies_file}'."
        )

    print(f"Strategy: '{strategy.name}' from '{strategies_file}'")

    trading_days = 365 if config["asset"]["is_traded_all_year"] else 252

    return RunConfig(
        strategy          = strategy,
        parameter_ranges  = config["parameter_ranges"],
        parameter_steps   = config["parameter_steps"],
        backtest_length   = config["backtest_length"],
        test_size         = config["test_size"],
        asset=Asset(
            ticker       = config["asset"]["ticker"],
            trading_days = trading_days,
        ),
    )
