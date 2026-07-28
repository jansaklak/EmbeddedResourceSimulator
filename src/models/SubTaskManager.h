#pragma once
#ifndef SUBTASKS_H
#define SUBTASKS_H

#include <unordered_map>
#include <cstddef>
#include <functional>

/**
 * @brief Hash key structure for fine-grained subtask lookup by task, subtask, and hardware IDs.
 */
struct SubTaskKey {
    int taskID;
    int subTaskID;
    int hwID;

    bool operator==(const SubTaskKey& other) const {
        return taskID == other.taskID && subTaskID == other.subTaskID && hwID == other.hwID;
    }
};

/**
 * @brief Hash function provider for SubTaskKey.
 */
struct SubTaskKeyHash {
    std::size_t operator()(const SubTaskKey& key) const {
        return std::hash<int>()(key.taskID) ^ std::hash<int>()(key.subTaskID) ^ std::hash<int>()(key.hwID);
    }
};

/**
 * @brief Registry and manager for decomposing extended tasks into fine-grained subtasks.
 */
class SubTaskManager {
private:
    std::unordered_map<SubTaskKey, int, SubTaskKeyHash> subTaskCosts;
    std::unordered_map<SubTaskKey, int, SubTaskKeyHash> subTaskTimes;

public:
    SubTaskManager();
    ~SubTaskManager();

    int getSubTaskCost(int taskID, int subTaskID, int hwID);
    int getSubTaskTime(int taskID, int subTaskID, int hwID);

    int getFastestHW(int taskID, int subTaskID);
    int getCheapestHW(int taskID, int subTaskID);
    int getNumSubTasks(int taskID) const;

    void setSubTaskCost(int taskID, int subTaskID, int hwID, int cost);
    void setSubTaskTime(int taskID, int subTaskID, int hwID, int cost);
};

/// Backward compatibility alias
using SubTasks = SubTaskManager;

#endif // SUBTASKS_H
