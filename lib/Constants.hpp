#pragma once

#include "Modes.hpp"
#include <array>
#include <cstdint>

using namespace std;

extern float MAX_PERIOD;
extern float MAX_EQ_RB;
extern const uint32_t NUM_OF_UE;
constexpr uint32_t NUM_OF_RB = 50;
constexpr uint32_t NUM_ITERATIONS = 1000;
constexpr uint32_t HMS = 10; // Harmony Memory Size
constexpr float HMCR = 0.9;  // Harmony Memory Considering Rate
constexpr float PAR = 0.3;   // Pitch Adjusting Rate

constexpr uint32_t NUM_OF_ANT = 10;
constexpr float PHEROMONE_COEFF = 1.0;
constexpr float HEURISTIC_COEFF = 2.0;
constexpr float ACO_RHO = 0.1;
constexpr float ACO_Q = 100.0;

extern array<uint32_t, UeType::NUM_OF_UE_TYPES> cumulativeNumberOfUEsPerMode;
extern array<float, UeType::NUM_OF_UE_TYPES> timeCoeffArr;
extern array<float, UeType::NUM_OF_UE_TYPES> rbCoeffArr;
extern array<float, UeType::NUM_OF_UE_TYPES> energyCoeffArr;