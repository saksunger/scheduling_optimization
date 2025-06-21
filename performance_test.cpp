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
};

// Function to generate UE list
vector<UE> generateUEList() {
    vector<UE> ue_list;

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
                               uint32_t hms, float hmcr, float par) {
    TestResult result;

    // Store parameters in result
    result.iterations = iterations;
    result.hms = hms;
    result.hmcr = hmcr;
    result.par = par;

    // Start timing
    auto start = high_resolution_clock::now();

    // Call the overloaded harmonySearch function with parameters
    vector<uint32_t> solution = harmonySearch(ue_list, iterations, hms, hmcr, par);

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
                            float rho, float q) {
    TestResult result;

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
                                                     pheroCoeff, heurCoeff, rho, q);

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

// Save harmony search results to CSV
void saveHarmonyResultsToCSV(const vector<TestResult>& results) {
    ofstream file("harmony_results.csv");
    file << "Iterations,HMS,HMCR,PAR,BestFitness,ComputationTime(ms),";
    file << "AvgLatencyURLLC,AvgEnergyURLLC,AvgLatencyEMBB,AvgEnergyEMBB,AvgLatencyMMTC,AvgEnergyMMTC\n";

    for (const auto& result : results) {
        file << result.iterations << "," << result.hms << "," << result.hmcr << "," << result.par << ",";
        file << result.bestFitness << "," << result.computationTime << ",";
        file << result.avgLatencyURLLC << "," << result.avgEnergyURLLC << ",";
        file << result.avgLatencyEMBB << "," << result.avgEnergyEMBB << ",";
        file << result.avgLatencyMMTC << "," << result.avgEnergyMMTC << "\n";
    }

    file.close();
    cout << "Harmony search results saved to harmony_results.csv" << endl;
}

// Save ant colony optimization results to CSV
void saveAntColonyResultsToCSV(const vector<TestResult>& results) {
    ofstream file("ant_colony_results.csv");
    file << "Iterations,NumAnts,PheroCoeff,HeurCoeff,Rho,Q,BestFitness,ComputationTime(ms),";
    file << "AvgLatencyURLLC,AvgEnergyURLLC,AvgLatencyEMBB,AvgEnergyEMBB,AvgLatencyMMTC,AvgEnergyMMTC\n";

    for (const auto& result : results) {
        file << result.iterations << "," << result.numAnts << "," << result.pheroCoeff << ",";
        file << result.heurCoeff << "," << result.rho << "," << result.q << ",";
        file << result.bestFitness << "," << result.computationTime << ",";
        file << result.avgLatencyURLLC << "," << result.avgEnergyURLLC << ",";
        file << result.avgLatencyEMBB << "," << result.avgEnergyEMBB << ",";
        file << result.avgLatencyMMTC << "," << result.avgEnergyMMTC << "\n";
    }

    file.close();
    cout << "Ant colony results saved to ant_colony_results.csv" << endl;
}

// Save comparison results to CSV
void saveComparisonResultsToCSV(const vector<pair<TestResult, TestResult>>& comparisons) {
    ofstream file("algorithm_comparison.csv");
    file << "HarmonyBestFitness,HarmonyTime(ms),";
    file << "AntColonyBestFitness,AntColonyTime(ms),";
    file << "FitnessDifference(%),TimeDifference(%)\n";

    for (const auto& comp : comparisons) {
        const auto& harmony = comp.first;
        const auto& antColony = comp.second;

        // Calculate differences
        float fitnessDiff = (antColony.bestFitness - harmony.bestFitness) / harmony.bestFitness * 100.0f;
        float timeDiff = (antColony.computationTime - harmony.computationTime) / harmony.computationTime * 100.0f;

        file << harmony.bestFitness << "," << harmony.computationTime << ",";
        file << antColony.bestFitness << "," << antColony.computationTime << ",";
        file << fitnessDiff << "," << timeDiff << "\n";
    }

    file.close();
    cout << "Algorithm comparison saved to algorithm_comparison.csv" << endl;
}

int main() {
    cout << "Running performance tests..." << endl;

    // Number of test cases
    const int NUM_TESTS = 3;

    // Generate UE lists for each test
    vector<vector<UE>> ueTestSets;
    for (int i = 0; i < NUM_TESTS; ++i) {
        ueTestSets.push_back(generateUEList());
    }

    // Harmony Search parameter variations
    vector<uint32_t> harmonyIterations = {50, 100, 150};
    vector<uint32_t> hmsList = {5, 10, 15};
    vector<float> hmcrList = {0.8, 0.9, 0.95};
    vector<float> parList = {0.1, 0.3, 0.5};

    // Ant Colony parameter variations
    vector<uint32_t> antIterations = {50, 100, 150};
    vector<uint32_t> numAntsList = {5, 10, 15};
    vector<float> pheroCoeffList = {0.5, 1.0, 1.5};
    vector<float> heurCoeffList = {1.0, 2.0, 3.0};
    vector<float> rhoList = {0.05, 0.1, 0.2};
    vector<float> qList = {50.0, 100.0, 150.0};

    // Results storage
    vector<TestResult> harmonyResults;
    vector<TestResult> antColonyResults;
    vector<pair<TestResult, TestResult>> comparisons;

    // Run tests
    for (int i = 0; i < NUM_TESTS; ++i) {
        cout << "Test " << (i+1) << " of " << NUM_TESTS << ":" << endl;

        // Run Harmony Search with current parameters
        cout << "  Running Harmony Search (Iterations=" << harmonyIterations[i]
             << ", HMS=" << hmsList[i] << ", HMCR=" << hmcrList[i]
             << ", PAR=" << parList[i] << ")..." << endl;

        TestResult harmonyResult = runHarmonySearchTest(
            ueTestSets[i], harmonyIterations[i], hmsList[i], hmcrList[i], parList[i]
        );
        harmonyResults.push_back(harmonyResult);

        // Run Ant Colony with current parameters
        cout << "  Running Ant Colony Optimization (Iterations=" << antIterations[i]
             << ", NumAnts=" << numAntsList[i] << ", PheroCoeff=" << pheroCoeffList[i]
             << ", HeurCoeff=" << heurCoeffList[i] << ", Rho=" << rhoList[i]
             << ", Q=" << qList[i] << ")..." << endl;

        TestResult antColonyResult = runAntColonyTest(
            ueTestSets[i], antIterations[i], numAntsList[i], pheroCoeffList[i],
            heurCoeffList[i], rhoList[i], qList[i]
        );
        antColonyResults.push_back(antColonyResult);

        // Store comparison
        comparisons.push_back({harmonyResult, antColonyResult});

        cout << "  Harmony Best Fitness: " << harmonyResult.bestFitness
             << ", Time: " << harmonyResult.computationTime << " ms" << endl;
        cout << "  Ant Colony Best Fitness: " << antColonyResult.bestFitness
             << ", Time: " << antColonyResult.computationTime << " ms" << endl;
        cout << endl;
    }

    // Save results to CSV files
    saveHarmonyResultsToCSV(harmonyResults);
    saveAntColonyResultsToCSV(antColonyResults);
    saveComparisonResultsToCSV(comparisons);

    cout << "All tests completed successfully!" << endl;

    return 0;
}