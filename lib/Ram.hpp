#pragma once

#include "Constants.hpp"
#include "Ue.hpp"
#include <array>
#include <vector>

using namespace std;

extern vector<vector<bool>> timeline;

inline void resetTimeline() {

  timeline.assign(MAX_TIME, vector<bool>(NUM_OF_RB, false));
}

extern array<uint32_t, UeType::NUM_OF_UE_TYPES> numberOfUEsPerMode;

extern array<float, UeType::NUM_OF_UE_TYPES> timeCoeffArr;
extern array<float, UeType::NUM_OF_UE_TYPES> energyCoeffArr;

extern array<float, UeType::NUM_OF_UE_TYPES> heuristicLatencyWeight;
extern array<float, UeType::NUM_OF_UE_TYPES> heuristicEnergyWeight;
