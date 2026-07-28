#pragma once
#ifndef TASK_GRAPH_H
#define TASK_GRAPH_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "Edge.h"

/**
 * @brief Represents a Directed Acyclic Graph (DAG) of tasks and precedence dependencies.
 */
class TaskGraph {
private:
    int maxVert;
    std::vector<std::vector<Edge>> adjList;
    int destination;
    void DFSUtil(int v, std::vector<bool>& visited, std::vector<int>& path, std::vector<std::vector<int>>& allPaths, int destination) const;

public:
    TaskGraph();
    ~TaskGraph();

    void addEdge(int u, int v);
    void addEdge(int u, int v, int weight);
    int getWeightEdge(int u, int v) const;
    bool checkEdge(int u, int v) const;

    std::vector<int> getNeighbourIndices(int idx) const;
    std::vector<int> getOutNeighbourIndices(int idx) const;
    std::vector<int> getInNeighbourIndices(int idx) const;

    std::vector<std::vector<Edge>> getAdjList() const;
    int getNumberOfEdges() const;
    int getVerticesSize() const;

    std::vector<std::vector<int>> DFS(int start, int end);
    std::vector<int> BFS();
    void printMatrix(std::ostream& out = std::cout) const;
    void printNeighbourIndices(int idx) const;
};

/// Backward compatibility alias
using Graf = TaskGraph;

#endif // TASK_GRAPH_H
