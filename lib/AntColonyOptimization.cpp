#include "CommonFuncs.hpp"
#include "Constants.hpp"
#include "Modes.hpp"
#include "RandomUtils.hpp"
#include <cmath>
#include <vector>

using namespace std;

vector<uint32_t> constructSolution(const vector<vector<float>> &pheromone,
                                   const vector<vector<float>> &heuristic) {
  vector<uint32_t> solution(NUM_OF_UE);

  for (uint32_t i = 0; i < NUM_OF_UE; ++i) {
    UeType type = getUeType(i);
    auto modSize = ueModes[type].size();

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
  }

  return solution;
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

vector<uint32_t> antColonyOptimization() {
  vector<vector<float>> pheromone(NUM_OF_UE, vector<float>{});
  vector<vector<float>> heuristic(NUM_OF_UE, vector<float>{});
  for (uint32_t i = 0; i < NUM_OF_UE; ++i) {
    pheromone[i] = vector<float>(ueModes[getUeType(i)].size(), 1.0);
    heuristic[i] = vector<float>(ueModes[getUeType(i)].size(), 1.0);
  }

  vector<uint32_t> bestSolution;
  float bestFitness = numeric_limits<float>::max();

  for (uint32_t iter = 0; iter < NUM_ITERATIONS; ++iter) {
    vector<vector<uint32_t>> allSolutions;
    vector<float> allFitnesses;
    for (uint32_t ant = 0; ant < NUM_OF_ANT; ++ant) {
      vector<uint32_t> solution = constructSolution(pheromone, heuristic);
      float fitness = calculateFitness(solution);
      if (fitness < bestFitness) {
        bestFitness = fitness;
        bestSolution = solution;
      }
      allSolutions.push_back(solution);
      allFitnesses.push_back(fitness);
    }
    updatePheromone(pheromone, allSolutions, allFitnesses);
  }

  return bestSolution;
}
