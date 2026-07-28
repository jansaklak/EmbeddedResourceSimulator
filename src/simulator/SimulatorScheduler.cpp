#include "TaskSchedulerSimulator.h"
#include <thread>
#include <chrono>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <mutex>
#include <map>
#include <stack>
#include <deque>

void TaskSchedulerSimulator::recurrent_distribution_helper(int root, std::vector<int> _currSet) {
    std::vector<int> currSet = _currSet;
    bool rootInstanceUsed = false;
    if (currSet.empty()) {
        return;
    }
    for (int currTask : currSet) {
        HardwareProcessor* currTask_lowestTimeHW = getLowestTimeHardware(currTask, 0);
        if (!rootInstanceUsed && getInstance(root)->getHardwarePtr() == currTask_lowestTimeHW) {
            addTaskToInstance(currTask, getInstance(root));
            rootInstanceUsed = true;
        } else {
            createInstance(currTask);
        }
    }
}

// S1: Najszybsza Dedykowana (Min-Time Dedicated)
void TaskSchedulerSimulator::scheduleMinTimeDedicated() {
    int tasks_amount = TaskGraph.getVerticesSize();
    for (int task_id = 0; task_id < tasks_amount; ++task_id) {
        createInstance(task_id);
    }
}

// S2: Najtańsza Dedykowana (Min-Cost Dedicated)
void TaskSchedulerSimulator::scheduleMinCostDedicated() {
    int tasks_amount = TaskGraph.getVerticesSize();
    for (int task_id = 0; task_id < tasks_amount; ++task_id) {
        HardwareProcessor* hw = getLowestTimeHardware(task_id, 1);
        createInstance(task_id, hw);
    }
}

// S3: Najszybsza z Upakowywaniem Instancji (Min-Time Instance Reuse)
void TaskSchedulerSimulator::scheduleMinTimeInstanceReuse() {
    int tasks_amount = TaskGraph.getVerticesSize();
    for (int task_id = 0; task_id < tasks_amount; ++task_id) {
        HardwareProcessor* hw = getLowestTimeHardware(task_id, 0);
        bool foundInstance = false;
        for (HardwareInstance* inst : Instances) {
            if (inst->getHardwarePtr() == hw) {
                addTaskToInstance(task_id, inst);
                foundInstance = true;
                break;
            }
        }
        if (!foundInstance) {
            createInstance(task_id, hw);
        }
    }
}

// S4: Najtańsza z Upakowywaniem Instancji (Min-Cost Instance Reuse)
void TaskSchedulerSimulator::scheduleMinCostInstanceReuse() {
    int tasks_amount = TaskGraph.getVerticesSize();
    for (int task_id = 0; task_id < tasks_amount; ++task_id) {
        HardwareProcessor* hw = getLowestTimeHardware(task_id, 1);
        bool foundInstance = false;
        for (HardwareInstance* inst : Instances) {
            if (inst->getHardwarePtr() == hw) {
                addTaskToInstance(task_id, inst);
                foundInstance = true;
                break;
            }
        }
        if (!foundInstance) {
            createInstance(task_id, hw);
        }
    }
}

// S5: Poziomowa BFS (BFS Level-Order Scheduling)
void TaskSchedulerSimulator::scheduleBfsLevelOrder() {
    int tasks_amount = TaskGraph.getVerticesSize();
    createInstance(0);
    std::vector<int> allocatedTasks(tasks_amount, 0);
    allocatedTasks[0] = 1;
    for (int i : TaskGraph.BFS()) {
        for (HardwareInstance* ins : Instances) {
            if (allocatedTasks[i] == 1) break;
            if (getInstanceEndingTime(ins) <= getStartingTime(i) && ins->getHardwarePtr() == getLowestTimeHardware(i, 0)) {
                allocatedTasks[i] = 1;
                addTaskToInstance(i, ins);
            }
        }
        if (allocatedTasks[i] == 0) {
            createInstance(i);
            allocatedTasks[i] = 1;
        }
    }
}

