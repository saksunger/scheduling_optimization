#pragma once

#include <cstdint>
#include <vector>

using namespace std;

float calculateFitness(const vector<uint32_t> &solution);
vector<uint32_t> harmonySearch();