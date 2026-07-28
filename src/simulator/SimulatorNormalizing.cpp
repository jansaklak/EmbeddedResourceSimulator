#include "TaskSchedulerSimulator.h"
#include <thread>
#include <chrono>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <mutex>
#include <map>


void normalize(std::vector<WeightTable>& values) {
    if (values.empty()) return;

    double min_TCP = std::numeric_limits<double>::max();
    double max_TCP = std::numeric_limits<double>::min();

    double min_TC = std::numeric_limits<double>::max();
    double max_TC = std::numeric_limits<double>::min();

    double min_Tw = std::numeric_limits<double>::max();
    double max_Tw = std::numeric_limits<double>::min();

    double min_ReTw = std::numeric_limits<double>::max();
    double max_ReTw = std::numeric_limits<double>::min();

    double min_Cw = std::numeric_limits<double>::max();
    double max_Cw = std::numeric_limits<double>::min();

    double min_ReCw = std::numeric_limits<double>::max();
    double max_ReCw = std::numeric_limits<double>::min();

    double min_tasksOnInst = std::numeric_limits<double>::max();
    double max_tasksOnInst = std::numeric_limits<double>::min();

    double min_runTime = std::numeric_limits<double>::max();
    double max_runTime = std::numeric_limits<double>::min();

    double min_reCalc = std::numeric_limits<double>::max();
    double max_reCalc = std::numeric_limits<double>::min();

    double min_idleTime = std::numeric_limits<double>::max();
    double max_idleTime = std::numeric_limits<double>::min();

    for (const auto& wt : values) {
        if (wt.TCP < min_TCP) min_TCP = wt.TCP;
        if (wt.TCP > max_TCP) max_TCP = wt.TCP;

        if (wt.TC < min_TC) min_TC = wt.TC;
        if (wt.TC > max_TC) max_TC = wt.TC;

        if (wt.Tw < min_TC) min_TC = wt.Tw;
        if (wt.Tw > max_Tw) max_Tw = wt.Tw;

        if (wt.reTw < min_ReTw) min_ReTw = wt.reTw;
        if (wt.reTw > max_ReTw) max_ReTw = wt.reTw;

        if (wt.Cw < min_Cw) min_Cw = wt.Cw;
        if (wt.Cw > max_Cw) max_Cw = wt.Cw;

        if (wt.reCw < min_ReCw) min_ReCw = wt.reCw;
        if (wt.reCw > max_ReCw) max_ReCw = wt.reCw;

        if (wt.StartingTime < min_tasksOnInst) min_tasksOnInst = wt.StartingTime;
        if (wt.StartingTime > max_tasksOnInst) max_tasksOnInst = wt.StartingTime;

        if (wt.runTime < min_runTime) min_runTime = wt.runTime;
        if (wt.runTime > max_runTime) max_runTime = wt.runTime;

        if (wt.reCalc < min_reCalc) min_reCalc = wt.reCalc;
        if (wt.reCalc > max_reCalc) max_reCalc = wt.reCalc;

        if (wt.idleTime < min_idleTime) min_idleTime = wt.idleTime;
        if (wt.idleTime > max_idleTime) max_idleTime = wt.idleTime;
    }

    for (auto& wt : values) {
        wt.TCP = (wt.TCP - min_TCP) / (max_TCP - min_TCP)                                           * 0.2;
        wt.TC = (wt.TC - min_TC) / (max_TC - min_TC)                                                * 0.1;
        wt.Tw = (wt.Tw - min_Tw) / (max_Tw - min_Tw)                                                * 0.025;
        wt.reTw = (wt.reTw - min_ReTw) / (max_ReTw - min_ReTw)                                      * 0.025;
        wt.Cw = (wt.Cw - min_Cw) / (max_Cw - min_Cw)                                                * 0.025;
        wt.reCw = (wt.reCw - min_ReCw) / (max_ReCw - min_ReCw)                                      * 0.025;
        wt.StartingTime = (wt.StartingTime - min_tasksOnInst) / (max_tasksOnInst - min_tasksOnInst) * 0.05;
        wt.runTime = (wt.runTime - min_runTime) / (max_runTime - min_runTime)                       * 0.1;
        wt.reCalc = (wt.reCalc - min_reCalc) / (max_reCalc - min_reCalc)                            * 0.3;
        //wt.idleTime = (wt.idleTime - min_idleTime) / (max_idleTime - min_idleTime)                * 0.1;
        wt.idleTime = (max_idleTime - wt.idleTime) / (max_idleTime - min_idleTime)                  * 0.1;
        wt.asBefore = wt.asBefore                                                                   * 0.05;
        wt.SUM = wt.TCP + wt.TC + wt.Tw + wt.reTw + wt.Cw + wt.StartingTime + wt.runTime + wt.reCalc + wt.idleTime + wt.asBefore;                                                                  
    } 
}

