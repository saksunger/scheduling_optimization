#include "HarmonySearch.hpp"
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

// Local Search operator (hill-climbing): performs a limited number of
// improvement steps on a solution. Each step picks a random UE and tries every
// alternative mode for it, keeping the move that gives the lowest cost; a move
// is accepted only if it strictly improves the cost (otherwise the UE is left
// unchanged). This refines promising harmonies that crossover/pitch-adjustment
// alone would only reach slowly.
float localSearch(const vector<UE> &ue_list, vector<uint32_t> &solution,
                  float currentCost, uint32_t steps) {
  for (uint32_t s = 0; s < steps; ++s) {
    uint32_t idx = getRandomInt(0, TOTAL_UE - 1);
    uint32_t modeCount = getModeOptionsForUe(ue_list[idx]).size();
    uint32_t bestMode = solution[idx];
    float bestCost = currentCost;
    for (uint32_t m = 0; m < modeCount; ++m) {
      if (m == solution[idx])
        continue;
      uint32_t prev = solution[idx];
      solution[idx] = m;
      float cost = calculateFitness(ue_list, solution).totalCost;
      solution[idx] = prev;
      if (isfinite(cost) && cost < bestCost) {
        bestCost = cost;
        bestMode = m;
      }
    }
    solution[idx] = bestMode; // accept best neighbor (may be unchanged)
    currentCost = bestCost;
  }
  return currentCost;
}

// Original function delegates to parameterized version with default values
vector<uint32_t> harmonySearch(const vector<UE> &ue_list) {
  return harmonySearch(ue_list, NUM_ITERATIONS, HMS, HMCR, PAR, LSR);
}

// Parameterized version
vector<uint32_t> harmonySearch(const vector<UE> &ue_list,
                               uint32_t iterations,
                               uint32_t hms,
                               float hmcr,
                               float par,
                               float lsr) {
  // Create a dummy vector for iteration fitness that won't be used
  vector<float> dummyIterationFitness;
  return harmonySearch(ue_list, iterations, hms, hmcr, par, lsr, dummyIterationFitness);
}

// New version that captures iteration fitness values
vector<uint32_t> harmonySearch(const vector<UE> &ue_list,
                               uint32_t iterations,
                               uint32_t hms,
                               float hmcr,
                               float par,
                               float lsr,
                               vector<float> &iterationFitness) {
  vector<vector<uint32_t>> harmonyMemory;
  vector<float> fitnessMemory;

  // Clear the iteration fitness vector to ensure it's empty
  iterationFitness.clear();

  for (uint32_t i = 0; i < hms; ++i) {
    auto sol = generateRandomSolution(ue_list);
    auto fit = calculateFitness(ue_list, sol);
    harmonyMemory.push_back(sol);
    fitnessMemory.push_back(fit.totalCost);
  }

  // Get the initial best fitness
  float initialBestFitness = *min_element(fitnessMemory.begin(), fitnessMemory.end());
  iterationFitness.push_back(initialBestFitness);

  for (uint32_t iter = 0; iter < iterations; ++iter) {
    vector<uint32_t> newSolution;

    for (uint32_t idx = 0; idx < TOTAL_UE; ++idx) {
      auto prob = getRandomFloat(0.0, 1.0);
      if (prob < hmcr) {
        auto hmIdx = getRandomInt(0, hms - 1);
        if (getRandomFloat(0.0, 1.0) < par) {
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
    float newCost = newFit.totalCost;

    // Local Search operator: with probability lsr, refine the freshly improvised
    // harmony with a few hill-climbing steps before considering it for memory.
    if (isfinite(newCost) && getRandomFloat(0.0, 1.0) < lsr) {
      newCost = localSearch(ue_list, newSolution, newCost, LS_STEPS);
    }

    auto maxFit = max_element(fitnessMemory.begin(), fitnessMemory.end());
    if (isfinite(newCost) && newCost < *maxFit) {
      uint32_t hmIdx = distance(fitnessMemory.begin(), maxFit);
      harmonyMemory[hmIdx] = newSolution;
      fitnessMemory[hmIdx] = newCost;
    }

    // Get the current best fitness
    float bestFitness = *min_element(fitnessMemory.begin(), fitnessMemory.end());
    iterationFitness.push_back(bestFitness);

    if (iter % 10 == 0) {
      cout << "Iteration " << iter << " Best Fitness: "
           << bestFitness << endl;
    }
  }

  auto minFit = min_element(fitnessMemory.begin(), fitnessMemory.end());
  uint32_t bestIdx = distance(fitnessMemory.begin(), minFit);
  return harmonyMemory[bestIdx];
}