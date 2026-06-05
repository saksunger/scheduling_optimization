#pragma once

#include "Ue.hpp"
#include <cstdint>
#include <vector>

using namespace std;

// Original function signature for backward compatibility
vector<uint32_t> harmonySearch(const vector<UE> &ue_list);

// New overloaded function with parameters
vector<uint32_t> harmonySearch(const vector<UE> &ue_list,
                               uint32_t iterations,
                               uint32_t hms,
                               float hmcr,
                               float par,
                               float lsr);

// New overloaded function with additional parameter to capture iteration fitness values
vector<uint32_t> harmonySearch(const vector<UE> &ue_list,
                               uint32_t iterations,
                               uint32_t hms,
                               float hmcr,
                               float par,
                               float lsr,
                               vector<float> &iterationFitness);