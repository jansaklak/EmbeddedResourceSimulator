#include <iostream>
#include <cassert>
#include "src/models/TaskGraph.h"
#include "src/models/ExecutionMatrix.h"
#include "src/simulator/TaskSchedulerSimulator.h"

const int SCALE = 100;

void testTaskGraph() {
    std::cout << "[TEST] Running TaskGraph unit tests..." << std::endl;
    TaskGraph graph;
    graph.addEdge(0, 1, 10);
    graph.addEdge(0, 2, 20);
    graph.addEdge(1, 3, 5);
    graph.addEdge(2, 3, 15);

    assert(graph.getVerticesSize() == 4);
    assert(graph.checkEdge(0, 1) == true);
    assert(graph.checkEdge(1, 2) == false);
    assert(graph.getWeightEdge(0, 2) == 20);

    auto bfs = graph.BFS();
    assert(bfs.size() == 4);
    assert(bfs[0] == 0);

    std::cout << "  ✅ TaskGraph tests passed!" << std::endl;
}

void testSimulatorStrategies() {
    std::cout << "[TEST] Running TaskSchedulerSimulator strategy tests..." << std::endl;

    // Test S1 (Min-Time Dedicated)
    {
        std::cout << "  Testing S1..." << std::endl;
        TaskSchedulerSimulator sim;
        assert(sim.Load_From_File("data/graph20.dat") == 1);
        sim.taskDistribution(1);
        assert(sim.getCriticalTime() > 0);
    }

    // Test S2 (Min-Cost Dedicated)
    {
        std::cout << "  Testing S2..." << std::endl;
        TaskSchedulerSimulator sim;
        assert(sim.Load_From_File("data/graph20.dat") == 1);
        sim.taskDistribution(2);
        assert(sim.getCriticalTime() > 0);
    }

    // Test S8 (Constrained Penalty Optimization)
    {
        std::cout << "  Testing S8..." << std::endl;
        TaskSchedulerSimulator sim;
        assert(sim.Load_From_File("data/graph20.dat") == 1);
        sim.setHardTime(250);
        sim.setPenaltyFactor(2);
        sim.taskDistribution(8);
        assert(sim.getCriticalTime() > 0);
    }

    // Test S9 (Single Core Baseline)
    {
        std::cout << "  Testing S9..." << std::endl;
        TaskSchedulerSimulator sim;
        assert(sim.Load_From_File("data/graph20.dat") == 1);
        sim.taskDistribution(9);
        assert(sim.getCriticalTime() > 0);
    }

    std::cout << "  ✅ TaskSchedulerSimulator strategy tests passed!" << std::endl;
}

int main() {
    std::cout << "==================================================" << std::endl;
    std::cout << " Running Embedded Resource Simulator Test Suite   " << std::endl;
    std::cout << "==================================================" << std::endl;

    testTaskGraph();
    testSimulatorStrategies();

    std::cout << "==================================================" << std::endl;
    std::cout << " 🎉 ALL TESTS PASSED SUCCESSFULLY!                 " << std::endl;
    std::cout << "==================================================" << std::endl;
    return 0;
}
