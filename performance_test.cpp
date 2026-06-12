#include "AntColonyOptimization.hpp"
#include "CommonFuncs.hpp"
#include "Constants.hpp"
#include "HarmonySearch.hpp"
#include "Modes.hpp"
#include "Ram.hpp"
#include "RandomUtils.hpp"
#include "Ue.hpp"
#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>
#include <algorithm>
#include <string>

using namespace std;
using namespace std::chrono;

// ---------------------------------------------------------------------------
// Experiment protocol:
//  - 4 fixed problem instances (saved to instances/instance_N.csv)
//  - 3 methods: Harmony Search (HS), HS + Local Search (HS+LS), Ant Colony
//  - 8 parameter configurations per method
//  - every run terminated at exactly FFE_BUDGET fitness function evaluations
//    (1 FFE = 1 calculateFitness call), so all methods get an equal budget
//  - NUM_RETEST independent runs per configuration with fixed seeds
// ---------------------------------------------------------------------------

const uint32_t FFE_BUDGET = 2000;
const int NUM_RETEST = 10;
const unsigned RUN_SEED_BASE = 9000;
const float LS_RATE = 0.3; // LSR used by the HS+LS method

// Structure to hold test results
struct TestResult {
    float bestFitness;
    double computationTime; // in milliseconds
    float avgLatencyURLLC;
    float avgEnergyURLLC;
    float avgLatencyEMBB;
    float avgEnergyEMBB;
    float avgLatencyMMTC;
    float avgEnergyMMTC;

    // Parameters used
    uint32_t budget;

    // Harmony Search specific
    uint32_t hms;
    float hmcr;
    float par;
    float lsr;

    // Ant Colony specific
    uint32_t numAnts;
    float pheroCoeff;
    float heurCoeff;
    float rho;
    float q;

    // Best-so-far fitness after each evaluation (size == budget)
    vector<float> ffeFitness;
};

// Structure for aggregated results
struct AggregatedResult {
    double avgFitness;
    float bestFitness;
    float worstFitness;
    double avgTime;
    double bestTime;
    double worstTime;

    // Average per-type metrics
    double avgLatencyURLLC;
    double avgEnergyURLLC;
    double avgLatencyEMBB;
    double avgEnergyEMBB;
    double avgLatencyMMTC;
    double avgEnergyMMTC;
};

// Problem instance definition: service mix (must sum to TOTAL_UE) plus the
// data-amount range per service type and a fixed generation seed.
struct InstanceDef {
    string name;
    array<uint32_t, UeType::NUM_OF_UE_TYPES> mix; // URLLC, eMBB, mMTC counts
    array<int32_t, UeType::NUM_OF_UE_TYPES> dataLo;
    array<int32_t, UeType::NUM_OF_UE_TYPES> dataHi;
    unsigned seed;
};

static const vector<InstanceDef> INSTANCES = {
    {"instance_1", {100, 250, 250}, {10, 50, 5},  {30, 200, 20}, 1001}, // baseline
    {"instance_2", {200, 200, 200}, {10, 50, 5},  {30, 200, 20}, 2002}, // URLLC-heavy
    {"instance_3", {60, 180, 360},  {10, 50, 10}, {30, 200, 30}, 3003}, // mMTC-heavy (IoT)
    {"instance_4", {80, 350, 170},  {10, 80, 5},  {30, 250, 20}, 4004}, // eMBB-heavy hotspot
};

// Harmony Search configurations: HS and HS+LS share the same 8 base
// configurations, only the Local Search rate differs (0 vs LS_RATE).
struct HsConfig {
    uint32_t hms;
    float hmcr;
    float par;
};

static const vector<HsConfig> HS_CONFIGS = {
    {5, 0.70, 0.10},
    {5, 0.90, 0.30},
    {10, 0.85, 0.20},
    {10, 0.95, 0.30},
    {20, 0.90, 0.10},
    {20, 0.95, 0.40},
    {30, 0.99, 0.30},
    {30, 0.80, 0.50},
};

