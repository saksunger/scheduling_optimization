#include "Constants.hpp"
#include "Ue.hpp"
#include <array>
#include <vector>

using namespace std;

vector<vector<bool>> timeline(MAX_TIME, vector<bool>(NUM_OF_RB, false));

array<uint32_t, UeType::NUM_OF_UE_TYPES> numberOfUEsPerMode = {100, 250, 250};

array<float, UeType::NUM_OF_UE_TYPES> timeCoeffArr = {0.07, 0.003, 0.001};
array<float, UeType::NUM_OF_UE_TYPES> energyCoeffArr = {0.5, 1.5, 3.0};

array<float, UeType::NUM_OF_UE_TYPES> heuristicLatencyWeight = {3.0, 1.0, 0.5};
array<float, UeType::NUM_OF_UE_TYPES> heuristicEnergyWeight = {0.5, 1.0, 3.0};
