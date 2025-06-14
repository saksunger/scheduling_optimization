#include "AntColonyOptimization.hpp"
#include "CommonFuncs.hpp"
#include "HarmonySearch.hpp"
#include <cstdint>

using namespace std;

int32_t main() {
  setMaxPeriodAndEqRb();
  auto solHarmony = harmonySearch();
  auto solAco = antColonyOptimization();
  printOrderAndItsFitness(solHarmony);
  printOrderAndItsFitness(solAco);
  return 0;
}