// Ant Colony configurations
struct AcoConfig {
    uint32_t numAnts;
    float pheroCoeff;
    float heurCoeff;
    float rho;
    float q;
};

static const vector<AcoConfig> ACO_CONFIGS = {
    {5, 1.0, 2.0, 0.10, 100.0},
    {5, 2.0, 3.0, 0.30, 100.0},
    {10, 0.8, 1.5, 0.10, 75.0},
    {10, 1.2, 2.5, 0.15, 125.0},
    {20, 1.0, 3.0, 0.25, 175.0},
    {20, 1.6, 3.5, 0.25, 175.0},
    {40, 2.0, 4.5, 0.30, 225.0},
    {40, 1.0, 2.0, 0.50, 150.0},
};

// Generate the UE list of an instance deterministically from its seed.
// numberOfUEsPerMode is set here so that per-type averaging stays correct.
vector<UE> generateUEList(const InstanceDef &inst) {
    numberOfUEsPerMode = inst.mix;
    globalRNG().seed(inst.seed);

    vector<UE> ue_list;
    for (uint32_t t = 0; t < UeType::NUM_OF_UE_TYPES; ++t) {
        for (uint32_t i = 0; i < inst.mix[t]; ++i) {
            ue_list.push_back({static_cast<UeType>(t),
                               static_cast<uint32_t>(getRandomInt(inst.dataLo[t], inst.dataHi[t]))});
        }
    }
    return ue_list;
}

// Save an instance to CSV so it can be published and re-used.
void saveInstanceCSV(const InstanceDef &inst, const vector<UE> &ue_list) {
    string filename = "instances/" + inst.name + ".csv";
    ofstream file(filename);
    file << "Index,Type,Data\n";
    for (size_t i = 0; i < ue_list.size(); ++i) {
        file << i << "," << printUeType(ue_list[i]) << "," << ue_list[i].data << "\n";
    }
    file.close();
    cout << "Instance saved to " << filename << endl;
}

// Fill the per-UE-type metrics of a finished run
void fillTypeMetrics(TestResult &result, const vector<UE> &ue_list,
                     const vector<uint32_t> &solution) {
    auto fitness = calculateFitness(ue_list, solution);
    result.bestFitness = fitness.totalCost;

    result.avgLatencyURLLC = fitness.totalTime[UeType::URLLC] / numberOfUEsPerMode[UeType::URLLC];
    result.avgEnergyURLLC = fitness.totalEnergy[UeType::URLLC] / numberOfUEsPerMode[UeType::URLLC];
    result.avgLatencyEMBB = fitness.totalTime[UeType::eMBB] / numberOfUEsPerMode[UeType::eMBB];
    result.avgEnergyEMBB = fitness.totalEnergy[UeType::eMBB] / numberOfUEsPerMode[UeType::eMBB];
    result.avgLatencyMMTC = fitness.totalTime[UeType::mMTC] / numberOfUEsPerMode[UeType::mMTC];
    result.avgEnergyMMTC = fitness.totalEnergy[UeType::mMTC] / numberOfUEsPerMode[UeType::mMTC];
}

// Run Harmony Search (with or without Local Search) under the FFE budget
TestResult runHarmonySearchTest(const vector<UE> &ue_list, uint32_t budget,
                                uint32_t hms, float hmcr, float par, float lsr,
                                unsigned seed) {
    TestResult result;
    globalRNG().seed(seed);

    result.budget = budget;
    result.hms = hms;
    result.hmcr = hmcr;
    result.par = par;
    result.lsr = lsr;

    auto start = high_resolution_clock::now();
    vector<uint32_t> solution =
        harmonySearchBudget(ue_list, hms, hmcr, par, lsr, budget, result.ffeFitness);
    auto end = high_resolution_clock::now();
    result.computationTime = duration_cast<microseconds>(end - start).count() / 1000.0;

    fillTypeMetrics(result, ue_list, solution);
    return result;
}

