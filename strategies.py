
class Strategy:
    def __init__(self, name, parameters):
        self.name = name
        self.parameters = parameters
        self.number_of_parameters = len(self.parameters)

    def is_valid(self, p):

        if len(p) != self.number_of_parameters:
            return False

        for i in range(len(p)):

            # Check if parameter is smaller than other parameter (if it should be)

            upper_param = self.parameters[i].get("upper_param")
            if upper_param and p[i] >= p[upper_param]:
                return False

            
            # Check if parameter is in numeric limits (if they exist)

            lower_limit = self.parameters[i].get("min")
            if lower_limit and p[i] < lower_limit:
                return False

            upper_limit = self.parameters[i].get("max")
            if upper_limit and p[i] > upper_limit:
                return False

        return True
