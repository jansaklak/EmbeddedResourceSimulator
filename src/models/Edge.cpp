#include "Edge.h"

Edge::Edge(int i_Vertex_Index1, int i_Vertex_Index2)
{
    vertex_Index1 = i_Vertex_Index1;
    vertex_Index2 = i_Vertex_Index2;
    waga = 0;
}

Edge::Edge(int i_Vertex_Index_1, int i_Vertex_Index_2, float i_weight)
{
    vertex_Index1 = i_Vertex_Index_1;
    vertex_Index2 = i_Vertex_Index_2;
    waga = i_weight;
}

int Edge::getV1() const{
    return vertex_Index1;
}

int Edge::getV2() const{
    return vertex_Index2;
}


int Edge::getWeight() const{
    return waga;
}