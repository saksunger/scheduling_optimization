#include "CommonFuncs.hpp"
#include "Constants.hpp"
#include "HarmonySearch.hpp"
#include "Modes.hpp"
#include <iostream>

using namespace std;

void setMaxPeriodAndEqRb() {
  float maxPeriod = 0;
  for (auto outer : ueModes) {
    for (auto inner : outer) {
      maxPeriod = maxPeriod > inner[0] ? maxPeriod : inner[0];
    }
  }
  MAX_PERIOD = maxPeriod;
  MAX_EQ_RB = MAX_PERIOD * NUM_OF_RB;
}

void printOrderAndItsFitness(vector<uint32_t> &sol) {
  UeType prevType = UeType::NUM_OF_UE_TYPES;
  for (uint32_t idx = 0; idx < NUM_OF_UE; ++idx) {
    UeType type = getUeType(idx);
    if (type != prevType) {
      if (prevType != UeType::NUM_OF_UE_TYPES) {
        cout << endl << endl << endl;
      }
      cout << "For UE type " << printUeType(type) << ":" << endl;
      cout << "----------------------------------------" << endl;
      prevType = type;
    }
    cout << sol[idx] << " ";
  }
  cout << endl << endl << endl;
  cout << "The fitness value is " << calculateFitness(sol) << endl;
}
