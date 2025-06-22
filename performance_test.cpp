#include "AntColonyOptimization.hpp"
#include "CommonFuncs.hpp"
#include "Constants.hpp"
#include "HarmonySearch.hpp"
#include "Modes.hpp"
#include "Ram.hpp"
#include "RandomUtils.hpp"
#include "Ue.hpp"
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <string>

using namespace std;
using namespace std::chrono;

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
    uint32_t iterations;

    // Harmony Search specific
    uint32_t hms;
    float hmcr;
    float par;

    // Ant Colony specific
    uint32_t numAnts;
    float pheroCoeff;
    float heurCoeff;
    float rho;
    float q;

    // Vector to store fitness values for each iteration
    vector<float> iterationFitness;
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

// Function to generate UE list with random seed
vector<UE> generateUEList(unsigned seed) {
    vector<UE> ue_list;
    srand(seed); // Set random seed

    // Add URLLC UEs
    for (uint32_t i = 0; i < numberOfUEsPerMode[UeType::URLLC]; ++i) {
        ue_list.push_back({UeType::URLLC, static_cast<uint32_t>(getRandomInt(10, 30))});
    }

    // Add eMBB UEs
    for (uint32_t i = 0; i < numberOfUEsPerMode[UeType::eMBB]; ++i) {
        ue_list.push_back({UeType::eMBB, static_cast<uint32_t>(getRandomInt(50, 200))});
    }

    // Add mMTC UEs
    for (uint32_t i = 0; i < numberOfUEsPerMode[UeType::mMTC]; ++i) {
        ue_list.push_back({UeType::mMTC, static_cast<uint32_t>(getRandomInt(5, 20))});
    }

    return ue_list;
}

// Function to run Harmony Search with different parameters
TestResult runHarmonySearchTest(const vector<UE>& ue_list, uint32_t iterations,
                               uint32_t hms, float hmcr, float par, unsigned seed) {
    TestResult result;
    srand(seed); // Set random seed for this test

    // Store parameters in result
    result.iterations = iterations;
    result.hms = hms;
    result.hmcr = hmcr;
    result.par = par;

    // Start timing
    auto start = high_resolution_clock::now();

    // Call the overloaded harmonySearch function with parameters
    vector<uint32_t> solution = harmonySearch(ue_list, iterations, hms, hmcr, par, result.iterationFitness);

    // End timing
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);
    result.computationTime = duration.count();

    // Calculate fitness
    auto fitness = calculateFitness(ue_list, solution);
    result.bestFitness = fitness.totalCost;

    // Calculate average metrics per UE type
    result.avgLatencyURLLC = fitness.totalTime[UeType::URLLC] / numberOfUEsPerMode[UeType::URLLC];
    result.avgEnergyURLLC = fitness.totalEnergy[UeType::URLLC] / numberOfUEsPerMode[UeType::URLLC];
    result.avgLatencyEMBB = fitness.totalTime[UeType::eMBB] / numberOfUEsPerMode[UeType::eMBB];
    result.avgEnergyEMBB = fitness.totalEnergy[UeType::eMBB] / numberOfUEsPerMode[UeType::eMBB];
    result.avgLatencyMMTC = fitness.totalTime[UeType::mMTC] / numberOfUEsPerMode[UeType::mMTC];
    result.avgEnergyMMTC = fitness.totalEnergy[UeType::mMTC] / numberOfUEsPerMode[UeType::mMTC];

    return result;
}

// Function to run Ant Colony with different parameters
TestResult runAntColonyTest(const vector<UE>& ue_list, uint32_t iterations,
                            uint32_t numAnts, float pheroCoeff, float heurCoeff,
                            float rho, float q, unsigned seed) {
    TestResult result;
    srand(seed); // Set random seed for this test

    // Store parameters in result
    result.iterations = iterations;
    result.numAnts = numAnts;
    result.pheroCoeff = pheroCoeff;
    result.heurCoeff = heurCoeff;
    result.rho = rho;
    result.q = q;

    // Start timing
    auto start = high_resolution_clock::now();

    // Call the overloaded antColonyOptimization function with parameters
    vector<uint32_t> solution = antColonyOptimization(ue_list, iterations, numAnts,
                                                     pheroCoeff, heurCoeff, rho, q,
                                                     result.iterationFitness);

    // End timing
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);
    result.computationTime = duration.count();

    // Calculate fitness
    auto fitness = calculateFitness(ue_list, solution);
    result.bestFitness = fitness.totalCost;

    // Calculate average metrics per UE type
    result.avgLatencyURLLC = fitness.totalTime[UeType::URLLC] / numberOfUEsPerMode[UeType::URLLC];
    result.avgEnergyURLLC = fitness.totalEnergy[UeType::URLLC] / numberOfUEsPerMode[UeType::URLLC];
    result.avgLatencyEMBB = fitness.totalTime[UeType::eMBB] / numberOfUEsPerMode[UeType::eMBB];
    result.avgEnergyEMBB = fitness.totalEnergy[UeType::eMBB] / numberOfUEsPerMode[UeType::eMBB];
    result.avgLatencyMMTC = fitness.totalTime[UeType::mMTC] / numberOfUEsPerMode[UeType::mMTC];
    result.avgEnergyMMTC = fitness.totalEnergy[UeType::mMTC] / numberOfUEsPerMode[UeType::mMTC];

    return result;
}

