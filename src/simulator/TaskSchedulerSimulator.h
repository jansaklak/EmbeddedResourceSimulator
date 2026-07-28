#pragma once
#ifndef COST_LIST_H
#define COST_LIST_H

#include <mutex>
#include <iostream>
#include <fstream>
#include <set>
#include <vector>
#include <queue>
#include <stack>
#include <functional>
#include <map>
#include <thread>
#include <chrono>
#include <unordered_map>
#include <iomanip>
#include <algorithm>
#include <random>
#include <limits>

#include "../models/HardwareInstance.h"
#include "../models/CommunicationBus.h"
#include "../models/TaskGraph.h"
#include "../models/HardwareProcessor.h"
#include "../models/ExecutionMatrix.h"
#include "../models/Edge.h"
#include "../models/TimeAndCost.h"
#include "../models/SubTaskManager.h"
#include "../models/ConfigParser.h"

constexpr int INF = std::numeric_limits<int>::max();

/**
 * @brief Represents conditional task evaluation parameters.
 */
struct TaskData {
    std::string variable;
    std::string op;
    int value;
};

/**
 * @brief Structure for heuristic weight normalization comparisons.
 */
struct WeightTable {
    Instance* inst;
    double TCP;
    double TC;
    double Tw;
    double reTw;
    double Cw;
    double reCw;
    double StartingTime;
    double runTime;
    double reCalc;
    double idleTime;
    double asBefore;
    double SUM;
};

/**
 * @brief Thread-safe random integer generator within [0, MAX - 1].
 */
inline int getRand(int MAX) {
    if (MAX < 1) MAX = 1;
    static std::mt19937 rng(1337);
    std::uniform_int_distribution<int> dist(0, MAX - 1);
    return dist(rng);
}

/**
 * @brief Random boolean generator with given probability denominator.
 */
inline bool randomBool(int prob) {
    if (prob < 0) prob = 0;
    static std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(0, prob);
    return dist(rng) == 0;
}

