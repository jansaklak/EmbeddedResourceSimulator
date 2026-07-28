#pragma once
#ifndef HARDWARE_H
#define HARDWARE_H

#include <iostream>
#include <set>
#include <string>

extern const int SCALE;

/**
 * @brief Hardware unit processing type classification.
 * HC: Dedicated Hardware Core (Hardware Accelerator).
 * PE: Processing Element (General Purpose Processor).
 */
enum class Hardware_Type {
    HC,
    PE
};

/**
 * @brief Represents a hardware processing unit (core or PE) in the embedded system.
 */
class HardwareProcessor {
private:
    int cost;
    Hardware_Type H_type;
    int restrictions;
    int id;

public:
    HardwareProcessor(double power, Hardware_Type type, int set_id);
    HardwareProcessor(int _type, int _cost, int _id);

    int getID() const;
    int getCost() const;
    int _getCost() const;
    std::string getType() const;

    void printHW(std::ostream& out = std::cout);
    bool operator<(const HardwareProcessor& other) const;
    friend std::ostream& operator<<(std::ostream& os, const HardwareProcessor& hw);
};

/// Backward compatibility alias
using Hardware = HardwareProcessor;

#endif // HARDWARE_H
