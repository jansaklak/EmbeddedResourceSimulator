#include "HardwareProcessor.h"
#include "HardwareInstance.h"
#include "ExecutionMatrix.h"
#include <unordered_set>
#include <iostream>

HardwareInstance::HardwareInstance(int _id, const HardwareProcessor* _hardwarePtr)
    : id(_id), hardwarePtr(_hardwarePtr){
        is_Virtual = 0;
    }

HardwareInstance::HardwareInstance(int _id, const HardwareProcessor* _hardwarePtr, bool _isVirtual)
    : id(_id), hardwarePtr(_hardwarePtr){
        is_Virtual = _isVirtual;
    }

int HardwareInstance::getID() const { 
    return id;
}

const HardwareProcessor* HardwareInstance::getHardwarePtr() const {
    return hardwarePtr;
}

bool HardwareInstance::isVirtual() const {
    if(this->is_Virtual==1) return 1;
    return 0;
}

bool HardwareInstance::operator<(const HardwareInstance& other) const {
    if (hardwarePtr != other.hardwarePtr) {
        return hardwarePtr < other.hardwarePtr;
    }
    return id < other.id;
}

std::ostream& operator<<(std::ostream& os, const HardwareInstance& instance) {
    if(instance.isVirtual()){
        os << "V_" << *instance.getHardwarePtr() << "_" << instance.getID();
    }
    else{
        os << *instance.getHardwarePtr() << "_" << instance.getID();
    }
    return os;
}

const std::set<int>& HardwareInstance::getTaskSet() const {
    return taskSet;
}

void HardwareInstance::setTasksSet(std::set<int> tasks){
    taskSet = tasks;
}

void HardwareInstance::addTask(int task) { 
    taskSet.insert(task); 
}

void HardwareInstance::removeTask(int task) { 
    taskSet.erase(task); 
}
