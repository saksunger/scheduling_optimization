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

// Budget-based version: runs until exactly maxEvals fitness function
// evaluations (FFE) have been spent, instead of a fixed iteration count.
// ffeBest is filled with the best-so-far fitness after each evaluation
// (size == maxEvals on return), so curves of different algorithms share
// the same FFE x-axis.
vector<uint32_t> harmonySearchBudget(const vector<UE> &ue_list,
                                     uint32_t hms,
                                     float hmcr,
                                     float par,
                                     float lsr,
                                     uint32_t maxEvals,
                                     vector<float> &ffeBest);