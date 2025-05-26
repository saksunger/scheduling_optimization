#include "Modes.hpp"
#include <vector>

using namespace std;

vector<vector<float>> urllcModes = {
    {2.0, 15.0, 9.0}, {5.0, 10.0, 7.0}, {8.0, 7.0, 5.0}, {12.0, 5.0, 4.0}};

vector<vector<float>> embbModes = {
    {20.0, 20.0, 9.0}, {30.0, 15.0, 7.0}, {40.0, 10.0, 5.0}};

vector<vector<float>> mmtcModes = {
    {50.0, 3.0, 1.0}, {60.0, 2.0, 0.5}, {30.0, 4.0, 2.0}};

UeModes ueModes = {urllcModes, embbModes, mmtcModes};