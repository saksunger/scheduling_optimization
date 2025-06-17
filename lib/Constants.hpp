#pragma once

#include <cstdint>

using namespace std;

constexpr uint32_t MAX_TIME = 6000;
constexpr uint32_t MAX_ENERGY_PER_UE = 150;
constexpr uint32_t NUM_OF_RB = 50;
constexpr uint32_t NUM_ITERATIONS = 100;

constexpr uint32_t HMS = 10; // Harmony Memory Size
constexpr float HMCR = 0.9;  // Harmony Memory Considering Rate
constexpr float PAR = 0.3;   // Pitch Adjusting Rate

constexpr uint32_t NUM_OF_ANT = 10;
constexpr float PHEROMONE_COEFF = 1.0;
constexpr float HEURISTIC_COEFF = 2.0;
constexpr float ACO_RHO = 0.1;
constexpr float ACO_Q = 100.0;

extern const uint32_t TOTAL_UE;