#include "TaskSchedulerSimulator.h"
#include <thread>
#include <chrono>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <mutex>
#include <map>
#include <set>
#include <unordered_set>
#include <regex>


std::vector<Hardware> Cost_List::getHardwares() const{
    return Hardwares;
}

std::vector<COM> Cost_List::getCOMS() const{
    return Channels;
}

Graf Cost_List::getGraph() const{
    return TaskGraph;
}
Times Cost_List::getTimes() const{
    return times;
}

TaskSchedulerSimulator::~TaskSchedulerSimulator() {
    clear();
}

TaskSchedulerSimulator::TaskSchedulerSimulator(TaskSchedulerSimulator&& other) noexcept {
    with_cost = other.with_cost;
    totalCost = other.totalCost;
    tasks_amount = other.tasks_amount;
    hardware_cores_amount = other.hardware_cores_amount;
    processing_unit_amount = other.processing_unit_amount;
    channels_amount = other.channels_amount;
    TotalCost = other.TotalCost;
    simulation_time_scale = other.simulation_time_scale;
    Hardwares = std::move(other.Hardwares);
    Channels = std::move(other.Channels);
    allocated_tasks = std::move(other.allocated_tasks);
    progress = std::move(other.progress);
    HWtoTasks = std::move(other.HWtoTasks);
    Instances = std::move(other.Instances);
    other.Instances.clear();
    paths = std::move(other.paths);
    HWInstancesCount = std::move(other.HWInstancesCount);
    taskInstanceMap = std::move(other.taskInstanceMap);
    task_schedule = std::move(other.task_schedule);
    conditions = std::move(other.conditions);
    conditionTaskMap = std::move(other.conditionTaskMap);
    subTaskHW = std::move(other.subTaskHW);
    CostListConfig = std::move(other.CostListConfig);
    unpredictedTasks = std::move(other.unpredictedTasks);
    conditionalTasks = std::move(other.conditionalTasks);
    extendedTasks = std::move(other.extendedTasks);
    sumTasksTable = std::move(other.sumTasksTable);
    TaskGraph = std::move(other.TaskGraph);
    times = std::move(other.times);
}

TaskSchedulerSimulator& TaskSchedulerSimulator::operator=(TaskSchedulerSimulator&& other) noexcept {
    if (this != &other) {
        clear();
        with_cost = other.with_cost;
        totalCost = other.totalCost;
        tasks_amount = other.tasks_amount;
        hardware_cores_amount = other.hardware_cores_amount;
        processing_unit_amount = other.processing_unit_amount;
        channels_amount = other.channels_amount;
        TotalCost = other.TotalCost;
        simulation_time_scale = other.simulation_time_scale;
        Hardwares = std::move(other.Hardwares);
        Channels = std::move(other.Channels);
        allocated_tasks = std::move(other.allocated_tasks);
        progress = std::move(other.progress);
        HWtoTasks = std::move(other.HWtoTasks);
        Instances = std::move(other.Instances);
        other.Instances.clear();
        paths = std::move(other.paths);
        HWInstancesCount = std::move(other.HWInstancesCount);
        taskInstanceMap = std::move(other.taskInstanceMap);
        task_schedule = std::move(other.task_schedule);
        conditions = std::move(other.conditions);
        conditionTaskMap = std::move(other.conditionTaskMap);
        subTaskHW = std::move(other.subTaskHW);
        CostListConfig = std::move(other.CostListConfig);
        unpredictedTasks = std::move(other.unpredictedTasks);
        conditionalTasks = std::move(other.conditionalTasks);
        extendedTasks = std::move(other.extendedTasks);
        sumTasksTable = std::move(other.sumTasksTable);
        TaskGraph = std::move(other.TaskGraph);
        times = std::move(other.times);
    }
    return *this;
}

TaskSchedulerSimulator::TaskSchedulerSimulator(){ 
    tasks_amount = 0;
    hardware_cores_amount = 0;
    processing_unit_amount = 0;
    channels_amount = 0;
    with_cost = 0;
    clear();
}

TaskSchedulerSimulator::TaskSchedulerSimulator(int _tasks, int _hcores, int _punits, int _channels, int _withCost)
    : with_cost(_withCost), tasks_amount(_tasks), hardware_cores_amount(_hcores),
    processing_unit_amount(_punits), channels_amount(_channels) {
        clear();
}

