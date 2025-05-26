#include "RandomUtils.hpp"
#include <cstdint>

int32_t getRandomInt(int32_t min, int32_t max) {
  std::uniform_int_distribution<> dist(min, max);
  return dist(globalRNG());
}

double getRandomDouble(double min, double max) {
  std::uniform_real_distribution<> dist(min, max);
  return dist(globalRNG());
}

bool getRandomBool() {
  std::uniform_int_distribution<> dist(0, 1);
  return static_cast<bool>(dist(globalRNG()));
}
