#include "Modes.hpp"
#include <vector>

using namespace std;

vector<Mode> urllcModes = {{1, 2, 4}, {2, 3, 3}, {5, 4, 2}};

vector<Mode> embbModes = {{10, 10, 4}, {8, 12, 5}, {6, 15, 8}};

vector<Mode> mmtcModes = {{40, 2, 1}, {60, 3, 1}, {100, 4, 1}};

vector<vector<Mode>> ueModes = {urllcModes, embbModes, mmtcModes};
