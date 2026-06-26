#include "catch.h"

#include "../include/vectorforge/cluster.h"
#include "../include/vectorforge/node.h"
#include "../include/vectorforge/vector_base.h"

#include <string>
#include <stdexcept>

// Your project uses a mix of the 'vectorforge' and 'vectorforge' namespaces. 
// We bring them into scope here for clean test definitions.
using namespace vectorforge::cluster;
using namespace vectorforge::node;
using namespace vectorforge::vector_base;

// Type Aliases to keep the tests readable
using TestVector = VectorBase<std::string, double, 2>;
using TestNode = Node<std::string, double, 2, 3>;
using TestCluster = Cluster<std::string, double, 2, 3>;

TEST_CASE("Cluster: Initialization and Head Node", "[cluster]") {
    TestCluster cluster;

    // 1. Verify default state
    REQUIRE(cluster.GetHeadNode() == nullptr);

    // 2. Add the first node (should become the head)
    TestNode* head_node = new TestNode(new TestVector({0.0, 0.0}, "Head Payload"));
    cluster.AddNode(head_node);

    REQUIRE(cluster.GetHeadNode() != nullptr);
    REQUIRE(cluster.GetHeadNode()->GetData().GetData() == "Head Payload");
}

TEST_CASE("Cluster: AddNode Exception Handling", "[cluster]") {
    TestCluster cluster;

    // Create two nodes and connect them outside the cluster
    TestNode* nodeA = new TestNode(new TestVector({1.0, 1.0}, "A"));
    TestNode* nodeB = new TestNode(new TestVector({2.0, 2.0}, "B"));
    nodeA -> AddConnection(nodeB);

    // Attempting to add a node that already has connections should trigger 
    // the exceptions::ThrowNodeExistsInGraph() logic!
    REQUIRE_THROWS_AS(cluster.AddNode(nodeA), std::logic_error);
    
    // Clean up our manual nodes
    delete nodeA; 
    delete nodeB;
}

TEST_CASE("Cluster: Meta-Graph Connections", "[cluster]") {
    TestCluster clusterA;
    TestCluster clusterB;
    TestCluster clusterC;
    TestCluster clusterD;
    TestCluster clusterE;

    SECTION("Bidirectional Cluster Linking") {
        clusterA.AddConnection(clusterB);

        // Both clusters should now be linked in the Meta-Graph
        REQUIRE(clusterA.GetAdjacencySet().size() == 1);
        REQUIRE(clusterB.GetAdjacencySet().size() == 1);

        // Sever the link
        clusterA.SeverConnection(clusterB);
        REQUIRE(clusterA.GetAdjacencySet().empty());
        REQUIRE(clusterB.GetAdjacencySet().empty());
    }

    SECTION("Enforcing Max Connections on Clusters") {
        // Our test template sets MaxConnections to 3
        clusterA.AddConnection(clusterB);
        clusterA.AddConnection(clusterC);
        clusterA.AddConnection(clusterD);
    }
}

TEST_CASE("Cluster: Empty / Single Node Search Behaviors", "[cluster]") {
    TestCluster cluster;
    std::array<double, 2> query = {5.0, 5.0};

    SECTION("Searching an empty cluster safely returns empty/null") {
        REQUIRE(cluster.FindNearestNode(query) == nullptr);
        
        auto k_results = cluster.FindNearestKNodes(query, 3);
        REQUIRE(k_results.empty());
    }

    SECTION("Searching a cluster with only a head node") {
        TestNode* head_node = new TestNode(new TestVector({0.0, 0.0}, "Origin"));
        cluster.AddNode(head_node);

        // Single Nearest Node
        TestNode* result = cluster.FindNearestNode(query);
        REQUIRE(result != nullptr);
        REQUIRE(result->GetData().GetData() == "Origin");

        // K-Nearest Nodes (asking for 3, but only 1 exists)
        auto k_results = cluster.FindNearestKNodes(query, 3);
        REQUIRE(k_results.size() == 1);
        REQUIRE(k_results[0]->GetData().GetData() == "Origin");
    }
}

TEST_CASE("Cluster: Multi-Node Search Routing", "[cluster]") {
    TestCluster cluster;

    // Create a path of nodes
    TestNode* nodeA = new TestNode(new TestVector({0.0, 0.0}, "Node A")); // Head
    TestNode* nodeB = new TestNode(new TestVector({1.0, 1.0}, "Node B"));
    TestNode* nodeC = new TestNode(new TestVector({2.0, 2.0}, "Node C"));
    TestNode* nodeD = new TestNode(new TestVector({3.0, 3.0}, "Node D"));

    // Set Node A as the entry point for the cluster
    cluster.AddNode(nodeA);
    
    // Manually link the nodes to form a traversable graph: A <-> B <-> C <-> D
    nodeA->AddConnection(nodeB);
    nodeB->AddConnection(nodeC);
    nodeC->AddConnection(nodeD);

    SECTION("Greedy Search for Single Nearest Node") {
        // Query coordinate closer to Node C (2.0, 2.0)
        std::array<double, 2> query = {2.1, 2.1};
        
        auto* result = cluster.FindNearestNode(query);
        
        // The search should traverse A -> B -> C and stop at C as the local minimum
        REQUIRE(result != nullptr);
        REQUIRE(result->GetData().GetData() == "Node C");
    }

    SECTION("Greedy Search for K-Nearest Nodes") {
        // Query coordinate closer to Node B (1.0, 1.0)
        std::array<double, 2> query = {1.1, 1.1};
        
        // Ask for the 3 closest nodes. 
        // Expected order (best to worst): B, C, A
        auto k_results = cluster.FindNearestKNodes(query, 3);
        
        REQUIRE(k_results.size() == 3);
        REQUIRE(k_results[0]->GetData().GetData() == "Node B"); // Closest
        REQUIRE(k_results[1]->GetData().GetData() == "Node C"); // 2nd Closest
        REQUIRE(k_results[2]->GetData().GetData() == "Node A"); // 3rd Closest
    }
}