// Save iteration fitness values to CSV
void saveIterationFitnessToCSV(const vector<vector<TestResult>>& allResults,
                              const string& algorithm) {
    for (size_t paramSet = 0; paramSet < allResults.size(); ++paramSet) {
        string filename = algorithm + "_fitness_iterations_paramset_" +
                         to_string(paramSet + 1) + ".csv";
        ofstream file(filename);

        // Write header with retest columns
        file << "Iteration";
        for (size_t r = 0; r < allResults[paramSet].size(); ++r) {
            file << ",Retest" << (r + 1);
        }
        file << "\n";

        // Get the maximum number of iterations
        size_t maxIterations = 0;
        for (const auto& result : allResults[paramSet]) {
            maxIterations = max(maxIterations, result.iterationFitness.size());
        }

        // Write data rows
        for (size_t iter = 0; iter < maxIterations; ++iter) {
            file << iter;
            for (const auto& result : allResults[paramSet]) {
                if (iter < result.iterationFitness.size()) {
                    file << "," << result.iterationFitness[iter];
                } else {
                    file << ",";  // Empty cell if this retest has fewer iterations
                }
            }
            file << "\n";
        }

        file.close();
        cout << "Iteration fitness values for " << algorithm
             << " parameter set " << (paramSet + 1)
             << " saved to " << filename << endl;
    }
}

