#include "TimeAndCost.h"

TimeAndCost::TimeAndCost(int _Cost, int _time) {
    cost = _Cost;
    time = _time;
}

int TimeAndCost::getCost() const{
    return cost;
}

int TimeAndCost::getTime() const{
    return time;
}
