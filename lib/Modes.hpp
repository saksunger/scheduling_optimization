#pragma once

#include <cstdlib>
#include <vector>

using namespace std;

typedef vector<vector<vector<float>>> UeModes;
enum UeType { URLLC, eMBB, mMTC, NUM_OF_UE_TYPES };
enum Order { Time, RB, Energy, NUM_OF_ORDERS };

extern UeModes ueModes;