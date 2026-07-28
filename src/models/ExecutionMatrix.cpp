
#include "HardwareProcessor.h"
#include "ExecutionMatrix.h"
#include <cstdlib>
#include <vector>
#include <iostream>
#include <algorithm>


ExecutionMatrix::ExecutionMatrix(){
    clear();
    return;
}

void ExecutionMatrix::clear(){
    HW_vec.clear();
    times_matrix.clear();
    cost_matrix.clear();
    normalized_matrix.clear();
    return;
}

void ExecutionMatrix::LoadHW(std::vector<HardwareProcessor> _HW_vec){
    HW_vec = _HW_vec;
}

ExecutionMatrix::ExecutionMatrix(int _size) {
    clear();
    graph_size = _size;
    return;
}

void ExecutionMatrix::setTimesMatrix(std::vector<std::vector<int>> _times_matrix){
    times_matrix.clear();
    times_matrix = _times_matrix;
    return;
}

void ExecutionMatrix::setCostsMatrix(std::vector<std::vector<int>> _cost_matrix){
    cost_matrix.clear();
    cost_matrix = _cost_matrix;
    return;
}

void ExecutionMatrix::setSubTaskTotalTime(int TaskID, int subTotalTime, int subTotalCost) {
    if (TaskID < 0 || static_cast<size_t>(TaskID) >= cost_matrix.size() || static_cast<size_t>(TaskID) >= times_matrix.size()) {
        std::cerr << "Invalid TaskID: " << TaskID << std::endl;
        return;
    }

    for (size_t i = 0; i < cost_matrix[TaskID].size(); i++) {
        cost_matrix[TaskID][i] = subTotalCost;
    }

    for (size_t i = 0; i < times_matrix[TaskID].size(); i++) {
        times_matrix[TaskID][i] = subTotalTime;
    }
}

void ExecutionMatrix::normalize(double task_ratio,double cost_ratio,double time_ratio) {
    normalized_matrix.resize(times_matrix.size());
    for(size_t i = 0; i < times_matrix.size(); i++) {
        normalized_matrix[i].resize(times_matrix[i].size());
        for(size_t j = 0; j < times_matrix[i].size(); j++) {
            normalized_matrix[i][j] = times_matrix[i][j] * cost_matrix[i][j];
        }
    }
}

void ExecutionMatrix::setRandomTimesAndCosts() {
    int randTime;
    int randCost;
    double randComplexity;
    times_matrix.clear();
    cost_matrix.clear();
    std::vector<int> times_row;
    std::vector<int> costs_row;
    for (int t = 0; t < graph_size; t++) {
        for (HardwareProcessor hw : HW_vec) {
            randComplexity = 1 + rand() % (SCALE / 4);
            randTime = randComplexity * SCALE * sqrt(SCALE) / (hw._getCost() + 1 + rand() % (SCALE / 8));
            randCost = SCALE * 8 / 1 + randTime + rand() % (SCALE / 8);
            times_row.push_back(randTime);
            costs_row.push_back(randCost);
        }
        times_matrix.push_back(times_row);
        cost_matrix.push_back(costs_row);
        times_row.clear();
        costs_row.clear();
    }
    return;

}

void ExecutionMatrix::show(std::ostream& out) const{
    printTimes(out);
    printCosts(out);
    return;
}

void ExecutionMatrix::printTimes(std::ostream& out) const{
    out << "@times\n";
    for (std::vector<int> row : times_matrix) {
        for (int i : row) {
            out << i << " ";
        }
        out << std::endl;
    }
    return;
}

void ExecutionMatrix::printCosts(std::ostream& out) const{
    out << "@cost\n";
    for (std::vector<int> row : cost_matrix) {
        for (int i : row) {
            out << i << " ";
        }
        out << std::endl;
    }
    return;
}


int ExecutionMatrix::getTime(int TaskID, const HardwareProcessor* h) const {
    if (h == nullptr || TaskID < 0 || TaskID >= static_cast<int>(times_matrix.size())) return 0;
    int hid = h->getID();
    if (hid < 0 || hid >= static_cast<int>(times_matrix[TaskID].size())) return 0;
    return times_matrix[TaskID][hid];
}

int ExecutionMatrix::getCost(int TaskID, const HardwareProcessor* h) const {
    if (h == nullptr || TaskID < 0 || TaskID >= static_cast<int>(cost_matrix.size())) return 0;
    int hid = h->getID();
    if (hid < 0 || hid >= static_cast<int>(cost_matrix[TaskID].size())) return 0;
    return cost_matrix[TaskID][hid];
}

int ExecutionMatrix::getNormalized(int TaskID, const HardwareProcessor* h) const {
    if (h == nullptr || TaskID < 0 || TaskID >= static_cast<int>(normalized_matrix.size())) return 0;
    int hid = h->getID();
    if (hid < 0 || hid >= static_cast<int>(normalized_matrix[TaskID].size())) return 0;
    return static_cast<int>(normalized_matrix[TaskID][hid]);
}