// Run Ant Colony Optimization under the FFE budget
TestResult runAntColonyTest(const vector<UE> &ue_list, uint32_t budget,
                            uint32_t numAnts, float pheroCoeff, float heurCoeff,
                            float rho, float q, unsigned seed) {
    TestResult result;
    globalRNG().seed(seed);

    result.budget = budget;
    result.numAnts = numAnts;
    result.pheroCoeff = pheroCoeff;
    result.heurCoeff = heurCoeff;
    result.rho = rho;
    result.q = q;

    auto start = high_resolution_clock::now();
    vector<uint32_t> solution = antColonyOptimizationBudget(
        ue_list, numAnts, pheroCoeff, heurCoeff, rho, q, budget, result.ffeFitness);
    auto end = high_resolution_clock::now();
    result.computationTime = duration_cast<microseconds>(end - start).count() / 1000.0;

    fillTypeMetrics(result, ue_list, solution);
    return result;
}

// Save best-so-far fitness per FFE to CSV (FFE 1 and then every 10th FFE)
void saveFFEHistoryCSV(const vector<TestResult> &results, const string &filename) {
    ofstream file(filename);

    file << "Evaluation";
    for (size_t r = 0; r < results.size(); ++r) {
        file << ",Retest" << (r + 1);
    }
    file << "\n";

    size_t budget = results.empty() ? 0 : results[0].ffeFitness.size();
    for (size_t e = 0; e < budget; ++e) {
        if (e != 0 && (e + 1) % 10 != 0)
            continue;
        file << (e + 1);
        for (const auto &result : results) {
            file << "," << result.ffeFitness[e];
        }
        file << "\n";
    }
    file.close();
}

// Aggregate test results
AggregatedResult aggregateResults(const vector<TestResult> &results) {
    AggregatedResult agg;
    if (results.empty()) return agg;

    double totalFitness = 0.0;
    double totalTime = 0.0;
    double totalLatencyURLLC = 0.0;
    double totalEnergyURLLC = 0.0;
    double totalLatencyEMBB = 0.0;
    double totalEnergyEMBB = 0.0;
    double totalLatencyMMTC = 0.0;
    double totalEnergyMMTC = 0.0;

    agg.bestFitness = results[0].bestFitness;
    agg.worstFitness = results[0].bestFitness;
    agg.bestTime = results[0].computationTime;
    agg.worstTime = results[0].computationTime;

    for (const auto &res : results) {
        totalFitness += res.bestFitness;
        totalTime += res.computationTime;

        totalLatencyURLLC += res.avgLatencyURLLC;
        totalEnergyURLLC += res.avgEnergyURLLC;
        totalLatencyEMBB += res.avgLatencyEMBB;
        totalEnergyEMBB += res.avgEnergyEMBB;
        totalLatencyMMTC += res.avgLatencyMMTC;
        totalEnergyMMTC += res.avgEnergyMMTC;

        if (res.bestFitness < agg.bestFitness) agg.bestFitness = res.bestFitness;
        if (res.bestFitness > agg.worstFitness) agg.worstFitness = res.bestFitness;
        if (res.computationTime < agg.bestTime) agg.bestTime = res.computationTime;
        if (res.computationTime > agg.worstTime) agg.worstTime = res.computationTime;
    }

    size_t n = results.size();
    agg.avgFitness = totalFitness / n;
    agg.avgTime = totalTime / n;
    agg.avgLatencyURLLC = totalLatencyURLLC / n;
    agg.avgEnergyURLLC = totalEnergyURLLC / n;
    agg.avgLatencyEMBB = totalLatencyEMBB / n;
    agg.avgEnergyEMBB = totalEnergyEMBB / n;
    agg.avgLatencyMMTC = totalLatencyMMTC / n;
    agg.avgEnergyMMTC = totalEnergyMMTC / n;

    return agg;
}

