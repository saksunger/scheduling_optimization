#include "CommonFuncs.hpp"
#include "Constants.hpp"
#include "Modes.hpp"
#include "Ram.hpp"
#include "RandomUtils.hpp"
#include "Ue.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>

using namespace std;

vector<int32_t> generateRBs(int32_t numUEs, int32_t totalTargetRB,
                            int32_t minRB, int32_t maxRB) {
  vector<int32_t> rbNeeds(numUEs);

  for (uint32_t i = 0; i < numUEs; ++i)
    rbNeeds[i] = getRandomInt(minRB, maxRB);

  int32_t currentSum = accumulate(rbNeeds.begin(), rbNeeds.end(), 0);

  auto scale = static_cast<float>(totalTargetRB) / currentSum;
  for (auto &val : rbNeeds)
    val = max(minRB, static_cast<int32_t>(val * scale));

  int32_t correctedSum = accumulate(rbNeeds.begin(), rbNeeds.end(), 0);
  int32_t diff = totalTargetRB - correctedSum;
  for (uint32_t i = 0; i < abs(diff); ++i)
    rbNeeds[i % numUEs] += (diff > 0) ? 1 : -1;

  return rbNeeds;
}

bool canSchedule(const UE &ue, const Mode &selected_mode, uint32_t start) {
  uint32_t periods = getRepetitionCount(ue, selected_mode);
  for (uint32_t p = 0; p < periods; ++p) {
    uint32_t t = start + p * selected_mode.period;
    if (t >= MAX_TIME)
      return false;

    uint32_t free_rb = 0;
    for (uint32_t r = 0; r < NUM_OF_RB; ++r) {
      if (!timeline[t][r])
        ++free_rb;
    }
    if (free_rb < selected_mode.rb_per_period)
      return false;
  }
  return true;
}

void scheduleUE(const UE &ue, const Mode &selected_mode, uint32_t start) {
  uint32_t periods = getRepetitionCount(ue, selected_mode);
  for (uint32_t p = 0; p < periods; ++p) {
    uint32_t t = start + p * selected_mode.period;
    if (t >= MAX_TIME)
      break;
    uint32_t assigned = 0;
    for (uint32_t r = 0;
         r < NUM_OF_RB && assigned < selected_mode.rb_per_period; ++r) {
      if (!timeline[t][r]) {
        timeline[t][r] = true;
        ++assigned;
      }
    }
  }
}

FitnessResult calculateFitness(const vector<UE> &ue_list,
                               const vector<uint32_t> &solution) {
  resetTimeline();
  FitnessResult result{0.0, {}, {}};

  for (uint32_t i = 0; i < TOTAL_UE; ++i) {
    const UE &ue = ue_list[i];
    const auto &selected_mode = getSelectedMode(ue_list[i], solution[i]);
    uint32_t repetitions = getRepetitionCount(ue, selected_mode);

    bool scheduled = false;
    for (uint32_t t = 0; t < MAX_TIME; ++t) {
      if (canSchedule(ue, selected_mode, t)) {
        scheduleUE(ue, selected_mode, t);
        auto finish_time = t + (repetitions - 1) * selected_mode.period;
        auto energy = repetitions * selected_mode.energy;

        result.totalCost += timeCoeffArr[getUeType(ue)] * finish_time +
                            energyCoeffArr[getUeType(ue)] * energy;
        result.totalTime[getUeType(ue)] += finish_time;
        result.totalEnergy[getUeType(ue)] += energy;

        scheduled = true;
        break;
      }
    }
    if (!scheduled)
      return {numeric_limits<float>::max(), {}, {}};
  }
  return result;
}

void printFitness(const vector<UE> &ue_list, const vector<uint32_t> &solution) {
  auto fitness = calculateFitness(ue_list, solution);
  cout << "The fitness value is " << fitness.totalCost << endl;
  for (uint32_t i = 0; i < UeType::NUM_OF_UE_TYPES; ++i) {
    cout << "Mean latency for UE type " << printUeType(static_cast<UeType>(i))
         << ":" << fitness.totalTime[i] / numberOfUEsPerMode[i] << endl;
    cout << "Mean energy for UE type " << printUeType(static_cast<UeType>(i))
         << ":" << fitness.totalEnergy[i] / numberOfUEsPerMode[i] << endl;
  }
}
