#include "CommonFuncs.hpp"
#include "Constants.hpp"
#include "Modes.hpp"
#include "RandomUtils.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

using namespace std;

vector<uint32_t> generateRandomSolution(const vector<UE> &ue_list) {
  vector<uint32_t> solution;
  for (uint32_t idx = 0; idx < TOTAL_UE; ++idx) {
    auto mod = getRandomInt(0, getModeOptionsForUe(ue_list[idx]).size() - 1);
    solution.push_back(mod);
  }
  return solution;
}

vector<uint32_t> harmonySearch(const vector<UE> &ue_list) {
  vector<vector<uint32_t>> harmonyMemory;
  vector<float> fitnessMemory;

  for (int i = 0; i < HMS; ++i) {
    auto sol = generateRandomSolution(ue_list);
    auto fit = calculateFitness(ue_list, sol);
    harmonyMemory.push_back(sol);
    fitnessMemory.push_back(fit.totalCost);
  }

  for (int iter = 0; iter < NUM_ITERATIONS; ++iter) {
    vector<uint32_t> newSolution;

    for (uint32_t idx = 0; idx < TOTAL_UE; ++idx) {
      auto prob = getRandomFloat(0.0, 1.0);
      if (prob < HMCR) {
        auto hmIdx = getRandomInt(0, HMS - 1);
        if (getRandomFloat(0.0, 1.0) < PAR) {
          uint32_t modSize = getModeOptionsForUe(ue_list[idx]).size();
          uint32_t cur = harmonyMemory[hmIdx][idx];
          uint32_t adj =
              (cur + (getRandomInt(0, 1) ? 1 : -1) + modSize) % modSize;
          newSolution.push_back(adj);
        } else {
          newSolution.push_back(harmonyMemory[hmIdx][idx]);
        }
      } else {
        auto mod =
            getRandomInt(0, getModeOptionsForUe(ue_list[idx]).size() - 1);
        newSolution.push_back(mod);
      }
    }

    auto newFit = calculateFitness(ue_list, newSolution);

    auto maxFit = max_element(fitnessMemory.begin(), fitnessMemory.end());
    if (isfinite(newFit.totalCost) && newFit.totalCost < *maxFit) {
      uint32_t hmIdx = distance(fitnessMemory.begin(), maxFit);
      harmonyMemory[hmIdx] = newSolution;
      fitnessMemory[hmIdx] = newFit.totalCost;
    }
    if (iter % 10 == 0) {
      cout << "Iteration " << iter << " Best Fitness: "
           << *min_element(fitnessMemory.begin(), fitnessMemory.end()) << endl;
    }
  }

  auto minFit = min_element(fitnessMemory.begin(), fitnessMemory.end());
  uint32_t bestIdx = distance(fitnessMemory.begin(), minFit);
  return harmonyMemory[bestIdx];
}
