from dataclasses import dataclass
from strategies import Strategy
from strategies import ParameterConfig
import sys, json, os

DEFAULT_CONFIG_FILE = "config.json"

@dataclass
class Asset():
    ticker: str
    trading_days: int

@dataclass
class RunConfig():
    """Central configuration for one optimization run"""
    strategy: Strategy

    # Grid search
    parameter_ranges: List[List[float]]
    parameter_steps: List[float]

    # Backtest settings
    backtest_length: int
    asset: Asset
    test_size: float

    # File paths

    temp_folder: str = ".temp"
    prices_file: str = "prices.temp"
    parameter_file: str = "parameters.temp"
    performances_file: str = "performances.temp"
    equity_file: str = "equity.temp"

    def __post_init__(self):
        self.temp_folder += '/'
        self.prices_path = self.temp_folder + self.prices_file
        self.parameter_path = self.temp_folder + self.parameter_file
        self.performances_path = self.temp_folder + self.performances_file
        self.equity_path = self.temp_folder + self.equity_file

        self.lookback = self._calculate_lookback()
    
    def _calculate_lookback(self) -> int:

        for i, param in enumerate(self.strategy.parameters):
            if param.defines_lookback:
                lookback = self.parameter_ranges[i][1 if self.parameter_steps[i] else 0]
                if lookback > 0:
                    return lookback

        raise ValueError(f"Could not derive the required lookback from given parameters.")

def load_config() -> RunConfig:
    if len(sys.argv) == 1:
        config_file = DEFAULT_CONFIG_FILE
    else:
        config_file = sys.argv[1]
    
    print(f"Config file: '{config_file}'")

    with open(config_file, 'r') as f:
        config = json.load(f)

    strategies_file = config.get("strategies_file")
    if not strategies_file:
        strategies_file = "strategies.json"
    
    with open(strategies_file, 'r') as f:
        strategies = json.load(f)

    for i, s in enumerate(strategies):
        if s["name"] == config["strategy_name"]:
            parameters = []
            for param in s["parameters"]:
                parameters.append(ParameterConfig(
                    name = param["name"],
                    min = param.get("min"),
                    max = param.get("max"),
                    upper_param = param.get("upper_param"),
                    defines_lookback = param.get("defines_lookback")
                ))
            strategy = Strategy(
                name = s["name"],
                index = i,
                parameters = parameters,
            )
            break

    print(f"Strategy: '{strategy.name}' from '{strategies_file}'")

    run_config = RunConfig(
        strategy = strategy,
        parameter_ranges = config["parameter_ranges"],
        parameter_steps = config["parameter_steps"],
        backtest_length = config["backtest_length"],
        test_size = config["test_size"],
        asset = Asset(
            ticker = config["asset"]["ticker"],
            trading_days = 365 if config["asset"]["is_traded_all_year"] else 252
        )
    )

    os.makedirs(os.path.dirname(run_config.temp_folder), exist_ok=True)

    return run_config