// Save aggregated harmony search results to CSV
void saveAggregatedHarmonyResultsToCSV(const vector<AggregatedResult>& results,
                                      const vector<vector<TestResult>>& allResults) {
    ofstream file("harmony_aggregated_results.csv");
    file << "Iterations,HMS,HMCR,PAR,AvgFitness,BestFitness,WorstFitness,"
         << "AvgTime(ms),BestTime(ms),WorstTime(ms),"
         << "AvgLatencyURLLC,AvgEnergyURLLC,AvgLatencyEMBB,AvgEnergyEMBB,AvgLatencyMMTC,AvgEnergyMMTC\n";

    for (size_t i = 0; i < results.size(); ++i) {
        const auto& agg = results[i];
        const auto& firstResult = allResults[i][0];

        file << firstResult.iterations << ","
             << firstResult.hms << ","
             << firstResult.hmcr << ","
             << firstResult.par << ","
             << agg.avgFitness << ","
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

    file.close();
    cout << "Aggregated harmony search results saved to harmony_aggregated_results.csv" << endl;
}

// Save aggregated ant colony results to CSV
void saveAggregatedAntColonyResultsToCSV(const vector<AggregatedResult>& results,
                                        const vector<vector<TestResult>>& allResults) {
    ofstream file("ant_colony_aggregated_results.csv");
    file << "Iterations,NumAnts,PheroCoeff,HeurCoeff,Rho,Q,"
         << "AvgFitness,BestFitness,WorstFitness,"
         << "AvgTime(ms),BestTime(ms),WorstTime(ms),"
         << "AvgLatencyURLLC,AvgEnergyURLLC,AvgLatencyEMBB,AvgEnergyEMBB,AvgLatencyMMTC,AvgEnergyMMTC\n";

    for (size_t i = 0; i < results.size(); ++i) {
        const auto& agg = results[i];
        const auto& firstResult = allResults[i][0];

        file << firstResult.iterations << ","
             << firstResult.numAnts << ","
             << firstResult.pheroCoeff << ","
             << firstResult.heurCoeff << ","
             << firstResult.rho << ","
             << firstResult.q << ","
             << agg.avgFitness << ","
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

    file.close();
    cout << "Aggregated ant colony results saved to ant_colony_aggregated_results.csv" << endl;
}

// Save comparison results to CSV
void saveComparisonResultsToCSV(const vector<AggregatedResult>& harmonyAggResults,
                               const vector<AggregatedResult>& antAggResults,
                               const vector<vector<TestResult>>& harmonyAllResults,
                               const vector<vector<TestResult>>& antAllResults) {
    ofstream file("algorithm_comparison.csv");
    file << "Algorithm,ParamSet,AvgFitness,BestFitness,WorstFitness,AvgTime(ms),BestTime(ms),WorstTime(ms)\n";

    // Save Harmony results
    for (size_t i = 0; i < harmonyAggResults.size(); ++i) {
        const auto& agg = harmonyAggResults[i];
        file << "Harmony," << i << ","
             << agg.avgFitness << "," << agg.bestFitness << "," << agg.worstFitness << ","
             << agg.avgTime << "," << agg.bestTime << "," << agg.worstTime << "\n";
    }

    // Save Ant Colony results
    for (size_t i = 0; i < antAggResults.size(); ++i) {
        const auto& agg = antAggResults[i];
        file << "AntColony," << i << ","
             << agg.avgFitness << "," << agg.bestFitness << "," << agg.worstFitness << ","
             << agg.avgTime << "," << agg.bestTime << "," << agg.worstTime << "\n";
    }

    file.close();
    cout << "Algorithm comparison saved to algorithm_comparison.csv" << endl;
}

// Aggregate test results
AggregatedResult aggregateResults(const vector<TestResult>& results) {
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

    for (const auto& res : results) {
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

int main() {
    cout << "Running performance tests..." << endl;

    // Number of test cases and retests
    const int NUM_TESTS = 3;
    const int NUM_RETEST = 10;

    // Random device for seed generation
    random_device rd;
    vector<unsigned> seeds;
    for (int i = 0; i < NUM_RETEST; ++i) {
        seeds.push_back(rd());
    }

    // Harmony Search parameter variations
    vector<uint32_t> harmonyIterations = {10, 20, 40};
    vector<uint32_t> hmsList = {5, 10, 15};
    vector<float> hmcrList = {0.75, 0.85, 0.95};
    vector<float> parList = {0.05, 0.2, 0.4};

    // Ant Colony parameter variations
    vector<uint32_t> antIterations = {10, 20, 40};
    vector<uint32_t> numAntsList = {5, 10, 20};
    vector<float> pheroCoeffList = {0.8, 1.2, 1.6};
    vector<float> heurCoeffList = {1.5, 2.5, 3.5};
    vector<float> rhoList = {0.1, 0.15, 0.25};
    vector<float> qList = {75.0, 125.0, 175.0};

    // Results storage
    vector<vector<TestResult>> harmonyAllResults;
    vector<vector<TestResult>> antColonyAllResults;
    vector<AggregatedResult> harmonyAggResults;
    vector<AggregatedResult> antColonyAggResults;

    // Run Harmony Search tests
    for (int i = 0; i < NUM_TESTS; ++i) {
        cout << "Harmony Search Parameter Set " << (i+1) << " of " << NUM_TESTS << ":" << endl;
        vector<TestResult> harmonyResults;

        for (int r = 0; r < NUM_RETEST; ++r) {
            cout << "  Retest " << (r+1) << " of " << NUM_RETEST << endl;
            vector<UE> ue_list = generateUEList(seeds[r]);

            TestResult result = runHarmonySearchTest(
                ue_list, harmonyIterations[i], hmsList[i], hmcrList[i], parList[i], seeds[r]
            );
            harmonyResults.push_back(result);

            cout << "    Fitness: " << result.bestFitness
                 << ", Time: " << result.computationTime << " ms" << endl;
        }

        harmonyAllResults.push_back(harmonyResults);
        harmonyAggResults.push_back(aggregateResults(harmonyResults));

        const auto& agg = harmonyAggResults.back();
        cout << "  Avg Fitness: " << agg.avgFitness
             << ", Best: " << agg.bestFitness
             << ", Worst: " << agg.worstFitness
             << ", Avg Time: " << agg.avgTime << " ms" << endl << endl;
    }

    // Run Ant Colony tests
    for (int i = 0; i < NUM_TESTS; ++i) {
        cout << "Ant Colony Parameter Set " << (i+1) << " of " << NUM_TESTS << ":" << endl;
        vector<TestResult> antResults;

        for (int r = 0; r < NUM_RETEST; ++r) {
            cout << "  Retest " << (r+1) << " of " << NUM_RETEST << endl;
            vector<UE> ue_list = generateUEList(seeds[r]);

            TestResult result = runAntColonyTest(
                ue_list, antIterations[i], numAntsList[i], pheroCoeffList[i],
                heurCoeffList[i], rhoList[i], qList[i], seeds[r]
            );
            antResults.push_back(result);

            cout << "    Fitness: " << result.bestFitness
                 << ", Time: " << result.computationTime << " ms" << endl;
        }

        antColonyAllResults.push_back(antResults);
        antColonyAggResults.push_back(aggregateResults(antResults));

        const auto& agg = antColonyAggResults.back();
        cout << "  Avg Fitness: " << agg.avgFitness
             << ", Best: " << agg.bestFitness
             << ", Worst: " << agg.worstFitness
             << ", Avg Time: " << agg.avgTime << " ms" << endl << endl;
    }

    // Save iteration fitness values to CSV files
    saveIterationFitnessToCSV(harmonyAllResults, "harmony");
    saveIterationFitnessToCSV(antColonyAllResults, "ant_colony");

    // Save aggregated results
    saveAggregatedHarmonyResultsToCSV(harmonyAggResults, harmonyAllResults);
    saveAggregatedAntColonyResultsToCSV(antColonyAggResults, antColonyAllResults);
    saveComparisonResultsToCSV(harmonyAggResults, antColonyAggResults,
                             harmonyAllResults, antColonyAllResults);

    cout << "All tests completed successfully!" << endl;

    return 0;
}