void Cost_List::clear(){
    for (Instance* inst : Instances) {
        delete inst;
    }
    Instances.clear();
    Hardwares.clear();
    Channels.clear();
    times = Times();
    TaskGraph = Graf();
    HWInstancesCount.clear();
    taskInstanceMap.clear();
    HWtoTasks.clear();
    task_schedule.clear();
    allocated_tasks.clear();
    progress.clear();
    unpredictedTasks.clear();
    conditionalTasks.clear();
    extendedTasks.clear();
    subTaskHW.clear();
}


void Cost_List::clearNUM(){
    int tasks_amount = 0;
    int hardware_cores_amount = 0;
    int processing_unit_amount = 0;
    int channels_amount = 0;
    bool with_cost = 0;
    int TotalCost = 0;
    int simulation_time_scale = 0;
}
    



void Cost_List::randALL(){
    times = Times(tasks_amount);
    createRandomTasksGraph();
    createRandomProc();
    times.LoadHW(Hardwares);
    times.setRandomTimesAndCosts();
    connectRandomCH();
}

void Cost_List::printALL(std::string filename,bool toScreen) const{
    std::ofstream outputFile(filename, std::ofstream::trunc);
    if (outputFile.is_open()) {
        // std::ofstream outputFileMatrix("matrix.dat", std::ofstream::trunc);
        // TaskGraph.printMatrix(outputFileMatrix);
    //@tasks
        if(toScreen) printTasks();
        printTasks(outputFile);
    //@proc
        if(toScreen) printProc();
        printProc(outputFile);
    //@times @cost
        if(toScreen) times.show();
        times.show(outputFile);
    //@coms
        printCOMS(outputFile);
        if(toScreen) printCOMS();
        if(conditionalTasks.size() > 0){
            printConditions(outputFile);
            if(toScreen) printConditions();
        }
        outputFile.close();
        std::cout << "Zapisano liste do pliku " << filename << std::endl;
    }
    else {
        std::cerr << "Nie można otworzyć pliku do zapisu." << std::endl;
        return;
    }
}

int Cost_List::createRandomProc() {
    Hardwares.clear();
    if(hardware_cores_amount < 1 || processing_unit_amount < 1){
        std::cerr << "Błedna liczba HC lub PU\n";
        return -1;
    }
    for(int i = 0; i<hardware_cores_amount;i++){
        Hardwares.push_back(Hardware(getRand(5)+1,Hardware_Type::HC,Hardwares.size()));
    }
    int HC_size = hardware_cores_amount;
    for(int j = 0; j<processing_unit_amount;j++){
        Hardwares.push_back(Hardware(getRand(5)+1,Hardware_Type::PE,Hardwares.size()-HC_size));
    }
    return 1;
}

void Cost_List::connectRandomCH(){
    if(channels_amount < 1){
        std::cerr<< "Zła liczba kanałów";
        return;
    }
    int bd = ((getRand(9)+1)*10);
    int con_cost = (getRand(9)+1)*5;
    int rd;
    COM c = COM(bd,con_cost,Channels.size());
    
    // for(Hardware h : Hardwares){
    //         std::cout << "DODAJE" << h;
    //         c.add_Hardware(&h);
    // }
    Channels.push_back(c);
    if(channels_amount>1){
        for(int j =1;j<channels_amount;j++){
            bd = ((getRand(9)+1)*10);
            con_cost = (getRand(9)+1)*5;
            COM c = COM(bd,con_cost,Channels.size());
            for(int k = 0;k<(Hardwares.size()/2);k++){
                if(randomBool(3)){
                    Hardware randHW = Hardwares[getRand(Hardwares.size() - 1)];
                    Channels[0].remove_Hardware(&randHW);
                    c.add_Hardware(&randHW);
                }
            }
            Channels.push_back(c);
        }
    }
    for(COM c : Channels){
        if(c.getSize()<1){
            Hardware randHW = Hardwares[getRand(Hardwares.size() - 1)];
            while(1){
                randHW = Hardwares[getRand(Hardwares.size() - 1)];
                if(!c.isConnected(&randHW)) break;
            }
            c.add_Hardware(&randHW);
            while(1){
                randHW = Hardwares[getRand(Hardwares.size() - 1)];
                if(!c.isConnected(&randHW)) break;
            }
            c.add_Hardware(&randHW);
        }
    }

    return;
}

