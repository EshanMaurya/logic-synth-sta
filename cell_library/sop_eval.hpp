#pragma once 
#include "cell_library.hpp"

bool sop_eval(const SOP& function,const std::vector<bool>& inputs);

// inputs[k] is the value of the input whose pinIndex is k; inverted flag is applied inside the function
