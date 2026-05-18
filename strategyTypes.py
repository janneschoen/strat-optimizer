
class StrategyType:
    def __init__(self, name, numParams, paramNames, lookbackParam):
        self.name = name
        self.numParams = numParams
        self.paramNames = paramNames
        self.lookbackParam = lookbackParam

    def isValid(self, p):
        if len(p) != self.numParams:
            return False

        if self.name == "SMA Crossover":
            if p[0] >= p[1]:
                return False
            if p[1] < 2:
                return False
            if p[2] <= 0:
                return False
            if p[2] > 1:
                return False

        if self.name == "RSI":
            if p[2] < 2:
                return False
            if p[0] < 0:
                return False
            if p[1] > 100:
                return False
            if p[0] > p[1]:
                return False

        return True
