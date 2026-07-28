#include "TaskSchedulerSimulator.h"
#include <thread>
#include <chrono>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <mutex>
#include <map>

Instance* TaskSchedulerSimulator::getInstance(int task_id) const{
    auto it = taskInstanceMap.find(task_id);
    if (it != taskInstanceMap.end()) {
        return it->second;
    } else {
        return nullptr;
    }
}

int TaskSchedulerSimulator::getStartingTime(int task_id) {
    if(task_id == 0) return 0;
    auto cacheIt = startingTimeCache.find(task_id);
    if (cacheIt != startingTimeCache.end()) {
        return cacheIt->second;
    }
    int maxTime = 0;
    for (std::vector<int> path : TaskGraph.DFS(0, task_id)) {
        int pathTime = 0;
        bool skipPath = false;
        for (int t_id : path) {
            if (t_id == task_id) {
                break;
            } else {
                const HardwareInstance* inst = getInstance(t_id);
                if (inst == nullptr) {
                    skipPath = true;
                    break;
                }
                const HardwareProcessor* hardwarePtr = inst->getHardwarePtr();
                if (hardwarePtr != nullptr) {
                    pathTime += times.getTime(t_id, hardwarePtr);
                }
            }
        }
        if (skipPath) {
            continue;
        }
        if (pathTime > maxTime) {
            maxTime = pathTime;
        }
    }
    startingTimeCache[task_id] = maxTime;
    return maxTime;
}

int TaskSchedulerSimulator::getStartingTimeScheduled(int task_id) {
    if(task_id == 0) return 0;
    int maxTime = 0;
    for (std::vector<int> path : TaskGraph.DFS(0, task_id)) {
        int pathTime = 0;
        bool skipPath = false;
        for (int t_id : path) {
            if (t_id == task_id) {
                break;
            } else {
                const HardwareInstance* inst = getInstance(t_id);
                if (inst == nullptr) {
                    skipPath = true;
                    break;
                }
                const HardwareProcessor* hardwarePtr = inst->getHardwarePtr();
                if (hardwarePtr != nullptr) {
                    pathTime += times.getTime(t_id, hardwarePtr);
                    if (pathTime < task_schedule[t_id].second) {
                        pathTime = task_schedule[t_id].second;
                    }
                }
            }
        }
        if (skipPath) {
            continue;
        }
        if (pathTime > maxTime) {
            maxTime = pathTime;
        }
    }
    return maxTime;
}

int TaskSchedulerSimulator::getEndingTime(int task_id) {
    const Instance* inst = getInstance(task_id);
    if (!inst || !inst->getHardwarePtr()) return getStartingTime(task_id);
    int runningTime = times.getTime(task_id, inst->getHardwarePtr());
    return getStartingTime(task_id) + runningTime;
}