void writeAggregateRow(ofstream &file, const AggregatedResult &agg) {
    file << agg.avgFitness << ","
         << agg.bestFitness << ","
         << agg.worstFitness << ","
         << agg.avgTime << ","
         << agg.bestTime << ","
         << agg.worstTime << ","
         << agg.avgLatencyURLLC << ","
         << agg.avgEnergyURLLC << ","
         << agg.avgLatencyEMBB << ","
         << agg.avgEnergyEMBB << ","
         << agg.avgLatencyMMTC << ","
         << agg.avgEnergyMMTC << "\n";
}

// Save aggregated harmony search results (one row per configuration)
void saveAggregatedHarmonyResultsToCSV(const vector<AggregatedResult> &results,
                                       const vector<vector<TestResult>> &allResults,
                                       const string &filename) {
    ofstream file(filename);
    file << "Config,Budget(FFE),HMS,HMCR,PAR,LSR,AvgFitness,BestFitness,WorstFitness,"
         << "AvgTime(ms),BestTime(ms),WorstTime(ms),"
         << "AvgLatencyURLLC,AvgEnergyURLLC,AvgLatencyEMBB,AvgEnergyEMBB,AvgLatencyMMTC,AvgEnergyMMTC\n";

    for (size_t i = 0; i < results.size(); ++i) {
        const auto &firstResult = allResults[i][0];
        file << (i + 1) << ","
             << firstResult.budget << ","
             << firstResult.hms << ","
             << firstResult.hmcr << ","
             << firstResult.par << ","
             << firstResult.lsr << ",";
        writeAggregateRow(file, results[i]);
    }
    file.close();
    cout << "Aggregated results saved to " << filename << endl;
}

// Save aggregated ant colony results (one row per configuration)
void saveAggregatedAntColonyResultsToCSV(const vector<AggregatedResult> &results,
                                         const vector<vector<TestResult>> &allResults,
                                         const string &filename) {
    ofstream file(filename);
    file << "Config,Budget(FFE),NumAnts,PheroCoeff,HeurCoeff,Rho,Q,"
         << "AvgFitness,BestFitness,WorstFitness,"
         << "AvgTime(ms),BestTime(ms),WorstTime(ms),"
         << "AvgLatencyURLLC,AvgEnergyURLLC,AvgLatencyEMBB,AvgEnergyEMBB,AvgLatencyMMTC,AvgEnergyMMTC\n";

    for (size_t i = 0; i < results.size(); ++i) {
        const auto &firstResult = allResults[i][0];
        file << (i + 1) << ","
             << firstResult.budget << ","
             << firstResult.numAnts << ","
             << firstResult.pheroCoeff << ","
             << firstResult.heurCoeff << ","
             << firstResult.rho << ","
             << firstResult.q << ",";
        writeAggregateRow(file, results[i]);
    }
    file.close();
    cout << "Aggregated results saved to " << filename << endl;
}

// Smoke test: generate + save every instance, check that random solutions are
// feasible and measure the cost of a single fitness evaluation.
int runSmokeTest() {
    for (const auto &inst : INSTANCES) {
        vector<UE> ue_list = generateUEList(inst);
        saveInstanceCSV(inst, ue_list);

        globalRNG().seed(12345);
        int infeasible = 0;
        const int N = 50;
        auto start = high_resolution_clock::now();
        for (int i = 0; i < N; ++i) {
            vector<uint32_t> solution;
            for (const auto &ue : ue_list) {
                solution.push_back(getRandomInt(0, getModeOptionsForUe(ue).size() - 1));
            }
            auto fit = calculateFitness(ue_list, solution);
            if (!isfinite(fit.totalCost))
                ++infeasible;
        }
        auto end = high_resolution_clock::now();
        double msPerEval = duration_cast<microseconds>(end - start).count() / 1000.0 / N;

        cout << inst.name << ": " << infeasible << "/" << N
             << " infeasible random solutions, " << msPerEval << " ms/FFE" << endl;
    }
    return 0;
}

