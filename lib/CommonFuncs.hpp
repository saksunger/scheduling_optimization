#pragma once

#include "Modes.hpp"
#include "Ue.hpp"
#include <cmath>
#include <cstdint>
#include <vector>

using namespace std;

struct FitnessResult {
  float totalCost;
  float totalEnergy[UeType::NUM_OF_UE_TYPES];
  float totalTime[UeType::NUM_OF_UE_TYPES];
};

// Fitness function evaluation (FFE) accounting: one FFE = one call to
// calculateFitness. Used to terminate every algorithm at the same budget.
extern uint64_t g_evalCount;
inline void resetEvalCounter() { g_evalCount = 0; }
inline uint64_t getEvalCount() { return g_evalCount; }

inline uint32_t getRepetitionCount(const UE &ue, const Mode &mode) {
  return static_cast<uint32_t>(
      ceil(static_cast<float>(ue.data) / mode.rb_per_period));
}

vector<int32_t> generateRBs(int32_t numUEs, int32_t totalTargetRB,
                            int32_t minRB, int32_t maxRB);
bool canSchedule(const UE &ue, const Mode &selected_mode, uint32_t start);
void scheduleUE(const UE &ue, const Mode &selected_mode, uint32_t start);
FitnessResult calculateFitness(const vector<UE> &ue_list,
                               const vector<uint32_t> &solution);
void printFitness(const vector<UE> &ue_list, const vector<uint32_t> &solution);