std::vector<int> TaskSchedulerSimulator::getLongestPath(int start) {
        std::vector<std::vector<Edge>> adjList = TaskGraph.getAdjList();
        std::vector<int> dist(TaskGraph.getVerticesSize(), std::numeric_limits<int>::min());
        std::vector<int> inDegree(TaskGraph.getVerticesSize(), 0);
        std::queue<int> q;
        for (const auto& edges : adjList) {
            for (const auto& edge : edges) {
                int v = edge.getV2();
                inDegree[v]++;
            }
        }
        for (int i = 0; i < TaskGraph.getVerticesSize(); ++i) {
            if (inDegree[i] == 0)
                q.push(i);
        }

        std::vector<int> longestPath;
        while (!q.empty()) {
            int u = q.front();
            q.pop();
            longestPath.push_back(u);
            for (const auto& edge : adjList[u]) {
                int v = edge.getV2();
                int w = times.getTime(edge.getV2(),getLowestTimeHardware(edge.getV2(),0));
                if (dist[u] + w > dist[v]) {
                    dist[v] = dist[u] + w;
                }
                inDegree[v]--;
                if (inDegree[v] == 0)
                    q.push(v);
            }
        }

        return longestPath;
    }

    void TaskSchedulerSimulator::createPaths(std::vector<std::vector<Edge>> adjList) {
        std::vector<bool> visited(TaskGraph.getVerticesSize(), false);
        bool allVisited = false;
        std::deque<std::deque<int>> queue;
        std::deque<int> firstElementInQueue;
        firstElementInQueue.push_back(0);
        queue.push_back(firstElementInQueue);

        while (!allVisited || queue.size() != 0) {
            std::deque<int> firstVectorFromQueue = queue.front();
            queue.pop_front();
            int current = firstVectorFromQueue.back();
            visited[current] = true;
            std::vector<Edge> neighboursOfCurrent(adjList[current]);

            if (neighboursOfCurrent.size() > 0) {
                for (std::vector<Edge>::iterator it = neighboursOfCurrent.begin(); it != neighboursOfCurrent.end(); it++) {
                    std::deque<int> newVector(firstVectorFromQueue);
                    newVector.push_back((*it).getV2());
                    queue.push_back(newVector);
                }
            }
            else {
                paths.push_back(firstVectorFromQueue);
            }
            allVisited = true;
            for (size_t i = 0; i < visited.size(); i++) {
                allVisited = allVisited && visited[i];
            }
        }
    }

    void TaskSchedulerSimulator::printPaths() {
        for (auto const& path : paths) {
            for (auto const& n : path) {
                std::cout << n << " ";
            }
            std::cout << std::endl;
        }
    }

    std::deque<int> TaskSchedulerSimulator::getMaxPath(std::vector<int> toSkip) const {
        std::deque<int> maxPath;
        int maxWeight = 0;

        for (const auto& path : paths) {
            int currentWeight = 0;
            bool pathSkipped = false;

            for (const auto& node : path) {
                if (toSkip[node] == 0) {
                    pathSkipped = true;
                    break;
                }
                currentWeight += (times.getTime(node, getInstance(node)->getHardwarePtr()) * 
                    times.getCost(node, getInstance(node)->getHardwarePtr()));
            }

            if (!pathSkipped && currentWeight > maxWeight) { // Zmieniamy warunek, aby szukać maksymalnej wagi
                maxWeight = currentWeight;
                maxPath = path;
            }
        }

        return maxPath;
    }


    Hardware* TaskSchedulerSimulator::getLowestTimeHardware(int task_id, int time_cost_normalized){
        Hardware* outHW = nullptr;
        int min_time = INF;
        for (Hardware& hw : Hardwares) {
            int time = INF;
            if(time_cost_normalized == 0){
                time = times.getTime(task_id, &hw);
            }
            else if (time_cost_normalized == 1){
                time = times.getCost(task_id, &hw);
            }
            else if(time_cost_normalized == 2){
                time = times.getNormalized(task_id, &hw);
            }
            if (time < min_time) {
                min_time = time;
                outHW = &hw;
            }
        }
        return outHW;
    }



    Hardware* TaskSchedulerSimulator::getSlowestHardware(int task_id) {
        Hardware*outHW = nullptr;
        int maxTime = 0;
        for (Hardware& hw : Hardwares) {
            if (times.getTime(task_id, &hw) > maxTime) {
                maxTime = times.getTime(task_id, &hw);
                outHW = &hw;
            }
        }
        return outHW;
    }

int TaskSchedulerSimulator::getCriticalTime() const{
    int maxTime = 0;
    // for(Instance* i : Instances){
    //     if(getInstanceEndingTime(i)>maxTime) maxTime = getInstanceEndingTime(i);
    // }
    for ( const auto &p : task_schedule )
    {
        if(p.second.second > maxTime){
            maxTime = p.second.second;
        }
    }
    return maxTime;
}

int TaskSchedulerSimulator::getInstanceStartingTime(const Instance* inst){
    int startingTime= 0;
    for(int i : inst->getTaskSet()){
        if(getStartingTime(i)>startingTime) startingTime = getStartingTime(i);
    }
    return startingTime;
}

int TaskSchedulerSimulator::getInstanceEndingTime(const Instance* inst){
    int endingTime= 0;
    for(int i : inst->getTaskSet()){
        if(getEndingTime(i)>endingTime) endingTime = getEndingTime(i);
    }
    return endingTime;
}

int TaskSchedulerSimulator::getTimeRunning(const Instance* inst){
    int total_time =0;
    for(int i : inst->getTaskSet()){
        total_time += getEndingTime(i) - getStartingTime(i);
    }
    return total_time;
}

int TaskSchedulerSimulator::getIdleTime(const Instance* inst,int timeStop){
        int total_time =0;
        for(int i : inst->getTaskSet()){
            if(getStartingTime(i) + (getEndingTime(i) - getStartingTime(i)) >=timeStop){
                break;
            }
            total_time += getEndingTime(i) - getStartingTime(i);
            
        }
        return timeStop - total_time;
}

const Instance* TaskSchedulerSimulator::getLongestRunningInstance(){
    int longest_running = std::numeric_limits<int>::min();
    const Instance* longest = nullptr;
    for (const Instance* inst : Instances) {
        int running_time = getTimeRunning(inst);
        if (running_time > longest_running) {
            longest_running = running_time;
            longest = inst;
        }
    }
    return longest;
}

const Instance* TaskSchedulerSimulator::getShortestRunningInstance() {
        int shortest_running = std::numeric_limits<int>::max();
        const Instance* shortest = nullptr;
        for (const Instance* inst : Instances) {
        int running_time = getTimeRunning(inst);
            if (running_time < shortest_running) {
                shortest_running = running_time;
                shortest = inst;
            }
        }
        return shortest;
}