void Cost_List::runTasks() {
    int TotalCost = 0;
    simulation_time_scale = 1;
    progress.resize(TaskGraph.getVerticesSize(),-2); // -2 - cant be done flag
    progress[0] = -1;
    std::vector<std::thread> threads;
    int numThreads = Instances.size();
    bool stop = false;
    int czas = 0;
    std::thread counterThread(Licznik, std::ref(stop), std::ref(czas));
    std::cout << "\nUruchamiam zadania w skali x" << simulation_time_scale << ":\n";
    for (Instance* i : Instances) threads.push_back(std::thread(&Cost_List::TaskRunner, this, *i));
    for (std::thread& thread : threads) thread.join();
    stop = true;
    counterThread.join();
    threads.clear();
    std::cout << "\n\nCzas trwania programu: " << czas << " milisekund. (skala x" << simulation_time_scale << ") \n\n";
}


void Cost_List::unpredictedHandler(int task_ID){
    if (allocated_tasks[task_ID] == 1) {
                return;
    }
    std::cout <<" Nie przewidziane jest: " << task_ID << "\n";
    int min_time = INF;
    int time;
    Instance* bestFoundInst = nullptr;
    for (Instance* inst : Instances) {  //Znajdz najtanszy z obecnych Inst
        if(inst->getHardwarePtr()->getType()=="PE"){
            time = times.getTime(task_ID, inst->getHardwarePtr());
            if (time < min_time) {
                min_time = time;
                bestFoundInst = inst;
                std::cout <<" ZNalazlem : " << *bestFoundInst << "\n";
            }
        }
    }
    if (bestFoundInst == nullptr){
        Hardware* bestPE = nullptr;
        int maxTime = 0;
        for (const Hardware& hw : Hardwares) {
            if (hw.getType()=="PE" && times.getTime(task_ID, &hw) > maxTime) {
                maxTime = times.getTime(task_ID, &hw);
                bestPE = &const_cast<Hardware&>(hw);
            }
        }
        bestFoundInst = new Instance(HWInstancesCount[bestPE->getID()], bestPE);
        HWInstancesCount[bestPE->getID()]++;
        Instances.push_back(bestFoundInst);
    }
    bestFoundInst->addTask(task_ID);
    taskInstanceMap[task_ID] = bestFoundInst;
    allocated_tasks[task_ID] = 1;
}



int Cost_List::createInstance(int task_ID, const Hardware* h) {
    
    if(allocated_tasks[task_ID]==1){
        return 0;
    }
    if(unpredictedTasks.find(task_ID) != unpredictedTasks.end()){
        unpredictedHandler(task_ID);
        return -2;
    }
    if(extendedTasks.find(task_ID) != extendedTasks.end()){
        subTaskHandler(task_ID);
        return -3;
    }
    Instance* newInst = new Instance(HWInstancesCount[h->getID()], h);
    HWInstancesCount[h->getID()]++;
    newInst->addTask(task_ID);
    //std::cout << "DLA ZAD" << task_ID << "STWORZONO" << *newInst << std::endl;
    Instances.push_back(newInst);
    taskInstanceMap[task_ID] = newInst;
    allocated_tasks[task_ID] = 1;
    //std::cout << "\n\n\ntask instnace map dla " << task_ID << " :: " << taskInstanceMap[task_ID] << "\n\n\n";
    return 1;
}

int Cost_List::createInstance(int task_ID) {
    if (allocated_tasks[task_ID] == 1) {
                return 0;
    }
    if(unpredictedTasks.find(task_ID) != unpredictedTasks.end()){
        unpredictedHandler(task_ID);
        return -2;
    }
    if(extendedTasks.find(task_ID) != extendedTasks.end()){
        subTaskHandler(task_ID);
        return -3;
    }
    Hardware* h = getLowestTimeHardware(task_ID, 0);
    Instance* newInst = new Instance(HWInstancesCount[h->getID()], h);
    HWInstancesCount[h->getID()]++;
    newInst->addTask(task_ID);
    Instances.push_back(newInst);
    taskInstanceMap[task_ID] = newInst;
    
    allocated_tasks[task_ID] = 1;
    return 1;
}

