from dataclasses import dataclass

@dataclass
class ParameterConfig():
    name: str
    min: float = None
    max: float = None
    upper_param: int = None
    defines_lookback: bool = False

@dataclass
class Strategy:
    name: str
    index: int
    parameters: List[ParameterConfig]

    def __post_init__(self):
        self.number_of_parameters = len(self.parameters)

    def is_valid(self, p: List[float]) -> bool :

        if len(p) != self.number_of_parameters:
            return False

        for i in range(len(p)):

            # Check if parameter is smaller than other parameter (if it should be)

            upper_param = self.parameters[i].upper_param
            if upper_param and p[i] >= p[upper_param]:
                return False

            # Check if parameter is in numeric limits (if they exist)

            minimum = self.parameters[i].min
            if minimum and p[i] < minimum:
                return False

            maximum = self.parameters[i].max
            if maximum and p[i] > maximum:
                return False

        return True