// S6: Zachłanna Ścieżki Krytycznej (Critical-Path Greedy)
void TaskSchedulerSimulator::scheduleCriticalPathGreedy() {
    HardwareProcessor* FirstLowestTimeHW = getLowestTimeHardware(0, 0);
    std::set<int> allocatedTasks;
    allocatedTasks.insert(0);
    int currTask = 0;
    std::stack<int> toCheck;
    toCheck.push(0);
    createInstance(currTask, FirstLowestTimeHW);

    while (!toCheck.empty()) {
        int max_task_id = 0;
        int max_time = 0;
        for (int i : TaskGraph.getNeighbourIndices(currTask)) {
            toCheck.push(i);
            int curr_time = times.getTime(i, getLowestTimeHardware(i, 0));
            if (curr_time > max_time) {
                max_time = curr_time;
                max_task_id = i;
            }
        }
        if (allocatedTasks.find(max_task_id) == allocatedTasks.end()) {
            for (HardwareInstance* ins : Instances) {
                if (getInstanceEndingTime(ins) < getStartingTime(max_task_id)) {
                    addTaskToInstance(max_task_id, ins);
                    allocatedTasks.insert(max_task_id);
                    break;
                }
            }
            if (allocatedTasks.find(max_task_id) == allocatedTasks.end()) {
                createInstance(max_task_id);
                allocatedTasks.insert(max_task_id);
            }
        }

        for (int i : TaskGraph.getNeighbourIndices(currTask)) {
            if (i != max_task_id && allocatedTasks.find(i) == allocatedTasks.end()) {
                for (HardwareInstance* ins : Instances) {
                    if (getInstanceEndingTime(ins) < getStartingTime(i)) {
                        addTaskToInstance(i, ins);
                        allocatedTasks.insert(i);
                        break;
                    }
                }
                if (allocatedTasks.find(i) == allocatedTasks.end()) {
                    createInstance(i);
                    allocatedTasks.insert(i);
                }
            }
        }
        currTask = toCheck.top();
        toCheck.pop();
    }
}

// S7: Hybrydowa z Dwuetapową Rafinacją (Two-Phase Refined)
void TaskSchedulerSimulator::scheduleTwoPhaseRefined() {
    createInstance(0);
    allocated_tasks[0] = 1;
    for (int i : TaskGraph.BFS()) {
        for (HardwareInstance* ins : Instances) {
            if (allocated_tasks[i] == 1) break;
            if (getInstanceEndingTime(ins) <= getStartingTime(i)) {
                allocated_tasks[i] = 1;
                addTaskToInstance(i, ins);
            }
        }
        if (allocated_tasks[i] == 0) {
            createInstance(i);
            allocated_tasks[i] = 1;
        }
    }
}