void Cost_List::addTaskToInstance(int task_ID, Instance* inst) {
    if(unpredictedTasks.find(task_ID) != unpredictedTasks.end()){
        unpredictedHandler(task_ID);
        return;
    }
    if(extendedTasks.find(task_ID) != extendedTasks.end()){
        subTaskHandler(task_ID);
        return;
    }
    if(getInstance(task_ID) != nullptr){
        removeTaskFromInstance(task_ID);
    }
    int startingTimeNewTask = getStartingTime(task_ID);
    if(startingTimeNewTask<getInstanceStartingTime(inst)){
        std::set<int> new_set;
        new_set.insert(task_ID);
        for(int tid : inst->getTaskSet()){
            new_set.insert(tid);
        }
        inst->setTasksSet(new_set);
    }
    else{
        inst->addTask(task_ID);
    }
    taskInstanceMap.erase(task_ID);

    taskInstanceMap[task_ID] = inst;
    allocated_tasks[task_ID] = 1;
    return;

    
}


void Cost_List::removeTaskFromInstance(int task_ID){
    if(unpredictedTasks.find(task_ID) != unpredictedTasks.end()){
        unpredictedHandler(task_ID);
        return;
    }
    taskInstanceMap[task_ID]->removeTask(task_ID);
    if(taskInstanceMap[task_ID]->getTaskSet().empty()){
        Instances.erase(std::remove(Instances.begin(), Instances.end(), taskInstanceMap[task_ID]), Instances.end());
    }
    allocated_tasks[task_ID] = 0;
    taskInstanceMap.erase(task_ID);
    return;
}

std::vector<int> Cost_List::findAllToSkipAfterConditional(int taskID) {
    std::unordered_set<int> visited;  // Track visited nodes
    std::unordered_set<int> toSkip;   // Nodes to skip
    std::queue<int> queue;
    queue.push(taskID);
    visited.insert(taskID);

    while (!queue.empty()) {
        int current = queue.front();
        queue.pop();

        // Check all outgoing neighbors of the current node
        for (int neighbor : TaskGraph.getOutNeighbourIndices(current)) {
            // If the neighbor is not visited
            if (visited.find(neighbor) == visited.end()) {
                visited.insert(neighbor);

                // Check all incoming neighbors of this neighbor
                bool onlyDependentOnConditional = true;
                for (int inNeighbor : TaskGraph.getInNeighbourIndices(neighbor)) {
                    if (inNeighbor != taskID && visited.find(inNeighbor) == visited.end()) {
                        onlyDependentOnConditional = false;
                        break;
                    }
                }

                if (onlyDependentOnConditional) {
                    toSkip.insert(neighbor);
                    queue.push(neighbor);
                }
            }
        }
    }

    return std::vector<int>(toSkip.begin(), toSkip.end());
}

