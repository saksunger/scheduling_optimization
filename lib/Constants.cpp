#include "Constants.hpp"
#include "Ram.hpp"
#include <array>
#include <cstdint>
#include <numeric>

using namespace std;

const uint32_t TOTAL_UE =
    accumulate(numberOfUEsPerMode.cbegin(), numberOfUEsPerMode.cend(), 0);
