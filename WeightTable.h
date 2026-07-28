#include <iostream>
#include <utility>
#include <vector>

struct WeightTable {
    int starting;
    int ending;
};

class Schedule {
private:
    std::vector<std::pair<int, TaskTime>> schedulePlan;

public:
    void addTaskToPlan(int task_id, int start, int end) {
        TaskTime time = {start, end};
        schedulePlan.push_back(std::make_pair(task_id, time));
    }
}