void TaskSchedulerSimulator::constructByWeight(std::vector<int> bfs_tasks,int MAX_TIME){
    std::vector<Instance*> possibleInstances;
    int i = tasks_amount * 10;
    for (const Hardware& hw : Hardwares){
        const Hardware* hw_ptr = &hw;
        Instance* newInst = new Instance(i,hw_ptr,1);
        possibleInstances.push_back(newInst);
        ++i;
    }
    //std::cout << "ZROBIONE";
    for(int task_id : bfs_tasks){
        std::cout << "\n\t ZADANIE" << task_id << "\n";
        std::vector<WeightTable> weightsTable;
        for(Instance* inst : possibleInstances){
            WeightTable wt = (
                WeightTable{inst,time_cost_proc(task_id,inst),time_cost(task_id,inst),time_weight(task_id,inst),
                        reuse_time_weight(task_id,inst),cost_weight(task_id,inst),allocated_cost(task_id,inst,MAX_TIME),
                        inst_starting(task_id,inst),inst_time_running(task_id,inst),
                        reCalculate(task_id,inst),longestIdle(task_id,inst),asBefore(task_id,inst),0}
            );
            weightsTable.push_back(wt);
            //printWeightTable(wt);
        }
        normalize(weightsTable);
        Instance* bestCurr;
        double best_weight = 0.0;
        for(WeightTable w : weightsTable){
            //std::cout << *w.inst << "\t\tPunkty: " << w.SUM << "\n";
            if(w.SUM > best_weight){
                best_weight = w.SUM;
                bestCurr = w.inst;
            }
        }
        //std::cout << " NAJLEPSZA INSTANCJA TO: " << *bestCurr;
        //std::cout << " JEJ HW TO: " << *bestCurr->getHardwarePtr();
        //std::cout << " JEJ TYP TO: " << bestCurr->isVirtual();
        bool isVirtual = bestCurr->isVirtual();
        const Hardware* currHw = bestCurr->getHardwarePtr();
        if(!isVirtual){
            addTaskToInstance(task_id,bestCurr);
        }
        else{
            createInstance(task_id,bestCurr->getHardwarePtr());
            possibleInstances.push_back(getInstance(task_id));
        }
    }
}

