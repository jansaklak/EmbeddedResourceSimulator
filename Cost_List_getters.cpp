#include "Cost_List.h"
#include "Times.h"
#include <thread>
#include <chrono>
#include <iomanip>
#include <vector>
#include <algorithm>
#include "TimeAndCost.h"
#include "Edge.h"
#include <mutex>
#include <algorithm>
#include <map>

Instance* Cost_List::getInstance(int task_id){
    auto it = taskInstanceMap.find(task_id);
    if (it != taskInstanceMap.end()) {
        return it->second;
    } else {
        std::cerr << "blad dystrybutora zadan dla zadania " << task_id <<  std::endl;
        return nullptr;
    }
}

int Cost_List::getStartingTime(int task_id) {
    
    if(task_id == 0) return 0;
    int lowestTime = std::numeric_limits<int>::max();
    std::vector<int> bestPath;
    for (std::vector<int> path : TaskGraph.DFS(0, task_id)) {
        int pathTime = 0;
        bool skipPath = false; // Flaga wskazująca, czy należy pominąć bieżącą ścieżkę
        for (int t_id : path) {
            if (t_id == task_id) {
                break; // Kończymy przetwarzanie ścieżki, jeśli dotarliśmy do szukanego zadania
            } else {
                //std::cout << "ODWIEDZAM " << t_id << "\n";
                const Instance* inst = getInstance(t_id);
                if (inst == nullptr) {
                    skipPath = true;
                    //std::cout << " PRZERYWAM T" << task_id << "//";
                    break;
                }
                const Hardware* hardwarePtr = inst->getHardwarePtr();
                pathTime += times.getTime(t_id, hardwarePtr);
            }
        }
        if (skipPath) {
            continue; // Przechodzimy do następnej ścieżki, pomijając aktualną
        }
        if (pathTime < lowestTime) {
            if(task_id == 45){
                std::cout << "Mozliwy czas " << pathTime;
            }
            lowestTime = pathTime;
            bestPath = path;
        }
    }
    return lowestTime;
}


int Cost_List::getEndingTime(int task_id){
    int runningTime = times.getTime(task_id,getInstance(task_id)->getHardwarePtr());
    return getStartingTime(task_id) + runningTime;
}

std::vector<int> Cost_List::getLongestPath(int start) const {
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

    Hardware* Cost_List::getLowestTimeHardware(int task_id, int time_cost_normalized) const{
    Hardware* outHW = nullptr;
    int min_time = INF;
    for (const Hardware& hw : Hardwares) {
        int time;
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
            outHW = &const_cast<Hardware&>(hw);
        }
    }
    return outHW;
}

int Cost_List::getCriticalTime(){
    int maxTime = 0;
    for(Instance* i : Instances){
        if(getInstanceEndingTime(i)>maxTime) maxTime = getInstanceEndingTime(i);
    }
    return maxTime;
}