#include "CommonFuncs.hpp"
#include "Constants.hpp"
#include "Modes.hpp"
#include "RandomUtils.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

using namespace std;

vector<uint32_t> generateRandomSolution() {
  vector<uint32_t> solution;
  for (uint32_t idx = 0; idx < NUM_OF_UE; ++idx) {
    UeType type = getUeType(idx);
    auto mod = getRandomInt(0, ueModes[type].size() - 1);
    solution.push_back(mod);
  }
  return solution;
}

vector<uint32_t> harmonySearch() {
  // Initialize Harmony Memory
  vector<vector<uint32_t>> harmonyMemory;
  vector<float> fitnessMemory;

  for (int i = 0; i < HMS; ++i) {
    auto sol = generateRandomSolution();
    float fit = calculateFitness(sol);
    harmonyMemory.push_back(sol);
    fitnessMemory.push_back(fit);
    // printOrderAndItsFitness(sol);
  }

  for (int iter = 0; iter < NUM_ITERATIONS; ++iter) {
    vector<uint32_t> newSolution;

    for (uint32_t idx = 0; idx < NUM_OF_UE; ++idx) {
      auto prob = getRandomFloat(0.0, 1.0);
      if (prob < HMCR) {
        // Pick from memory
        auto hmIdx = getRandomInt(0, HMS - 1);
        newSolution.push_back(harmonyMemory[hmIdx][idx]);
      } else {
        // Random new note
        UeType type = getUeType(idx);
        auto mod = getRandomInt(0, ueModes[type].size() - 1);
        newSolution.push_back(mod);
      }
    }

    // printOrderAndItsFitness(newSolution);

    float newFit = calculateFitness(newSolution);

    // Replace worst if better
    auto maxFit = max_element(fitnessMemory.begin(), fitnessMemory.end());
    if (newFit < *maxFit) {
      uint32_t hmIdx = distance(fitnessMemory.begin(), maxFit);
      harmonyMemory[hmIdx] = newSolution;
      fitnessMemory[hmIdx] = newFit;
    }
  }

  // Output best solution
  auto minFit = min_element(fitnessMemory.begin(), fitnessMemory.end());
  uint32_t bestIdx = distance(fitnessMemory.begin(), minFit);
  return harmonyMemory[bestIdx];
}