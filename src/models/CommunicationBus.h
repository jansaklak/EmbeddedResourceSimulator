#pragma once
#ifndef COM_H
#define COM_H

#include <iostream>
#include <vector>
#include "HardwareProcessor.h"

/**
 * @brief Represents a communication bus / channel interconnecting hardware processing units.
 */
class CommunicationBus {
private:
    int bandwidth;
    int connect_cost;
    int id;
    std::vector<HardwareProcessor> HW_line;

public:
    CommunicationBus(int _bandwidth, int _cost, int _id);
    CommunicationBus(std::vector<HardwareProcessor> h_list, int _bandwidth, int _cost, int _id);

    void add_Hardware(const HardwareProcessor* h);
    void remove_Hardware(const HardwareProcessor* h);
    void printCOM(std::ostream& out, std::vector<HardwareProcessor> hws) const;

    int getSize() const;
    int getID() const;
    int getBandwidth() const;
    int getCost() const;

    bool isConnected(const HardwareProcessor* other) const;
    bool operator<(const CommunicationBus& other) const;
};

/// Backward compatibility alias
using COM = CommunicationBus;

#endif // COM_H