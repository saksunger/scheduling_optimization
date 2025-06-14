#pragma once

#include "Constants.hpp"
#include "Modes.hpp"
#include <string>

using namespace std;

void setMaxPeriodAndEqRb();
float calculateFitness(const vector<uint32_t> &solution);
void printOrderAndItsFitness(vector<uint32_t> &sol);

inline UeType getUeType(uint32_t idx) {
  UeType type =
      idx < cumulativeNumberOfUEsPerMode[UeType::URLLC]
          ? UeType::URLLC
          : (idx < cumulativeNumberOfUEsPerMode[UeType::eMBB] ? UeType::eMBB
                                                              : UeType::mMTC);
  return type;
}

inline string printUeType(UeType type) {
  if (type == UeType::URLLC) {
    return "URLLC";
  } else if (type == UeType::eMBB) {
    return "eMBB";
  } else if (type == UeType::mMTC) {
    return "mMTC";
  } else {
    return "Wrong Type!";
  }
}