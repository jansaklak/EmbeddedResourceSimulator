#include "CommunicationBus.h"

CommunicationBus::CommunicationBus(int _bandwidth,int _cost,int _id) {
    bandwidth = _bandwidth;
    connect_cost = _cost;
    id = _id;
}
CommunicationBus::CommunicationBus(std::vector<HardwareProcessor> _HW_line, int _bandwidth,int _cost,int _id) {
    HW_line = _HW_line;
    bandwidth = _bandwidth;
    connect_cost = _cost;
    id = _id;
}
void CommunicationBus::add_Hardware(const HardwareProcessor* h) {
    if(isConnected(h)){
        return;
    }
    HW_line.push_back(*h);
}

void CommunicationBus::remove_Hardware(const HardwareProcessor* h) {
    for (auto it = HW_line.begin(); it != HW_line.end(); ++it) {
        if ((*it).getID() == h->getID()) {
            HW_line.erase(it);
            break;
        }
    }
}

int CommunicationBus::getSize() const{
    return HW_line.size();
}
int CommunicationBus::getBandwidth() const{
    return bandwidth;
}
int CommunicationBus::getCost() const{
    return connect_cost;
}
int CommunicationBus::getID() const{
    return id;
}

bool CommunicationBus::operator<(const CommunicationBus& other) const {
        return id < other.id;
}

void CommunicationBus::printCOM(std::ostream& out,std::vector<HardwareProcessor> hws) const{
    out << "CHAN" << id << " " << bandwidth << " " << connect_cost << " ";
            for (HardwareProcessor h : hws){
                out << isConnected(&h) << " ";
            }
    out << "\n";
}

bool CommunicationBus::isConnected(const HardwareProcessor* other) const{
    for(HardwareProcessor h : HW_line){  
        if(h.getID() == other->getID()) return true;
    }
    return false;
}