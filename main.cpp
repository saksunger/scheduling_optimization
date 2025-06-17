#include "AntColonyOptimization.hpp"
#include "CommonFuncs.hpp"
#include "HarmonySearch.hpp"
#include "Ram.hpp"
#include "Ue.hpp"
#include <cmath>
#include <iostream>
#include <vector>

using namespace std;

int main() {
  vector<UE> ue_list;

  auto rbURLLC = generateRBs(100, 3000, 20, 40);
  auto rbEMBB = generateRBs(250, 30000, 90, 150);
  auto rbMMTC = generateRBs(250, 12500, 30, 70);

  for (int i = 0; i < numberOfUEsPerMode[UeType::URLLC]; ++i) {
    ue_list.push_back({UeType::URLLC, static_cast<uint32_t>(rbURLLC[i])});
  }
  for (int i = 0; i < numberOfUEsPerMode[UeType::eMBB]; ++i) {
    ue_list.push_back({UeType::eMBB, static_cast<uint32_t>(rbEMBB[i])});
  }
  for (int i = 0; i < numberOfUEsPerMode[UeType::mMTC]; ++i) {
    ue_list.push_back({UeType::mMTC, static_cast<uint32_t>(rbMMTC[i])});
  }

  cout << "Running Harmony Search..." << endl;
  vector<uint32_t> best_hs = harmonySearch(ue_list);
  printFitness(ue_list, best_hs);

  cout << "Running Ant Colony Optimization..." << endl;
  vector<uint32_t> best_aco = antColonyOptimization(ue_list);
  printFitness(ue_list, best_aco);

  return 0;
}
