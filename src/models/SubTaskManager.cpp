#include "SubTaskManager.h"
#include <iostream>
#include <algorithm>
#define INF 2147483647
#include <unordered_set>
SubTaskManager::SubTaskManager(){

}
                   
// Destructor
SubTaskManager::~SubTaskManager() {
    subTaskCosts.clear();
}

// Function to set subtask cost
void SubTaskManager::setSubTaskCost(int taskID, int subTaskID, int hwID, int cost) {
    SubTaskKey key { taskID, subTaskID, hwID };
    subTaskCosts[key] = cost;
}

void SubTaskManager::setSubTaskTime(int taskID, int subTaskID, int hwID, int time) {
    SubTaskKey key { taskID, subTaskID, hwID };
    subTaskTimes[key] = time;
}

// Function to get subtask cost
int SubTaskManager::getSubTaskCost(int taskID, int subTaskID, int hwID) {
    SubTaskKey key { taskID, subTaskID, hwID };
    auto it = subTaskCosts.find(key);
    if (it != subTaskCosts.end()) {
        return it->second;
    } else {
        return -1;
    }
}

int SubTaskManager::getSubTaskTime(int taskID, int subTaskID, int hwID) {
    SubTaskKey key { taskID, subTaskID, hwID };
    auto it = subTaskTimes.find(key);
    if (it != subTaskTimes.end()) {
        return it->second;
    } else {
        return -1;
    }
}

int SubTaskManager::getCheapestHW(int taskID, int subTaskID) {
    int minCost = INF;
    int cheapestHW = -1;

    for (const auto& entry : subTaskCosts) {
        const SubTaskKey& key = entry.first;
        if (key.taskID == taskID && key.subTaskID == subTaskID) {
            int cost = entry.second;
            if (cost < minCost) {
                minCost = cost;
                cheapestHW = key.hwID;
            }
        }
    }

    return cheapestHW;
}

int SubTaskManager::getFastestHW(int taskID, int subTaskID) {
    int minCost = INF;
    int cheapestHW = -1;

    for (const auto& entry : subTaskTimes) {
        const SubTaskKey& key = entry.first;
        if (key.taskID == taskID && key.subTaskID == subTaskID) {
            int cost = entry.second;
            if (cost < minCost) {
                minCost = cost;
                cheapestHW = key.hwID;
            }
        }
    }

    return cheapestHW;
}

int SubTaskManager::getNumSubTasks(int taskID) const {
    std::unordered_set<int> uniqueSubTaskIDs;

    for (const auto& entry : subTaskCosts) {
        const SubTaskKey& key = entry.first;
        if (key.taskID == taskID) {
            uniqueSubTaskIDs.insert(key.subTaskID);
        }
    }

    return uniqueSubTaskIDs.size();
}