#pragma once

#include "Ue.hpp"
#include <cstdint>
#include <cstdlib>
#include <vector>

using namespace std;

struct Mode {
  uint32_t period;
  uint32_t rb_per_period;
  uint32_t energy;
};

extern vector<vector<Mode>> ueModes;

inline const vector<Mode> &getModeOptionsForUe(const UE &ue) {
  return ueModes[getUeType(ue)];
}

inline const Mode &getSelectedMode(const UE &ue, uint32_t index) {
  return getModeOptionsForUe(ue)[index];
}
