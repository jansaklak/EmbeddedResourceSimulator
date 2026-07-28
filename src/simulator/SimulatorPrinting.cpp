#include "TaskSchedulerSimulator.h"
#include <thread>
#include <chrono>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <mutex>
#include <map>


void TaskSchedulerSimulator::printSchedule() {
    int instance_ending_time;
    int criticalTime = 0;

    task_schedule.clear();
    for(Instance* i : Instances) {
        instance_ending_time = 0;
        
        //std::cout << "Dla instancji " << *i;
        std::vector<std::pair<int, int>> taskWithStartingTime;
        for (int taskID : i->getTaskSet()) {
            taskWithStartingTime.push_back(std::make_pair(taskID, getStartingTime(taskID)));
        }
        std::sort(taskWithStartingTime.begin(), taskWithStartingTime.end(), [](const auto& a, const auto& b) {
            return a.second < b.second;
        });
        for (const auto& pair : taskWithStartingTime) {
            if(instance_ending_time<=pair.second){
                
                //std::cout << "Zadanie " << pair.first << " moge zaczac o " << getStartingTime(pair.first) << "a instancja jest wolna od " << instance_ending_time << "\n" ;
                std::pair<int, int> timeRunning = {getStartingTime(pair.first), getStartingTime(pair.first) + times.getTime(pair.first, i->getHardwarePtr())};
                //std::cout << "Przed wstawieniem: " << timeRunning.first << ", " << timeRunning.second << "\n";

                // Wstawienie do mapy
                auto result = task_schedule.emplace(pair.first, timeRunning);
                if (!result.second) {
                    task_schedule[pair.first] = timeRunning;
                }

                // Sprawdzenie wartości po wstawieniu do mapy
                auto it = task_schedule.find(pair.first);
                if (it != task_schedule.end()) {
                    //std::cout << "Zakonczy sie o : " << timeRunning.second << " lub " << it->second.second << "\n";
                    instance_ending_time = it->second.second;
                } else {
                    //std::cout << "Blad: nie znaleziono klucza w task_schedule\n";
                }


            }
            else{
                
                

                std::pair<int,int> timeRunning = {instance_ending_time,instance_ending_time+times.getTime(pair.first,i->getHardwarePtr())};
                task_schedule.insert({pair.first,timeRunning});
                instance_ending_time = task_schedule[pair.first].second;
            }
        }
    }

}



void printWeightTable(const WeightTable& wt) {
    std::cout << "INST: " << std::setw(1) << *wt.inst << "\t|";
    std::cout << "TCP: " << std::setw(5) << wt.TCP << "|";
    std::cout << "TC: " << std::setw(5) << wt.TC << "|";
    std::cout << "Tw: " << std::setw(5) << wt.Tw << "|";
    std::cout << "reTw: " << std::setw(5) << wt.reTw << "|";
    std::cout << "Cw: " << std::setw(5) << wt.Cw << "|";
    std::cout << "reCw: " << std::setw(5) << wt.reCw << "|";
    std::cout << "tasksOnInst: " << std::setw(5) << wt.StartingTime << "|";
    std::cout << "runTime: " << std::setw(5) << wt.runTime << "|";
    std::cout << "reCalc: " << std::setw(5) << wt.reCalc << "|";
    std::cout << "idleTime: " << std::setw(5) << wt.idleTime << "|";
    std::cout << "asBefore: " << std::setw(5) << wt.asBefore << "\n";
}

void TaskSchedulerSimulator::printTasks(std::ostream& out) const{
    out << "@tasks " << tasks_amount << std::endl;
    std::vector<int> outIdx;
    int size = tasks_amount;
    for(int i = 0; i < size; i++) {
        outIdx = TaskGraph.getOutNeighbourIndices(i);
        if(conditionalTasks.find(i) != conditionalTasks.end()){
            out << "C";
        }
        if(unpredictedTasks.find(i) != unpredictedTasks.end()){
            out << "U";
        }
        out << "T" << i << " ";
        out << outIdx.size() << " ";
        for(int j : outIdx) {
            out << j << "(" << TaskGraph.getWeightEdge(i, j) << ") ";
        }
        out << std::endl;
    }
}

void TaskSchedulerSimulator::printCOMS(std::ostream& out) const{
    out << "@comm " << Channels.size() <<"\n";
    for(COM c : Channels){
        c.printCOM(out,Hardwares);
    }
}

void TaskSchedulerSimulator::printProc(std::ostream& out) const{
    out << "@proc " << Hardwares.size() << std::endl;
    for (Hardware h : Hardwares) {
        h.printHW(out);
         out << std::endl;
    }
}

void TaskSchedulerSimulator::printConditions(std::ostream& out) const{

    out << "@conditions " << std::endl;
    
    for(int taskID : conditionalTasks){
        out << "C" << taskID << "(" << conditionTaskMap.at(taskID).variable << conditionTaskMap.at(taskID).op << conditionTaskMap.at(taskID).value << ")" << std::endl;
    }
}




