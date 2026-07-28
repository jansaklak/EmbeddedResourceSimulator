#include <iostream>
#include <utility>
#include <vector>


class Task {
private:
    int id;
    int type;
    bool& condition;
// 0-normal 1-warunkowe
public:
    Task(int _id){
        id = id;
        type = 0;
    }
    Task(int _id, bool& _condition){
        condition = _condition;
    }
    int getID(){
        return id;
    }
    int getType(){
        return type;
    }
    bool getCondition(){
        return condition;
    }
    std::ostream& operator<<(std::ostream& os, const Task& t) {
        os << t.getID();
        return os;
    }
}