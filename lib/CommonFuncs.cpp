#include "CommonFuncs.hpp"
#include "Constants.hpp"
#include "Modes.hpp"
#include <cmath>
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

float calculateFitness(const vector<uint32_t> &solution) {
  float fitness = 0.0;
  float total_eq_rb = 0.0;

  for (uint32_t idx = 0; idx < NUM_OF_UE; ++idx) {
    UeType type = getUeType(idx);

    float timeCoeff = timeCoeffArr[type];
    float rbCoeff = rbCoeffArr[type];
    float energyCoeff = energyCoeffArr[type];

    auto &mod = ueModes[type][solution[idx]];
    float time = mod[Order::Time];
    float rb = mod[Order::RB];
    float energy = mod[Order::Energy];

    float eq_rb = rb * MAX_PERIOD / time;
    total_eq_rb += eq_rb;
    fitness += timeCoeff * time + rbCoeff * rb + energyCoeff * energy;
  }

  if (total_eq_rb > MAX_EQ_RB) {
    float penalty = pow(total_eq_rb - MAX_EQ_RB, 2);
    fitness += 1000.0 * penalty;
  }

  return fitness;
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
