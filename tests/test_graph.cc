#include "catch.h"

#include "../include/vectorforge/graph.h"
#include "../include/vectorforge/cluster.h"
#include "../include/vectorforge/node.h"
#include "../include/vectorforge/vector_base.h"

#include <string>
#include <array>

using namespace vectorforge::cluster;
using namespace vectorforge::node;
using namespace vectorforge::vector_base;
using namespace vectorforge::graph;

// Type Aliases for easy testing
using TestVector = VectorBase<std::string, double, 2>;
using TestNode = Node<std::string, double, 2, 3>;
using TestCluster = Cluster<std::string, double, 2, 3>;

// Assuming your Graph template signature matches the others
using TestGraph = Graph<std::string, double, 2, 3>; 

TEST_CASE("Graph: Initialization and First Cluster", "[meta_graph]") {
    TestGraph meta_graph;
    
    std::array<double, 2> initial_coords = {0.0, 0.0};
    meta_graph.AddNode("First Payload", initial_coords);

    TestCluster* nearest = meta_graph.GetNearestCluster(initial_coords);
    REQUIRE(nearest != nullptr);
    
    REQUIRE(nearest->GetHeadNode()->GetData() == "First Payload");
}

TEST_CASE("Graph: Cluster Routing (GetNearestCluster)", "[meta_graph]") {
    TestGraph meta_graph;

    // ========================================================================
    // TDD TARGET: This test will fail until you finish the `else { ... }` block
    // in your AddNode function to actually spawn new clusters for these vectors!
    // ========================================================================

    meta_graph.AddNode("Cluster A", {0.0, 0.0});
    meta_graph.AddNode("Cluster B", {10.0, 10.0});
    meta_graph.AddNode("Cluster C", {20.0, 20.0});

    // We will manually fetch the clusters to link them together for the test
    TestCluster* clusterA = meta_graph.GetNearestCluster({0.0, 0.0});
    TestCluster* clusterB = meta_graph.GetNearestCluster({10.0, 10.0});
    TestCluster* clusterC = meta_graph.GetNearestCluster({20.0, 20.0});
    
    // Sanity check: Ensure AddNode actually created distinct clusters before linking!
    REQUIRE(clusterA != nullptr);
    REQUIRE(clusterB != nullptr);
    REQUIRE(clusterC != nullptr);
    REQUIRE(clusterA != clusterB); 

    // Manually link the clusters in the Graph to form: A <-> B <-> C
    // (Using pointers as per your architecture rules)
    clusterA->AddConnection(clusterB);
    clusterB->AddConnection(clusterC);

    SECTION("Greedy Routing successfully jumps across the Meta-Graph") {
        // Query a coordinate closest to Cluster C (19, 19)
        std::array<double, 2> query = {19.0, 19.0};

        // The Graph should start at the head (Cluster A), measure the distance 
        // to B's head node, jump to B, measure C's head node, and finally jump to C!
        TestCluster* result = meta_graph.GetNearestCluster(query);

        REQUIRE(result != nullptr);
        REQUIRE(result == clusterC);
        REQUIRE(result->GetHeadNode()->GetData() == "Cluster C");
    }
}