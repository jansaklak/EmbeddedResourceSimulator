
#include "HardwareProcessor.h"
#include <cstdlib>
#include <iostream>

HardwareProcessor::HardwareProcessor(double power, Hardware_Type type,int _id) {
    if (power > 10) power = 10;
    double mn = std::rand() % (2 * SCALE) + SCALE;
    if (type == Hardware_Type::HC) {
        H_type = Hardware_Type::HC;
    } else {
        H_type = Hardware_Type::PE;
        mn = mn / 10;
    }
    cost = power * mn;
    restrictions = 0;
    id = _id;
}

bool HardwareProcessor::operator<(const HardwareProcessor& other) const {
        if (H_type != other.H_type) {
            if(H_type == Hardware_Type::PE){
                return 1;
            }
            else{
                return 0;
            }
        }
        return id < other.id;
}

HardwareProcessor::HardwareProcessor(int _type,int _cost,int _id) {
    if (_type == 0) {
        H_type = Hardware_Type::HC;
    } else {
        H_type = Hardware_Type::PE;
    }
    restrictions = 0;
    cost = _cost;
    id = _id;
}

std::string HardwareProcessor::getType() const{
    if(H_type == Hardware_Type::HC){
        return "HC";
    }
    else{
        return "PE";
    }
}

std::ostream& operator<<(std::ostream& os, const HardwareProcessor& hw){
    os << hw.getType() << hw.getID();
    return os;
}

int HardwareProcessor::getCost() const {
    if(H_type == Hardware_Type::HC) return 0;
    return cost;
}

int HardwareProcessor::_getCost() const {
    return cost;
}

int HardwareProcessor::getID() const {
    return id;
}

void HardwareProcessor::printHW(std::ostream &out) {
    out << getCost() << " " << restrictions << " ";
    if (H_type == Hardware_Type::PE) {
        out << 1;
    }
    if (H_type == Hardware_Type::HC) {
        out << 0;
    }
}
