"""
strategy.py — Strategy metadata and parameter validation

A Strategy bundles a public name with a list of ParameterConfig
entries.  Dispatch in the C engine is by that name (see the
STRATEGY_REGISTRY table in src/backtesting.c), so there is no
positional index to keep in sync with strategies.json.

ParameterConfig supports optional relational constraints:
  • upper_param            index of another parameter that this
                           value must be strictly less than
                           (e.g. Fast SMA < Slow SMA)
  • defines_lookback       exactly one parameter per strategy must
                           carry this flag; its maximum value
                           determines how many past prices the
                           C engine must provide

Numeric bounds are NOT declared here — the search space is fully
defined by `parameter_ranges` / `parameter_steps` in the run config
(a single source of truth for the grid).
"""

from dataclasses import dataclass
from typing import List

# Maximum number of parameters the C engine can represent
# (strategy_config_t.params).  Keep in sync with MAX_PARAMS in
# src/common.h — backtesting.py asserts the two match at import.
MAX_PARAMS = 3


@dataclass
class ParameterConfig:
    name:             str
    upper_param:      int  = None
    defines_lookback: bool = False


@dataclass
class Strategy:
    name:       str
    parameters: List[ParameterConfig]

    def __post_init__(self):
        self.number_of_parameters = len(self.parameters)
        if self.number_of_parameters > MAX_PARAMS:
            raise ValueError(
                f"Strategy '{self.name}' defines "
                f"{self.number_of_parameters} parameters, but the engine "
                f"supports at most {MAX_PARAMS} (MAX_PARAMS)."
            )

    def is_valid(self, p: List[float]) -> bool:
        """
        Return True if the combination *p* satisfies all relational
        constraints defined for this strategy.

        The numeric search space is enforced when the grid is built
        (see parameters.py); this method only checks constraints that
        cannot be expressed as independent per-parameter ranges.
        """
        if len(p) != self.number_of_parameters:
            return False

        for i in range(len(p)):
            upper = self.parameters[i].upper_param
            if upper is not None and p[i] >= p[upper]:
                return False

        return True
