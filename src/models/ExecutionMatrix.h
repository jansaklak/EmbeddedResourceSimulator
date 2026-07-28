#pragma once
#ifndef TIMES_H
#define TIMES_H

#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include "TaskGraph.h"
#include "HardwareProcessor.h"

/**
 * @brief Manages the matrices of execution times t(T, H) and costs c(T, H) for tasks on hardware processors.
 */
class ExecutionMatrix {
private:
    int graph_size;
    std::vector<HardwareProcessor> HW_vec;
    std::vector<std::vector<int>> times_matrix;
    std::vector<std::vector<int>> cost_matrix;
    std::vector<std::vector<double>> normalized_matrix;

    void CountCosts();
    void clear();

public:
    ExecutionMatrix();
    ExecutionMatrix(int graph_size);

    int getTime(int TaskID, const HardwareProcessor* h) const;
    int getCost(int TaskID, const HardwareProcessor* h) const;
    int getNormalized(int TaskID, const HardwareProcessor* h) const;

    void setRandomTimesAndCosts();
    void setSubTaskTotalTime(int TaskID, int subTotalTime, int subTotalCost);
    void setTimesMatrix(std::vector<std::vector<int>> _times_matrix);
    void setCostsMatrix(std::vector<std::vector<int>> _costs_matrix);

    void normalize(double task_ratio = 1.0, double cost_ratio = 1.0, double time_ratio = 1.0);
    void updateNormalized(int TaskID, const HardwareProcessor* h);
    void LoadHW(std::vector<HardwareProcessor> _HW_vec);

    void show(std::ostream& out = std::cout) const;
    void printTimes(std::ostream& out = std::cout) const;
    void printCosts(std::ostream& out = std::cout) const;
};

/// Backward compatibility alias
using Times = ExecutionMatrix;

#endif // TIMES_H
