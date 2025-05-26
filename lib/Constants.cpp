#include "Constants.hpp"
#include "Modes.hpp"
#include <array>
#include <cstdint>

using namespace std;

float MAX_PERIOD;
float MAX_EQ_RB;
array<uint32_t, UeType::NUM_OF_UE_TYPES> numberOfUEsPerMode = {12, 12, 20};
array<uint32_t, UeType::NUM_OF_UE_TYPES> cumulativeNumberOfUEsPerMode = {
    numberOfUEsPerMode[0], numberOfUEsPerMode[0] + numberOfUEsPerMode[1],
    numberOfUEsPerMode[0] + numberOfUEsPerMode[1] + numberOfUEsPerMode[2]};
array<float, UeType::NUM_OF_UE_TYPES> timeCoeffArr = {3.0, 1.0, 0.5};
array<float, UeType::NUM_OF_UE_TYPES> rbCoeffArr = {1.0, 2.0, 1.0};
array<float, UeType::NUM_OF_UE_TYPES> energyCoeffArr = {1.0, 1.0, 3.0};

const uint32_t NUM_OF_UE =
    cumulativeNumberOfUEsPerMode[UeType::NUM_OF_UE_TYPES - 1];