"""
strategies.py — Strategy metadata and parameter validation

A Strategy bundles a name, an index (its position in the C engine's
get_signal[] table), and a list of ParameterConfig entries.

ParameterConfig supports optional constraints:
  • min / max              numeric bounds for the parameter value
  • upper_param            index of another parameter that this
                           value must be strictly less than
                           (e.g. Fast SMA < Slow SMA)
  • defines_lookback       exactly one parameter per strategy must
                           carry this flag; its maximum value
                           determines how many past prices the
                           C engine must provide
"""

from dataclasses import dataclass
from typing import List


@dataclass
class ParameterConfig:
    name:             str
    min:              float = None
    max:              float = None
    upper_param:      int   = None
    defines_lookback: bool  = False


@dataclass
class Strategy:
    name:       str
    index:      int           # index into the C signal function table
    parameters: List[ParameterConfig]

    def __post_init__(self):
        self.number_of_parameters = len(self.parameters)

    def is_valid(self, p: List[float]) -> bool:
        """
        Return True if the combination *p* satisfies all constraints
        defined for this strategy.

        Enforced rules:
          1. Parameter count must match.
          2. If upper_param is set, p[i] < p[upper_param].
          3. min / max bounds are respected.
          4. Leading zeros in integers are treated as falsy and
             ignored (by design, since 0 means "no bound").
        """

        if len(p) != self.number_of_parameters:
            return False

        for i in range(len(p)):

            upper = self.parameters[i].upper_param
            if upper is not None and p[i] >= p[upper]:
                return False

            lo = self.parameters[i].min
            if lo is not None and p[i] < lo:
                return False

            hi = self.parameters[i].max
            if hi is not None and p[i] > hi:
                return False

        return True
