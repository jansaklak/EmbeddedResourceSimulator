#pragma once
#ifndef INSTANCE_H
#define INSTANCE_H

#include "Hardware.h"
#include <set>
#include <iostream>

/**
 * @brief Represents an instantiated hardware unit (physical or virtual instance) hosting scheduled tasks.
 */
class HardwareInstance {
private:
    int id;
    const HardwareProcessor* hardwarePtr;
    std::set<int> taskSet;
    bool is_Virtual;

public:
    HardwareInstance(int _id, const HardwareProcessor* _hardwarePtr);
    HardwareInstance(int _id, const HardwareProcessor* _hardwarePtr, bool _isVirtual);

    int getID() const;
    const HardwareProcessor* getHardwarePtr() const;
    bool isVirtual() const;

    bool operator<(const HardwareInstance& other) const;
    friend std::ostream& operator<<(std::ostream& os, const HardwareInstance& instance);

    void setTasksSet(std::set<int> tasks);
    const std::set<int>& getTaskSet() const;
    void addTask(int task);
    void removeTask(int task);
};

/// Backward compatibility alias
using Instance = HardwareInstance;

#endif // INSTANCE_H