void Cost_List::TaskRunner(Instance inst) {
    int taskCounter = 0;
    const Hardware* running_hw = inst.getHardwarePtr();
    std::set<int> toDo = inst.getTaskSet();

    while (toDo.size() > 0) {
        std::vector<int> toRemove;
        for (int t : toDo) {
            if (progress[t] == -1) {
                if (conditionalTasks.find(t) != conditionalTasks.end() && evaluateCondition(t) == 0) {
                    std::cout << "\tCT" << t << " skipped\n";
                    progress[t] = 2;
                    ++taskCounter;
                    for (int taskToSkip : findAllToSkipAfterConditional(t)) {
                        std::cout << "\tT" << taskToSkip << " skipped (after CT " << t << ")\n";
                        progress[taskToSkip] = 20;
                    }
                } else {
                    int time = times.getTime(t, running_hw);
                    {
                        std::cout << "T" << t << " on " << inst << std::endl;
                        progress[t] = 0;
                        for (int k = 1; k < time + 1; ++k) {
                            progress[t] = (((k + 1) * 100) / time);
                            std::this_thread::sleep_for(std::chrono::milliseconds(simulation_time_scale));
                        }
                        ++taskCounter;
                        std::cout << "\tT" << t << " done\n";
                    }
                }
                for (int toRun : TaskGraph.getOutNeighbourIndices(t)) {
                    if (progress[toRun] == -2) progress[toRun] = -1;
                }
            }
            if (progress[t] > 0) {
                toRemove.push_back(t);
            }
        }

        for (int t : toRemove) {
            toDo.erase(t);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    std::cout << "\t\t\t" << inst << " ended\n";
    return;
}

int extractTaskIdFromDoneCondition(const std::string& variable) {
    std::regex done_regex(R"(DONE\[(\d+)\])");
    std::smatch match;
    if (std::regex_search(variable, match, done_regex) && match.size() > 1) {
        return std::stoi(match.str(1));
    }
    return -1;
}


int Cost_List::evaluateCondition(int task_id) {
    std::string variable = conditionTaskMap[task_id].variable;
    std::string op = conditionTaskMap[task_id].op;
    int value_int = conditionTaskMap[task_id].value;
    int actual_value_int;
    if (variable == "CRITICAL_TIME") {
        actual_value_int = getCriticalTime();
        if(Instances.size() == 0){
            return -1;
        }
    } else if (variable == "TOTAL_COST") {
        actual_value_int = totalCost;
        if(Instances.size() == 0){
            return -1;
        }
    } else if (variable == "HC_INSTANCES") {
        actual_value_int = hardware_cores_amount;
        if(Instances.size() == 0){
            return -1;
        }
    } else if (variable == "PE_INSTANCES") {
        actual_value_int = processing_unit_amount;
        if(Instances.size() == 0){
            return -1;
        }
    } else if (variable == "TOTAL_INSTANCES") {
        actual_value_int = Hardwares.size();
        if(Hardwares.size() == 0){
            return -1;
        }
    } else if (variable == "CURR_TIME") {
        actual_value_int = task_schedule[task_id].first;
        if(task_id != 0 && actual_value_int <= 0){
            return -1;
        }
    }
    else if (variable.find("DONE[") == 0) {
        int done_task_id = extractTaskIdFromDoneCondition(variable);
        if(progress.size() < done_task_id){
            return -1;
        }
        else{
            actual_value_int = progress[done_task_id];
        }
        
    }

     else {
        auto it = CostListConfig.find(variable);
        if (it == CostListConfig.end()) {
            
            std::cerr << "Error: Variable " << variable << " not found in config." << std::endl;
            return 0;
        }
        std::string actual_value = it->second;
        actual_value_int = std::stoi(actual_value);
    }

    if (op == ">" && actual_value_int > value_int) {
        return 1;
    } else if (op == ">=" && actual_value_int >= value_int) {
        return 1;
    } else if (op == "<" && actual_value_int < value_int) {
        return 1;
    } else if (op == "<=" && actual_value_int <= value_int) {
        return 1;
    } else if (op == "==" && actual_value_int == value_int) {
        return 1;
    }

    return 0;
}

void Cost_List::calculateTotalCost() {
    totalCost = 0;
    int criticTime = getCriticalTime();
    int HARD_TIME = 250;
    int PUNISHMENT = 2;

    for (const Instance* instance : Instances) {
        if (!instance || !instance->getHardwarePtr()) continue;
        totalCost += instance->getHardwarePtr()->getCost();
        for (int taskID : instance->getTaskSet()) {
            totalCost += times.getCost(taskID, instance->getHardwarePtr());
        }
    }
    if (criticTime > HARD_TIME) {
        totalCost += (criticTime - HARD_TIME) * PUNISHMENT;
    }
}

void Cost_List::exportToJSON(std::ostream& out) const {
    out << "{\n";
    out << "  \"tasksCount\": " << tasks_amount << ",\n";
    out << "  \"hardwareCount\": " << Hardwares.size() << ",\n";
    out << "  \"channelsCount\": " << Channels.size() << ",\n";
    out << "  \"criticalTime\": " << getCriticalTime() << ",\n";
    out << "  \"totalCost\": " << totalCost << ",\n";

    // Tasks list
    out << "  \"tasks\": [\n";
    int size = TaskGraph.getVerticesSize();
    for (int i = 0; i < size; ++i) {
        out << "    {\n";
        out << "      \"id\": " << i << ",\n";
        out << "      \"isConditional\": " << (conditionalTasks.count(i) ? "true" : "false") << ",\n";
        out << "      \"isUnpredicted\": " << (unpredictedTasks.count(i) ? "true" : "false") << ",\n";
        out << "      \"isExtended\": " << (extendedTasks.count(i) ? "true" : "false") << ",\n";
        if (conditionTaskMap.count(i)) {
            const auto& cond = conditionTaskMap.at(i);
            out << "      \"condition\": \"" << cond.variable << " " << cond.op << " " << cond.value << "\",\n";
        } else {
            out << "      \"condition\": null,\n";
        }
        out << "      \"outEdges\": [";
        auto outIdx = TaskGraph.getOutNeighbourIndices(i);
        for (size_t j = 0; j < outIdx.size(); ++j) {
            out << "{\"target\": " << outIdx[j] << ", \"weight\": " << TaskGraph.getWeightEdge(i, outIdx[j]) << "}";
            if (j + 1 < outIdx.size()) out << ", ";
        }
        out << "]\n";
        out << "    }" << (i + 1 < size ? "," : "") << "\n";
    }
    out << "  ],\n";

    // Hardwares
    out << "  \"hardware\": [\n";
    for (size_t i = 0; i < Hardwares.size(); ++i) {
        out << "    {\"id\": " << Hardwares[i].getID() << ", \"type\": \"" << Hardwares[i].getType() << "\", \"cost\": " << Hardwares[i].getCost() << "}" << (i + 1 < Hardwares.size() ? "," : "") << "\n";
    }
    out << "  ],\n";

    // Schedule
    out << "  \"schedule\": [\n";
    size_t count = 0;
    for (const auto& p : task_schedule) {
        int tid = p.first;
        int startTime = p.second.first;
        int endTime = p.second.second;
        auto instIt = taskInstanceMap.find(tid);
        std::string instStr = (instIt != taskInstanceMap.end() && instIt->second && instIt->second->getHardwarePtr()) 
                              ? (instIt->second->getHardwarePtr()->getType() + std::to_string(instIt->second->getHardwarePtr()->getID()) + "_" + std::to_string(instIt->second->getID())) 
                              : "HW";
        int hwId = (instIt != taskInstanceMap.end() && instIt->second && instIt->second->getHardwarePtr()) 
                   ? instIt->second->getHardwarePtr()->getID() : 0;
        
        out << "    {\n";
        out << "      \"taskId\": " << tid << ",\n";
        out << "      \"startTime\": " << startTime << ",\n";
        out << "      \"endTime\": " << endTime << ",\n";
        out << "      \"hwId\": " << hwId << ",\n";
        out << "      \"unit\": \"" << instStr << "\"\n";
        out << "    }" << (++count < task_schedule.size() ? "," : "") << "\n";
    }
    out << "  ]\n";
    out << "}\n";
}

void Cost_List::exportToJSONFile(const std::string& filename) const {
    std::ofstream outFile(filename);
    if (outFile.is_open()) {
        exportToJSON(outFile);
        outFile.close();
    }
}





//     // std::cout << "Porządek topologiczny: * - stan końcowy, () - czas na najszybszym HW\n";
//     // for(int i : getLongestPath(0)){
//     //     std::cout << i <<"(" << times.getTime(i,getLowestTimeHardware(i,0)) <<")";
//     //     if(TaskGraph.getOutNeighbourIndices(i).size()==0){
//     //         std::cout <<"*";
//     //     }
//     //     std::cout <<"-";
//     // }
//     std::cout << "\n";
//     return;
// }





// std::mutex removeMutex;
// std::mutex addMutex;
// std::condition_variable taskCV;

// std::mutex tasksCounterMutex;

// void Cost_List::TaskProgress(int task_id, int time, int hw_id) {

//     this->progress[task_id] = 1;

//     for (int i = 0; i < time; ++i) {
//         progress[task_id] = ((i + 1) * 100) / time;
//         std::this_thread::sleep_for(std::chrono::milliseconds(1));
//     }

//     addMutex.lock();
//     for(int i : TaskGraph.getOutNeighbourIndices(task_id)){
//         if(!progress[i]>0){
//             tasksToAdd.push_back(i);
//         }
//     }
//     addMutex.unlock();

//     removeMutex.lock();
//     tasksToRemove.push_back(task_id);
//     removeMutex.unlock();


//     HWinUse[hw_id] = 0;
// }



// void Cost_List::ReadProgress() {
//     while(true){
//         for(int i : progress){
//         std::cout << "Task" << i << " ON " << progress[i] << "%\n";
//         }
//         std::this_thread::sleep_for(std::chrono::milliseconds(500));
//     }

// }


    // int startingTimeNewTask = getStartingTime(task_ID);

    // // Temporary set to store tasks in sorted order
    // std::set<int> sortedTaskSet;

    // // Flag to check if the new task has been inserted
    // bool taskInserted = false;
    // for (int taskId : inst->getTaskSet()) {
    //     int startingTimeCurrTask = getStartingTime(taskId);
    //     if (startingTimeNewTask < startingTimeCurrTask && !taskInserted) {
    //         sortedTaskSet.insert(task_ID);
    //         taskInserted = true;
    //     }
    //     sortedTaskSet.insert(taskId);
    // }
    // if (!taskInserted) {
    //     sortedTaskSet.insert(task_ID);
    // }
    // inst->setTasksSet(sortedTaskSet);
    // taskInstanceMap[task_ID] = inst;