static void Licznik(bool& stop, int& czas) {
    auto start = std::chrono::steady_clock::now();
    while (!stop) {
        auto current = std::chrono::steady_clock::now();
        czas = std::chrono::duration_cast<std::chrono::milliseconds>(current - start).count();
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
}

/**
 * @brief Central simulation engine managing task scheduling, DAG traversal, hardware allocation, and reporting.
 */
class TaskSchedulerSimulator {
private:
    // Core Configuration & Stats
    bool with_cost;
    int totalCost;
    int tasks_amount;
    int hardware_cores_amount;
    int processing_unit_amount;
    int channels_amount;
    int simulation_time_scale;
    int hard_time = 250;
    int penalty_factor = 2;

    // Collections & Storage
    std::vector<HardwareProcessor> Hardwares;
    std::vector<CommunicationBus> Channels;
    std::vector<int> allocated_tasks;
    std::vector<int> progress;
    std::vector<std::set<int>> HWtoTasks;
    std::vector<HardwareInstance*> Instances;
    std::vector<std::deque<int>> paths;

    // Task & Schedule Mapping
    std::map<int, int> HWInstancesCount;
    std::map<int, HardwareInstance*> taskInstanceMap;
    std::map<int, std::pair<int, int>> task_schedule;
    std::unordered_map<int, int> startingTimeCache;
    std::map<int, std::string> conditions;
    std::map<int, TaskData> conditionTaskMap;
    std::map<int, std::vector<HardwareProcessor>> subTaskHW;
    std::unordered_map<std::string, std::string> CostListConfig;

    // Special Task Sets
    std::set<int> unpredictedTasks;
    std::set<int> conditionalTasks;
    std::set<int> extendedTasks;

    // Sub-components
    SubTaskManager sumTasksTable;
    Graf TaskGraph;
    ExecutionMatrix times;

    // Internal Helpers
    std::vector<int> findAllToSkipAfterConditional(int taskID);
    void createRandomTasksGraph();
    void connectRandomCH();
    int createRandomProc();
    int Load_Config();

    // Heuristics & Weights
    void getCurrWeight(int task_id, bool changeInstances, int MAX_TIME);
    void recurrent_distribution_helper(int root, std::vector<int> _currSet);
    void reallocateFastest(int maxTIME, std::vector<bool>& checked);
    double time_cost_proc(int task_id, const HardwareInstance* inst, double t_factor = 1.0, double c_factor = 1.0, double p_factor = 1.0);
    double time_cost(int task_id, const HardwareInstance* inst);
    double time_weight(int task_id, const HardwareInstance* inst);
    double reuse_time_weight(int task_id, const HardwareInstance* inst);
    double cost_weight(int task_id, const HardwareInstance* inst);
    double allocated_cost(int task_id, const HardwareInstance* inst, double MAX_TIME);
    double inst_starting(int task_id, const HardwareInstance* inst);
    double inst_time_running(int task_id, const HardwareInstance* inst);
    double reCalculate(int task_id, const HardwareInstance* inst);
    double longestIdle(int task_id, const HardwareInstance* inst);
    double asBefore(int task_id, const HardwareInstance* inst);

public:
    // Lifecycle Management
    TaskSchedulerSimulator();
    TaskSchedulerSimulator(int tasks, int hcores, int punits, int channels, int withCost);
    TaskSchedulerSimulator(const TaskSchedulerSimulator& other) = delete;
    TaskSchedulerSimulator& operator=(const TaskSchedulerSimulator& other) = delete;
    TaskSchedulerSimulator(TaskSchedulerSimulator&& other) noexcept;
    TaskSchedulerSimulator& operator=(TaskSchedulerSimulator&& other) noexcept;
    ~TaskSchedulerSimulator();

    // Data Operations
    int Load_From_File(const std::string& filename);
    void randALL();
    void createRandomConditionalTasksGraph();
    void clear();
    void clearNUM();
    void invalidateStartingTimeCache() { startingTimeCache.clear(); }
    void makeConditional(int Task_ID);
    void setHardTime(int ht) { hard_time = ht; }
    int getHardTime() const { return hard_time; }
    void setPenaltyFactor(int pf) { penalty_factor = pf; }
    int getPenaltyFactor() const { return penalty_factor; }

    // Queries & Metrics
    ExecutionMatrix getTimes() const;
    Graf getGraph() const;
    int getStartingTime(int task_id);
    int getStartingTimeScheduled(int task_id);
    int getEndingTime(int task_id);
    int getInstanceStartingTime(const HardwareInstance* inst);
    int getInstanceEndingTime(const HardwareInstance* inst);
    int getCriticalTime() const;
    int getTimeRunning(const HardwareInstance* inst);
    int getIdleTime(const HardwareInstance* inst, int timeStop);
    std::vector<int> getLongestPath(int start);
    std::vector<HardwareProcessor> getHardwares() const;
    std::vector<CommunicationBus> getCOMS() const;
    std::deque<int> getMaxPath(std::vector<int> toSkip) const;
    HardwareProcessor* getLowestTimeHardware(int task_id, int time_cost_normalized);
    HardwareProcessor* getSlowestHardware(int);
    const HardwareInstance* getShortestRunningInstance();
    const HardwareInstance* getLongestRunningInstance();

    // Instance Management
    HardwareInstance* getInstance(int task_id) const;
    int createInstance(int task_ID, const HardwareProcessor* h);
    int createInstance(int task_ID);
    void addTaskToInstance(int task_ID, HardwareInstance* inst);
    void removeTaskFromInstance(int task_ID);
    void sortTaskSet(HardwareInstance* inst);
    void removeTaskHelper(int task_ID);
    void calculateTotalCost();

    // Task & Path Processing
    void subTaskHandler(int task_id);
    void unpredictedHandler(int task_ID);
    int evaluateCondition(int task_id);
    void constructByWeight(std::vector<int> bfs_tasks, int MAX_TIME = INF);
    void printUnpredictedTasks();
    void skipConditional();
    void createPaths(std::vector<std::vector<Edge>>);
    void printPaths();

    // Strategy Dispatch & Algorithms
    void taskDistribution(int rule);
    void scheduleMinTimeDedicated();
    void scheduleMinCostDedicated();
    void scheduleMinTimeInstanceReuse();
    void scheduleMinCostInstanceReuse();
    void scheduleBfsLevelOrder();
    void scheduleCriticalPathGreedy();
    void scheduleTwoPhaseRefined();
    void scheduleConstrainedPenaltyOptimization();
    void scheduleSingleCoreBaseline();
    void scheduleNormalizedInstanceReuse();
    void scheduleNormalizedBfs();
    void scheduleRecursiveNeighborhood();
    void scheduleNormalizedPackingBfs();
    void runTasks();
    void TaskRunner(HardwareInstance inst);

    // Output & Export Functions
    void printSchedule();
    void printTasks(std::ostream& out = std::cout) const;
    void printProc(std::ostream& out = std::cout) const;
    void printCOMS(std::ostream& out = std::cout) const;
    void printALL(std::string filename, bool toScreen) const;
    void printConditions(std::ostream& out = std::cout) const;
    void printToGantt(std::string filename = "gantt_data.dat");
    void printInstances();
    void printTotalCost();
    void exportToJSON(std::ostream& out) const;
    void exportToJSONFile(const std::string& filename) const;
};

/// Backward compatibility alias
using Cost_List = TaskSchedulerSimulator;

#endif // COST_LIST_H