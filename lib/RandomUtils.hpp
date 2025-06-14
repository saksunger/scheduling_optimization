#pragma once

#include <cstdint>
#include <random>

inline std::mt19937 &globalRNG() {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  return gen;
}

int32_t getRandomInt(int32_t min, int32_t max);
float getRandomFloat(float min, float max);
bool getRandomBool();
