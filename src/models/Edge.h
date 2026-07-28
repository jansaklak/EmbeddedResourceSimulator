#pragma once

class Edge
{
    int vertex_Index1;
    int vertex_Index2;
    float waga;

public:
    Edge(int i_Vertex_Index1, int i_Vertex_Index2);
    Edge(int i_Vertex_Index_1, int i_Vertex_Index_2, float i_weight);
    int getV1() const;
    int getV2() const;
    int getWeight() const;
};


