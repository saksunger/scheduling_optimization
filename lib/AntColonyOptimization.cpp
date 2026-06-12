#include "AntColonyOptimization.hpp"
#include "CommonFuncs.hpp"
#include "Constants.hpp"
#include "Modes.hpp"
#include "Ram.hpp"
#include "RandomUtils.hpp"
#include "Ue.hpp"
#include <cmath>
#include <iostream>
#include <vector>

using namespace std;

// Forward declarations for parameterized helper functions
vector<uint32_t> constructSolution(const vector<UE> &ue_list,
                                   const vector<vector<float>> &pheromone,
                                   const vector<vector<float>> &heuristic,
                                   float pheroCoeff,
                                   float heurCoeff);

void updatePheromone(vector<vector<float>> &pheromone,
                     const vector<vector<uint32_t>> &solutions,
                     const vector<float> &fitnesses,
                     float rho,
                     float q);

// Original helper functions that delegate to parameterized versions
vector<uint32_t> constructSolution(const vector<UE> &ue_list,
                                   const vector<vector<float>> &pheromone,
                                   const vector<vector<float>> &heuristic) {
  return constructSolution(ue_list, pheromone, heuristic, PHEROMONE_COEFF, HEURISTIC_COEFF);
}

void updatePheromone(vector<vector<float>> &pheromone,
                     const vector<vector<uint32_t>> &solutions,
                     const vector<float> &fitnesses) {
  updatePheromone(pheromone, solutions, fitnesses, ACO_RHO, ACO_Q);
}

// Parameterized constructSolution
vector<uint32_t> constructSolution(const vector<UE> &ue_list,
                                   const vector<vector<float>> &pheromone,
                                   const vector<vector<float>> &heuristic,
                                   float pheroCoeff,
                                   float heurCoeff) {
  vector<uint32_t> solution(TOTAL_UE);

  for (uint32_t i = 0; i < TOTAL_UE; ++i) {
    auto modSize = getModeOptionsForUe(ue_list[i]).size();

    vector<float> probs(modSize);
    float sum = 0.0;
    for (uint32_t j = 0; j < modSize; ++j) {
      probs[j] = pow(pheromone[i][j], pheroCoeff) *
                 pow(heuristic[i][j], heurCoeff);
      sum += probs[j];
    }

    float r = getRandomFloat(0.0, sum);
    float cum = 0.0;
    for (uint32_t j = 0; j < probs.size(); ++j) {
      cum += probs[j];
      if (r <= cum) {
        solution[i] = j;
        break;
      }
    }
    if (cum < r)
      solution[i] = probs.size() - 1;
  }

  return solution;
}

float getHeuristic(const Mode &m, const UeType type) {
  return 1.0 / (heuristicLatencyWeight[type] * m.period +
                heuristicEnergyWeight[type] * m.energy + 1e-6f);
}

// Parameterized updatePheromone
void updatePheromone(vector<vector<float>> &pheromone,
                     const vector<vector<uint32_t>> &solutions,
                     const vector<float> &fitnesses,
                     float rho,
                     float q) {
  for (auto &row : pheromone) {
    for (auto &val : row) {
      val *= (1 - rho); // evaporation
    }
  }
  for (uint32_t k = 0; k < solutions.size(); ++k) {
    float contrib = q / (1.0 + fitnesses[k]);
    for (uint32_t i = 0; i < solutions[k].size(); ++i) {
      pheromone[i][solutions[k][i]] += contrib;
    }
  }
}

// Original function delegates to parameterized version
vector<uint32_t> antColonyOptimization(const vector<UE> &ue_list) {
  return antColonyOptimization(ue_list, NUM_ITERATIONS, NUM_OF_ANT,
                               PHEROMONE_COEFF, HEURISTIC_COEFF, ACO_RHO, ACO_Q);
}

// Parameterized version
vector<uint32_t> antColonyOptimization(const vector<UE> &ue_list,
                                       uint32_t iterations,
                                       uint32_t numAnts,
                                       float pheroCoeff,
                                       float heurCoeff,
                                       float rho,
                                       float q) {
  // Create a dummy vector for iteration fitness that won't be used
  vector<float> dummyIterationFitness;
  return antColonyOptimization(ue_list, iterations, numAnts, pheroCoeff,
                              heurCoeff, rho, q, dummyIterationFitness);
}

