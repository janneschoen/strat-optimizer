from config import RunConfig
import itertools, numpy as np

def generate_parameter_combinations(run: RunConfig) -> (int, List[Tuple[float]]):

    if len(run.parameter_steps) != len(run.parameter_ranges):
        raise ValueError(f"Got {len(run.parameter_ranges)} parameters but {len(run.parameter_steps)} steps.")


    # Generate cartesian product by multiplying lists of parameters

    parameter_lists = []

    for parameter_range in run.parameter_ranges:

        step = run.parameter_steps[len(parameter_lists)]
        if step == 0:
            parameter_lists.append([parameter_range[0]])
        else:
            parameter_lists.append(np.arange(parameter_range[0], parameter_range[1], step))

    all_combos = list(itertools.product(*parameter_lists))


    # Save valid parameter combos

    parameter_combos = []

    for combo in all_combos:
        if run.strategy.is_valid(combo):
            parameter_combos.append(combo)

    number_of_combinations = len(parameter_combos)

    if(number_of_combinations) == 0:
        raise ValueError("Could not generate any valid parameter combinations.")
    

    # Writing parameter combinations to file

    with open(run.parameter_path, 'w') as file:
        for combo in parameter_combos:
            for param in combo:
                file.write(str(param)+" ")
            file.write("\n")
    
    return number_of_combinations, parameter_combos