// S8: Optymalizacja Kosztowo-Czasowa z Funkcją Kary (Constrained Penalty Optimization)
void TaskSchedulerSimulator::scheduleConstrainedPenaltyOptimization() {
    int LOOP_COUNTER = 3;
    int HARD_TIME = 250;
    int PUNISHMENT = 2;

    std::vector<int> bfs_tasks = TaskGraph.BFS();
    std::vector<int> tasks_visited_count(TaskGraph.getVerticesSize(), LOOP_COUNTER);

    // Initial fast packing
    for (int currTask : bfs_tasks) {
        HardwareProcessor* hw = getLowestTimeHardware(currTask, 0);
        for (HardwareInstance* inst : Instances) {
            if (inst->getHardwarePtr() == hw && getInstanceEndingTime(inst) <= getStartingTime(currTask)) {
                addTaskToInstance(currTask, inst);
                allocated_tasks[currTask] = 1;
                break;
            }
        }
        createInstance(currTask);
    }

    int possibleMoves = 0;
    for (int t = 0; t < TaskGraph.getVerticesSize(); t++) {
        possibleMoves += tasks_visited_count[t];
    }

    std::deque<int> criticalTimeResults;
    for (int k = 0; k < TaskGraph.getVerticesSize(); k++) {
        criticalTimeResults.push_back(INF - k);
    }

    while (possibleMoves > 0 || criticalTimeResults.front() != criticalTimeResults.back()) {
        createPaths(TaskGraph.getAdjList());
        std::deque<int> maxWezel = getMaxPath(tasks_visited_count);

        if (maxWezel.empty()) break;
        int Task_to_Check = maxWezel.back();

        if (Task_to_Check == 0 || tasks_visited_count[Task_to_Check] > 0) {
            tasks_visited_count[Task_to_Check]--;
            int min_time = INF;
            HardwareInstance* bestFoundInst = nullptr;

            for (HardwareInstance* inst : Instances) {
                int cost = times.getCost(Task_to_Check, inst->getHardwarePtr());
                if (cost < min_time) {
                    min_time = cost;
                    bestFoundInst = inst;
                }
            }

            addTaskToInstance(Task_to_Check, bestFoundInst);
        }

        printSchedule();

        int totalTime = getCriticalTime();
        if (totalTime > HARD_TIME) {
            totalCost = 0;
            for (HardwareInstance* instance : Instances) {
                totalCost += instance->getHardwarePtr()->getCost();
                for (int taskID : instance->getTaskSet()) {
                    totalCost += times.getCost(taskID, instance->getHardwarePtr());
                }
            }
            totalCost += (totalTime - HARD_TIME) * PUNISHMENT;
        }

        criticalTimeResults.pop_front();
        criticalTimeResults.push_back(getCriticalTime());
        possibleMoves = 0;
        for (int t = 0; t < TaskGraph.getVerticesSize(); t++) {
            possibleMoves += tasks_visited_count[t];
        }
    }
}

// S9: Monolityczna Jednoprocesorowa (Single Core Baseline)
void TaskSchedulerSimulator::scheduleSingleCoreBaseline() {
    int tasks_amount = TaskGraph.getVerticesSize();
    if (tasks_amount == 0) return;

    HardwareProcessor* hw = getLowestTimeHardware(0, 0);
    if (!hw && !Hardwares.empty()) hw = &Hardwares[0];
    if (!hw) return;

    createInstance(0, hw);
    HardwareInstance* singleInst = getInstance(0);
    if (!singleInst) return;

    for (int t = 1; t < tasks_amount; t++) {
        if (unpredictedTasks.find(t) != unpredictedTasks.end()) {
            unpredictedHandler(t);
            continue;
        }
        if (extendedTasks.find(t) != extendedTasks.end()) {
            subTaskHandler(t);
            continue;
        }
        singleInst->addTask(t);
        taskInstanceMap[t] = singleInst;
        allocated_tasks[t] = 1;
    }
}

// S40: Normalized Instance Reuse
void TaskSchedulerSimulator::scheduleNormalizedInstanceReuse() {
    int tasks_amount = TaskGraph.getVerticesSize();
    for (int task_id = 0; task_id < tasks_amount; ++task_id) {
        HardwareProcessor* hw = getLowestTimeHardware(task_id, 2);
        bool foundInstance = false;
        for (HardwareInstance* inst : Instances) {
            if (inst->getHardwarePtr() == hw) {
                addTaskToInstance(task_id, inst);
                foundInstance = true;
                break;
            }
        }
        if (!foundInstance) {
            createInstance(task_id, hw);
        }
    }
}

// S41: Normalized BFS
void TaskSchedulerSimulator::scheduleNormalizedBfs() {
    std::vector<int> bfs_tasks = TaskGraph.BFS();
    for (int currTask : bfs_tasks) {
        HardwareProcessor* hw = getLowestTimeHardware(currTask, 2);
        for (HardwareInstance* inst : Instances) {
            if (inst->getHardwarePtr() == hw && getInstanceEndingTime(inst) <= getStartingTime(currTask)) {
                addTaskToInstance(currTask, inst);
                allocated_tasks[currTask] = 1;
                break;
            }
        }
        if (allocated_tasks[currTask] == 0) {
            createInstance(currTask, getLowestTimeHardware(currTask, 2));
            allocated_tasks[currTask] = 1;
        }
    }
    times.normalize();
}