// New version that captures iteration fitness values
vector<uint32_t> antColonyOptimization(const vector<UE> &ue_list,
                                       uint32_t iterations,
                                       uint32_t numAnts,
                                       float pheroCoeff,
                                       float heurCoeff,
                                       float rho,
                                       float q,
                                       vector<float> &iterationFitness) {
  // Clear the iteration fitness vector to ensure it's empty
  iterationFitness.clear();

  vector<vector<float>> pheromone(TOTAL_UE, vector<float>{});
  vector<vector<float>> heuristic(TOTAL_UE, vector<float>{});
  for (uint32_t i = 0; i < TOTAL_UE; ++i) {
    auto modSize = getModeOptionsForUe(ue_list[i]).size();
    pheromone[i] = vector<float>(modSize, 1.0);
    heuristic[i].resize(modSize);
    for (uint32_t j = 0; j < modSize; ++j) {
      const Mode &m = getSelectedMode(ue_list[i], j);
      heuristic[i][j] = getHeuristic(m, getUeType(ue_list[i]));
    }
  }

  vector<uint32_t> bestSolution;
  float bestFitness = numeric_limits<float>::max();

  // Add initial fitness value (infinite since no solution yet)
  iterationFitness.push_back(bestFitness);

  for (uint32_t iter = 0; iter < iterations; ++iter) {
    vector<vector<uint32_t>> allSolutions;
    vector<float> allFitnesses;
    for (uint32_t ant = 0; ant < numAnts; ++ant) {
      vector<uint32_t> solution =
          constructSolution(ue_list, pheromone, heuristic, pheroCoeff, heurCoeff);
      auto fitness = calculateFitness(ue_list, solution);
      if (!isfinite(fitness.totalCost)) {
        continue;
      }
      if (fitness.totalCost < bestFitness) {
        bestFitness = fitness.totalCost;
        bestSolution = solution;
      }
      allSolutions.push_back(solution);
      allFitnesses.push_back(fitness.totalCost);
    }
    updatePheromone(pheromone, allSolutions, allFitnesses, rho, q);
    for (uint32_t i = 0; i < bestSolution.size(); ++i) {
      pheromone[i][bestSolution[i]] += q / (1.0 + bestFitness);
    }

    // Record the best fitness for this iteration
    if (isfinite(bestFitness)) {
      iterationFitness.push_back(bestFitness);
    } else {
      iterationFitness.push_back(numeric_limits<float>::max());
    }

    if (iter % 10 == 0) {
      cout << "Iteration " << iter << " Best Fitness: " << bestFitness << endl;
    }
  }

  return bestSolution;
}

// Budget-based version: identical search logic, but terminated by an exact
// FFE budget so that different algorithms can be compared under equal
// budgets. The ant loop stops mid-iteration when the budget runs out; the
// pheromone update then uses the partial ant set.
vector<uint32_t> antColonyOptimizationBudget(const vector<UE> &ue_list,
                                             uint32_t numAnts,
                                             float pheroCoeff,
                                             float heurCoeff,
                                             float rho,
                                             float q,
                                             uint32_t maxEvals,
                                             vector<float> &ffeBest) {
  resetEvalCounter();
  ffeBest.clear();

  auto recordProgress = [&](float best) {
    uint64_t upTo = min<uint64_t>(getEvalCount(), maxEvals);
    while (ffeBest.size() < upTo)
      ffeBest.push_back(best);
  };

  vector<vector<float>> pheromone(TOTAL_UE, vector<float>{});
  vector<vector<float>> heuristic(TOTAL_UE, vector<float>{});
  for (uint32_t i = 0; i < TOTAL_UE; ++i) {
    auto modSize = getModeOptionsForUe(ue_list[i]).size();
    pheromone[i] = vector<float>(modSize, 1.0);
    heuristic[i].resize(modSize);
    for (uint32_t j = 0; j < modSize; ++j) {
      const Mode &m = getSelectedMode(ue_list[i], j);
      heuristic[i][j] = getHeuristic(m, getUeType(ue_list[i]));
    }
  }

  vector<uint32_t> bestSolution;
  float bestFitness = numeric_limits<float>::max();

  while (getEvalCount() < maxEvals) {
    vector<vector<uint32_t>> allSolutions;
    vector<float> allFitnesses;
    for (uint32_t ant = 0; ant < numAnts && getEvalCount() < maxEvals; ++ant) {
      vector<uint32_t> solution =
          constructSolution(ue_list, pheromone, heuristic, pheroCoeff, heurCoeff);
      auto fitness = calculateFitness(ue_list, solution);
      if (!isfinite(fitness.totalCost)) {
        recordProgress(bestFitness);
        continue;
      }
      if (fitness.totalCost < bestFitness) {
        bestFitness = fitness.totalCost;
        bestSolution = solution;
      }
      allSolutions.push_back(solution);
      allFitnesses.push_back(fitness.totalCost);
      recordProgress(bestFitness);
    }
    updatePheromone(pheromone, allSolutions, allFitnesses, rho, q);
    for (uint32_t i = 0; i < bestSolution.size(); ++i) {
      pheromone[i][bestSolution[i]] += q / (1.0 + bestFitness);
    }
  }

  while (ffeBest.size() < maxEvals)
    ffeBest.push_back(bestFitness);

  return bestSolution;
}