void TaskSchedulerSimulator::printToGantt(std::string filename){
    std::ofstream outputFile(filename, std::ofstream::trunc);
    if (outputFile.is_open()) {
        for(int t = 0; t<tasks_amount;t++){
            if(extendedTasks.find(t) != extendedTasks.end()){
                int subTaskStart = task_schedule[t].first;
                for(int i = 0; i<sumTasksTable.getNumSubTasks(t);i++){
                    int currSubTime = sumTasksTable.getSubTaskTime(t,i,subTaskHW[t][i].getID());
                    outputFile << t << "_" << i << " " << subTaskHW[t][i] << " " << subTaskStart << " " << subTaskStart + currSubTime << std::endl;
                    subTaskStart += currSubTime;
            }
            }
            else{
                if(conditionalTasks.find(t) != conditionalTasks.end()){
                    outputFile << "C";
                }
                if(unpredictedTasks.find(t) != unpredictedTasks.end()){
                    outputFile << "U";
                }
                const Instance* inst = getInstance(t);
                if (inst != nullptr) {
                    outputFile  << t << " " << *inst << " " << task_schedule[t].first <<" " << task_schedule[t].second << std::endl;
                }
            }
        }
        std::cout << "Zapisano wykres zadań do: " << filename << std::endl;
        outputFile.close();
    }
    else {
        std::cerr << "Nie można otworzyć pliku do zapisu wykresu." << std::endl;
        return;
    }

}

void TaskSchedulerSimulator::printInstances() {
    std::sort(Instances.begin(), Instances.end());
    tasks_amount = TaskGraph.getVerticesSize();
    std::cout << "Stworzono " << Instances.size() << " komponentów\n";
    int task_id;
    int task_time;
    int totalCostOfInstances = 0;
    
    for(int tid = 0; tid<tasks_amount;++tid){
        if(unpredictedTasks.find(tid) != unpredictedTasks.end()){
            std::cout << "U";
        }
        std::cout  << "T" << tid;
        
        
        if(extendedTasks.find(tid) != extendedTasks.end()){
            std::cout << "\n";
            int subTaskStart = task_schedule[tid].first;
            for(int i = 0; i<sumTasksTable.getNumSubTasks(tid);i++){
                int currSubTime = sumTasksTable.getSubTaskTime(tid,i,subTaskHW[tid][i].getID());
                std::cout << "\tT" << tid << "_" << i << "\tna " << subTaskHW[tid][i] << "od: " << subTaskStart << "do: " << subTaskStart + currSubTime << std::endl;
                subTaskStart += currSubTime;
            }
        }
        else{
            const Instance* inst = getInstance(tid);
            if (inst != nullptr) {
                std::cout <<"\tna " <<  *inst << " od:" << task_schedule[tid].first <<" do:" << task_schedule[tid].second << std::endl;
            } else {
                std::cout <<"\t(pominięte / nieprzydzielone)" << std::endl;
            }
        }
    }

    int criticTime = getCriticalTime();
    
    for (const Instance* inst : Instances) {
        int expTime = 0;
        int expCost = 0;
        for (int taskID : inst->getTaskSet()) {
            expTime += times.getTime(taskID, inst->getHardwarePtr());
            expCost += times.getCost(taskID, inst->getHardwarePtr());
        }
        
        expCost += inst->getHardwarePtr()->getCost();
        totalCostOfInstances += expCost;
        std::cout << *inst << " Zadan: " << inst->getTaskSet().size() << " Spodziewany czas: " << expTime << " Bezczynnosci: " << getIdleTime(inst,criticTime) << " koszt: " << expCost  << " w tym początkowy: " << inst->getHardwarePtr()->getCost() << "\n";
    }
    
    std::cout << "Czas scieżki krytycznej: " << criticTime;
    std::cout << "\tKoszt całkowity instancji: " << totalCostOfInstances << std::endl;
}

void TaskSchedulerSimulator::printTotalCost() {
    std::cout << "Całkowity koszt systemu z uwzględnieniem kary: " << totalCost << std::endl;
}

void TaskSchedulerSimulator::printUnpredictedTasks(){

    if(unpredictedTasks.size() > 0){
        std::cout <<"Nieprzewidziane zadania to: ";
        for(int task : unpredictedTasks){
            std::cout << "T" << task << ", ";
        }
    }
    if(extendedTasks.size() > 0){
        std::cout <<"Rozszerzone zadania to: ";
        for(int task : extendedTasks){
            std::cout << "T" << task << ", ";
        }
    }
    if(conditionalTasks.size() > 0){
        std::cout <<"Warunkowe zadania to: ";
        for(int task : conditionalTasks){
            std::cout << "T" << task << ", ";
        }
    }
    std::cout << "\n";
}