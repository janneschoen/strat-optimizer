
class StrategyType:
    def __init__(self, name, parameters):
        self.name = name
        self.parameters = parameters
        self.numParams = len(self.parameters)

    def isValid(self, p):

        if len(p) != self.numParams:
            return False

        for i in range(len(p)):

            # Check if parameter is bigger than other parameter (if it should be smaller)

            upper_param = self.parameters[i].get("upper_param")
            if upper_param and p[i] > p[upper_param]:
                return False

            
            # Check if parameter is in numeric limits (if they exist)

            lower_limit = self.parameters[i].get("min")
            if lower_limit and p[i] < lower_limit:
                return False

            upper_limit = self.parameters[i].get("max")
            if upper_limit and p[i] > upper_limit:
                return False

        return True
