#include "CommonFuncs.hpp"
#include "HarmonySearch.hpp"
#include <cstdint>

using namespace std;

int32_t main() {
  setMaxPeriodAndEqRb();
  auto sol = harmonySearch();
  printOrderAndItsFitness(sol);
  return 0;
}