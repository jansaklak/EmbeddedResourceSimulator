#include "TaskSchedulerSimulator.h"
#include <thread>
#include <chrono>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <mutex>
#include <map>

void Cost_List::subTaskHandler(int taskID){
    int currHW;
    int totalCost = 0;
    int totalTime = 0;
    for(int i = 0; i<sumTasksTable.getNumSubTasks(taskID);i++){
        currHW = sumTasksTable.getFastestHW(taskID,i);
        //std::cout << "Zadanie" << taskID << "_" << i << " - przydzielam do :" << Hardwares[currHW] << std::endl;
        subTaskHW[taskID].push_back(Hardwares[currHW]);
        totalCost += sumTasksTable.getSubTaskCost(taskID,i,currHW);
        totalTime += sumTasksTable.getSubTaskTime(taskID,i,currHW);
    }
    //std::cout << "Całkowity czas podzielonego zadania " << taskID << "wynosi: " << totalTime;
    //std::cout << "\tCałkowity koszt podzielonego zadania " << taskID << "wynosi: " << totalCost << std::endl;
    times.setSubTaskTotalTime(taskID,totalTime,totalCost);
    //std::cout << "Całkowity czas podzielonego zadania " << taskID << "wynosi: " << times.getTime(taskID,&Hardwares[currHW]);
    //std::cout << "\tCałkowity koszt podzielonego zadania " << taskID << "wynosi: " << times.getCost(taskID,&Hardwares[currHW]) << std::endl;
    Instance* newInst = new Instance(HWInstancesCount[currHW],&Hardwares[currHW]);
    HWInstancesCount[currHW]++;
    taskInstanceMap[taskID] = newInst;
    newInst->addTask(taskID);
    allocated_tasks[taskID] = 1;
    Instances.push_back(newInst);
}