void TaskSchedulerSimulator::getCurrWeight(int task_id,bool changeInstances,int MAX_TIME){
    //std::cout<<"\nDLA ZADANIA " << task_id << "\n";
    double remaining_time;
    double longest_running;
    double least_running;
    double least_time_running;
    double previous_weight;
    std::map<Hardware,double> HWtoWeight;
    std::vector<Instance*> possibleInstances;
    possibleInstances = Instances;
    int i = tasks_amount * 10;
    for (const Hardware& hw : Hardwares){
        const Hardware* hw_ptr = &hw;
        Instance* newInst = new Instance(i,hw_ptr,1);
        possibleInstances.push_back(newInst);
        ++i;
    }
    std::vector<WeightTable> weightsTable;
    for(Instance* inst : possibleInstances){
        WeightTable wt = (
            WeightTable{inst,time_cost_proc(task_id,inst),time_cost(task_id,inst),time_weight(task_id,inst),
                    reuse_time_weight(task_id,inst),cost_weight(task_id,inst),allocated_cost(task_id,inst,MAX_TIME),
                    inst_starting(task_id,inst),inst_time_running(task_id,inst),
                    reCalculate(task_id,inst),longestIdle(task_id,inst),asBefore(task_id,inst),0}
        );
        weightsTable.push_back(wt);
        //printWeightTable(wt);
    }
    normalize(weightsTable);
    Instance* bestCurr;
    double best_weight = INF;
    for(WeightTable w : weightsTable){
        //std::cout << *w.inst << "\t\tPunkty: " << w.SUM << "\n";
        if(w.SUM < best_weight){
            best_weight = w.SUM;
            bestCurr = w.inst;
        }
    }
    //std::cout << " NAJLEPSZA INSTANCJA TO: " << *bestCurr;
    //std::cout << " JEJ HW TO: " << *bestCurr->getHardwarePtr();
    //std::cout << " JEJ TYP TO: " << bestCurr->isVirtual();
    bool isVirtual = bestCurr->isVirtual();
    const Hardware* currHw = bestCurr->getHardwarePtr();
    
    if(bestCurr==getInstance(task_id) || changeInstances==0){
        for (Instance* inst : possibleInstances) {
            if (inst && inst->isVirtual()) delete inst;
        }
        return;
    }
    if(!bestCurr->isVirtual()){
        removeTaskFromInstance(task_id);
        addTaskToInstance(task_id,bestCurr);
        for (Instance* inst : possibleInstances) {
            if (inst && inst->isVirtual()) delete inst;
        }
        return;
    }
    else{
        removeTaskFromInstance(task_id);
        createInstance(task_id,currHw);
        for (Instance* inst : possibleInstances) {
            if (inst && inst->isVirtual()) delete inst;
        }
        return;
    }  
}

double TaskSchedulerSimulator::time_cost_proc(int task_id,const Instance* inst,double t_factor,double c_factor,double p_factor){ //WIECEJ - GORZEJ
    double val;
    val = times.getTime(task_id,inst->getHardwarePtr()) * t_factor + times.getCost(task_id,inst->getHardwarePtr()) * c_factor;
    if(inst->isVirtual()){
         val += (inst->getHardwarePtr()->getCost() * p_factor);
    }
    return val;
}

double TaskSchedulerSimulator::time_cost(int task_id,const Instance* inst){ //WIECEJ - GORZEJ
    return times.getTime(task_id,inst->getHardwarePtr()) * 1.0 + times.getCost(task_id,inst->getHardwarePtr());
}

double TaskSchedulerSimulator::time_weight(int task_id,const Instance* inst){ //WIECEJ - GORZEJ
    return times.getTime(task_id,inst->getHardwarePtr());
}

double TaskSchedulerSimulator::reuse_time_weight(int task_id,const Instance* inst){ //WIECEJ - GORZEJ
    if(inst->isVirtual()) return INF;
    return times.getTime(task_id,inst->getHardwarePtr());    
}

double TaskSchedulerSimulator::cost_weight(int task_id,const Instance* inst){ //WIECEJ - GORZEJ
    return times.getCost(task_id,inst->getHardwarePtr());
}

double TaskSchedulerSimulator::allocated_cost(int task_id,const Instance* inst,double MAX_TIME){ //WIECEJ - GORZEJ
    return time_cost_proc(task_id,inst,getCriticalTime()/MAX_TIME); //MAX_TIME rosnie to coraz mniej istotny czas
}

double TaskSchedulerSimulator::inst_starting(int task_id,const Instance* inst){ //WIECEJ - GORZEJ
    if(inst->isVirtual()) return INF;
    return inst->getTaskSet().size();
}

double TaskSchedulerSimulator::inst_time_running(int task_id,const Instance* inst){ //WIECEJ - GORZEJ
    if(inst->isVirtual()) return INF;
    return getTimeRunning(inst);
}

double TaskSchedulerSimulator::reCalculate(int task_id,const Instance* inst){ 
    return 0;
}

double TaskSchedulerSimulator::longestIdle(int task_id,const Instance* inst){ //WIECEJ - LEPIEJ
    if(inst->isVirtual()) return 0;
    return getIdleTime(inst,getStartingTime(task_id));
}

double TaskSchedulerSimulator::asBefore(int task_id,const Instance* inst){ //WIECEJ - GORZEJ
    if(inst->isVirtual()){
        return 1;
    }
    for(int i : TaskGraph.getInNeighbourIndices(task_id)){
        if(getInstance(i) == inst){
            return 0;
        }
    }
    return 1;
}


