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

// Budget-based version: runs until exactly maxEvals fitness function
// evaluations (FFE) have been spent (one FFE = one calculateFitness call,
// including infeasible ants). ffeBest is filled with the best-so-far fitness
// after each evaluation (size == maxEvals on return).
vector<uint32_t> antColonyOptimizationBudget(const vector<UE> &ue_list,
                                             uint32_t numAnts,
                                             float pheroCoeff,
                                             float heurCoeff,
                                             float rho,
                                             float q,
                                             uint32_t maxEvals,
                                             vector<float> &ffeBest);