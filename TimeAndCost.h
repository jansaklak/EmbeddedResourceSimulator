#pragma once
#ifndef TIMEANDCOST_H
#define TIMEANDCOST_H

class TimeAndCost {
private:
    int cost;
    int time;

public:
    TimeAndCost(int _Cost, int _time){
        cost = _Cost;
        time = _time;
    }
    int getCost() const{
        return cost;
    }
    int getTime() const{
        return time;
    }
};

#endif // TIMEANDCOST_H