int main(int argc, char **argv) {
    // Every instance must keep the total UE count fixed: TOTAL_UE is a
    // link-time constant used by the algorithms and the fitness function.
    for (const auto &inst : INSTANCES) {
        uint32_t sum = inst.mix[0] + inst.mix[1] + inst.mix[2];
        if (sum != TOTAL_UE) {
            cerr << "Instance " << inst.name << " has " << sum
                 << " UEs, expected " << TOTAL_UE << endl;
            return 1;
        }
    }

    filesystem::create_directories("instances");
    filesystem::create_directories("results");

    if (argc > 1 && string(argv[1]) == "--smoke") {
        return runSmokeTest();
    }

    // Optional instance index argument (1..4) to allow parallel processes
    int onlyInstance = (argc > 1) ? atoi(argv[1]) : 0;

    cout << "Running performance tests (budget = " << FFE_BUDGET
         << " FFE, " << NUM_RETEST << " retests)..." << endl;

    for (size_t instIdx = 0; instIdx < INSTANCES.size(); ++instIdx) {
        if (onlyInstance != 0 && static_cast<int>(instIdx + 1) != onlyInstance)
            continue;

        const InstanceDef &inst = INSTANCES[instIdx];
        const string instTag = "instance" + to_string(instIdx + 1);
        vector<UE> ue_list = generateUEList(inst);
        saveInstanceCSV(inst, ue_list);

        // Harmony Search without (lsr = 0) and with (lsr = LS_RATE) Local Search
        const vector<pair<string, float>> hsMethods = {
            {"harmony", 0.0f}, {"harmony_ls", LS_RATE}};

        for (const auto &method : hsMethods) {
            vector<vector<TestResult>> allResults;
            vector<AggregatedResult> aggResults;

            for (size_t c = 0; c < HS_CONFIGS.size(); ++c) {
                const auto &cfg = HS_CONFIGS[c];
                vector<TestResult> results;
                for (int r = 0; r < NUM_RETEST; ++r) {
                    results.push_back(runHarmonySearchTest(
                        ue_list, FFE_BUDGET, cfg.hms, cfg.hmcr, cfg.par,
                        method.second, RUN_SEED_BASE + r));
                }
                saveFFEHistoryCSV(results, "results/" + method.first + "_ffe_" +
                                               instTag + "_config" + to_string(c + 1) + ".csv");
                allResults.push_back(results);
                aggResults.push_back(aggregateResults(results));

                cout << inst.name << " " << method.first << " config " << (c + 1)
                     << ": avg " << aggResults.back().avgFitness
                     << ", best " << aggResults.back().bestFitness
                     << ", avg time " << aggResults.back().avgTime << " ms" << endl;
            }
            saveAggregatedHarmonyResultsToCSV(
                aggResults, allResults,
                "results/" + method.first + "_aggregated_" + instTag + ".csv");
        }

        // Ant Colony Optimization
        {
            vector<vector<TestResult>> allResults;
            vector<AggregatedResult> aggResults;

            for (size_t c = 0; c < ACO_CONFIGS.size(); ++c) {
                const auto &cfg = ACO_CONFIGS[c];
                vector<TestResult> results;
                for (int r = 0; r < NUM_RETEST; ++r) {
                    results.push_back(runAntColonyTest(
                        ue_list, FFE_BUDGET, cfg.numAnts, cfg.pheroCoeff,
                        cfg.heurCoeff, cfg.rho, cfg.q, RUN_SEED_BASE + r));
                }
                saveFFEHistoryCSV(results, "results/ant_colony_ffe_" + instTag +
                                               "_config" + to_string(c + 1) + ".csv");
                allResults.push_back(results);
                aggResults.push_back(aggregateResults(results));

                cout << inst.name << " ant_colony config " << (c + 1)
                     << ": avg " << aggResults.back().avgFitness
                     << ", best " << aggResults.back().bestFitness
                     << ", avg time " << aggResults.back().avgTime << " ms" << endl;
            }
            saveAggregatedAntColonyResultsToCSV(
                aggResults, allResults,
                "results/ant_colony_aggregated_" + instTag + ".csv");
        }
    }

    cout << "All tests completed successfully!" << endl;
    return 0;
}