// S50: Recursive Neighborhood Distribution
void TaskSchedulerSimulator::scheduleRecursiveNeighborhood() {
    int tasks_amount = TaskGraph.getVerticesSize();
    HardwareProcessor* lowestTimeHW = getLowestTimeHardware(0, 0);
    createInstance(0, lowestTimeHW);
    allocated_tasks[0] = 1;
    int currTask = 0;
    std::vector<int> currSet = TaskGraph.getOutNeighbourIndices(currTask);

    while (!currSet.empty()) {
        currSet = TaskGraph.getOutNeighbourIndices(currTask);
        int min_time = 0;
        for (int t : currSet) {
            if (times.getTime(t, lowestTimeHW) > min_time) {
                min_time = times.getTime(t, lowestTimeHW);
                currTask = t;
            }
        }
        if (allocated_tasks[currTask] == 0) {
            addTaskToInstance(currTask, Instances[0]);
            allocated_tasks[currTask] = 1;
        }

        for (int t : currSet) {
            if (allocated_tasks[t] == 0) {
                createInstance(t, getLowestTimeHardware(t, 0));
                allocated_tasks[t] = 1;
            }
        }
    }
    for (int i = 0; i < tasks_amount; i++) {
        if (allocated_tasks[i] == 0) {
            createInstance(i, getLowestTimeHardware(i, 0));
            allocated_tasks[i] = 1;
        }
    }
    for (int i = 0; i < tasks_amount; i++) {
        for (HardwareInstance* ins : Instances) {
            if (getStartingTime(i) < getInstanceEndingTime(ins)) {
                removeTaskFromInstance(i);
                addTaskToInstance(i, ins);
            }
        }
    }
}

// S100: Normalized Packing BFS
void TaskSchedulerSimulator::scheduleNormalizedPackingBfs() {
    std::vector<int> bfs_tasks = TaskGraph.BFS();
    for (int currTask : bfs_tasks) {
        HardwareProcessor* hw = getLowestTimeHardware(currTask, 0);
        for (HardwareInstance* inst : Instances) {
            if (inst->getHardwarePtr() == hw && getInstanceEndingTime(inst) <= getStartingTime(currTask)) {
                addTaskToInstance(currTask, inst);
                allocated_tasks[currTask] = 1;
                break;
            }
        }
        createInstance(currTask);
    }
}

// Main Dispatcher Method
void TaskSchedulerSimulator::taskDistribution(int rule) {
    int tasks_amount = TaskGraph.getVerticesSize();
    times.normalize();
    allocated_tasks.assign(tasks_amount, 0);

    switch (rule) {
        case 1:   scheduleMinTimeDedicated(); break;
        case 2:   scheduleMinCostDedicated(); break;
        case 3:   scheduleMinTimeInstanceReuse(); break;
        case 4:   scheduleMinCostInstanceReuse(); break;
        case 5:   scheduleBfsLevelOrder(); break;
        case 6:   scheduleCriticalPathGreedy(); break;
        case 7:   scheduleTwoPhaseRefined(); break;
        case 8:   scheduleConstrainedPenaltyOptimization(); break;
        case 9:   scheduleSingleCoreBaseline(); break;
        case 40:  scheduleNormalizedInstanceReuse(); break;
        case 41:  scheduleNormalizedBfs(); break;
        case 50:  scheduleRecursiveNeighborhood(); break;
        case 100: scheduleNormalizedPackingBfs(); break;
        default:
            std::cerr << "Nieznana reguła dystrybucji zadań (" << rule << "), stosuję domyślną S1\n";
            scheduleMinTimeDedicated();
            break;
    }

    printSchedule();
    calculateTotalCost();
    printUnpredictedTasks();
    printInstances();
    printTotalCost();
}
