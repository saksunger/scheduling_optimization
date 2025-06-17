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

vector<uint32_t> constructSolution(const vector<UE> &ue_list,
                                   const vector<vector<float>> &pheromone,
                                   const vector<vector<float>> &heuristic) {
  vector<uint32_t> solution(TOTAL_UE);

  for (uint32_t i = 0; i < TOTAL_UE; ++i) {
    auto modSize = getModeOptionsForUe(ue_list[i]).size();

    vector<float> probs(modSize);
    float sum = 0.0;
    for (int j = 0; j < modSize; ++j) {
      probs[j] = pow(pheromone[i][j], PHEROMONE_COEFF) *
                 pow(heuristic[i][j], HEURISTIC_COEFF);
      sum += probs[j];
    }

    float r = getRandomFloat(0.0, sum);
    float cum = 0.0;
    for (int j = 0; j < probs.size(); ++j) {
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

void updatePheromone(vector<vector<float>> &pheromone,
                     const vector<vector<uint32_t>> &solutions,
                     const vector<float> &fitnesses) {
  for (auto &row : pheromone) {
    for (auto &val : row) {
      val *= (1 - ACO_RHO); // evaporation
    }
  }
  for (uint32_t k = 0; k < solutions.size(); ++k) {
    float contrib = ACO_Q / (1.0 + fitnesses[k]);
    for (uint32_t i = 0; i < solutions[k].size(); ++i) {
      pheromone[i][solutions[k][i]] += contrib;
    }
  }
}

vector<uint32_t> antColonyOptimization(const vector<UE> &ue_list) {
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

  for (uint32_t iter = 0; iter < NUM_ITERATIONS; ++iter) {
    vector<vector<uint32_t>> allSolutions;
    vector<float> allFitnesses;
    for (uint32_t ant = 0; ant < NUM_OF_ANT; ++ant) {
      vector<uint32_t> solution =
          constructSolution(ue_list, pheromone, heuristic);
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
    updatePheromone(pheromone, allSolutions, allFitnesses);
    for (int i = 0; i < bestSolution.size(); ++i) {
      pheromone[i][bestSolution[i]] += ACO_Q / (1.0 + bestFitness);
    }
    if (iter % 10 == 0) {
      cout << "Iteration " << iter << " Best Fitness: " << bestFitness << endl;
    }
  }

  return bestSolution;
}
