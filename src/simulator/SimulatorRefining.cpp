#include "TaskSchedulerSimulator.h"
#include <thread>
#include <chrono>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <mutex>
#include <map>

void Cost_List::reallocateFastest(int maxTIME,std::vector<bool>& checked){
    
    int lastFastestTaskID = -1;
    int endingTime = INF;
    for (int i = 0; i < tasks_amount; ++i) {
        if(task_schedule[i].second < endingTime && checked[i] == 0){
            bool toCheck = 1;
            if(TaskGraph.getOutNeighbourIndices(i).size()!=0){
                for(int k : TaskGraph.getOutNeighbourIndices(i)){
                    if(checked[k]==0){
                        toCheck = 0;
                    }
                }
            }
            if(toCheck){
                endingTime = task_schedule[i].second;
                lastFastestTaskID = i;
            }
        }
    }
    //std::cout << "Najszybciej konczace sie zadanie to " << lastFastestTaskID << " w czasie " << endingTime << "\n";
    
    getCriticalTime();
    Instance* bestInst = getInstance(lastFastestTaskID);
    int lowestCost = times.getCost(lastFastestTaskID,getInstance(lastFastestTaskID)->getHardwarePtr());
    for(Instance* inst : Instances){
        if(inst != getInstance(lastFastestTaskID)){
            int newCost = times.getCost(lastFastestTaskID,inst->getHardwarePtr());
            int remainTime = maxTIME - (getStartingTime(lastFastestTaskID) + times.getTime(lastFastestTaskID,inst->getHardwarePtr()));
            if(remainTime > 0 && newCost < lowestCost){
                bestInst = inst;
            }
        }
    }
    if(bestInst != getInstance(lastFastestTaskID)){
        //std::cout << "Przeniesione z " << *getInstance(lastFastestTaskID);
        removeTaskFromInstance(lastFastestTaskID);
        addTaskToInstance(lastFastestTaskID,bestInst);
        //std::cout << "Na " << *getInstance(lastFastestTaskID);

    }
    else{
        //std::cout << "nie przeniesione\n";
    }
    
    checked[lastFastestTaskID] = 1;
    
}