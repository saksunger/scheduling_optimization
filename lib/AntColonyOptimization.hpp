#pragma once

#include "Ue.hpp"
#include <cstdint>
#include <vector>

using namespace std;

// Original function signature for backward compatibility
vector<uint32_t> antColonyOptimization(const vector<UE> &ue_list);

// New overloaded function with parameters
vector<uint32_t> antColonyOptimization(const vector<UE> &ue_list,
                                       uint32_t iterations,
                                       uint32_t numAnts,
                                       float pheroCoeff,
                                       float heurCoeff,
                                       float rho,
                                       float q);

// New overloaded function with additional parameter to capture iteration fitness values
vector<uint32_t> antColonyOptimization(const vector<UE> &ue_list,
                                       uint32_t iterations,
                                       uint32_t numAnts,
                                       float pheroCoeff,
                                       float heurCoeff,
                                       float rho,
                                       float q,
                                       vector<float> &iterationFitness);