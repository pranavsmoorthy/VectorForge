#include "catch.h"

#include <string>
#include <stdexcept>

#include <iostream>

#include "../include/vectorforge/node.h"
#include "../include/vectorforge/vector_base.h"

// Create a type alias to keep the tests clean and readable
// Template arguments: <Payload: string, Math: double, 2 Dimensions, Max 3 Connections>
using TestNode = vectorforge::node::Node<std::string, double, 2, 3>;
using TestVector = vectorforge::vector_base::VectorBase<std::string, double, 2>;

TEST_CASE("Node: Initialization and Memory Ownership", "[node]") {
    // 1. Create a heap-allocated vector
    TestVector* vec = new TestVector({1.0, 2.0}, "Payload A");
    
    // 2. Pass ownership to the Node
    TestNode nodeA(vec);

    REQUIRE(nodeA.GetData() == "Payload A");
    REQUIRE(nodeA.GetAdjacencySet().empty());
}

TEST_CASE("Node: Adding and Severing Connections", "[node]") {
    TestNode nodeA(new TestVector({0.0, 0.0}, "Node A"));
    TestNode nodeB(new TestVector({1.0, 1.0}, "Node B"));
    TestNode nodeC(new TestVector({2.0, 2.0}, "Node C"));
    TestNode nodeD(new TestVector({3.0, 3.0}, "Node D"));
    TestNode nodeE(new TestVector({4.0, 4.0}, "Node E"));

    SECTION("Bidirectional Connection Logic") {
        nodeA.AddConnection(&nodeB);

        // Both nodes should now have each other in their adjacency sets
        REQUIRE(nodeA.GetAdjacencySet().size() == 1);
        REQUIRE(nodeB.GetAdjacencySet().size() == 1);

        REQUIRE(nodeA.GetAdjacencySet().count(&nodeB) == 1);
        REQUIRE(nodeB.GetAdjacencySet().count(&nodeA) == 1);

        // Severing the connection should remove the pointers from both sets
        nodeA.SeverConnection(&nodeB);
        REQUIRE(nodeA.GetAdjacencySet().empty());
        REQUIRE(nodeB.GetAdjacencySet().empty());
    }

    SECTION("Enforcing Max Connections Exception") {
        // Our MaxConnections template parameter is set to 3.
        nodeA.AddConnection(&nodeB);
        nodeA.AddConnection(&nodeC);
        nodeA.AddConnection(&nodeD);

        // The 4th connection should throw the logic_error we defined
        REQUIRE_THROWS_AS(nodeA.AddConnection(&nodeE), std::logic_error);
    }
}

TEST_CASE("Node: Move Semantics and Neighbor Pointer Updates", "[node]") {
    TestNode nodeA(new TestVector({1.0, 1.0}, "Node A"));
    TestNode nodeB(new TestVector({2.0, 2.0}, "Node B"));
    
    // Connect A and B
    nodeA.AddConnection(&nodeB);

    SECTION("Move Assignment Operator (Tests Selected Code)") {
        TestNode nodeC(new TestVector({3.0, 3.0}, "Node C (To be overwritten)"));
        
        // This triggers the specific code block selected in the Canvas.
        // nodeC steals nodeA's memory, AND nodeB must be updated to point to nodeC!
        nodeC = std::move(nodeA);

        // 1. Verify Node C stole the payload successfully
        REQUIRE(nodeC.GetData() == "Node A");
        
        // 2. Verify Node B's pointers were updated successfully
        REQUIRE(nodeB.GetAdjacencySet().count(&nodeA) == 0); // B no longer points to A
        REQUIRE(nodeB.GetAdjacencySet().count(&nodeC) == 1); // B now points to C!
        
        // 3. Verify Node C acquired the connections
        REQUIRE(nodeC.GetAdjacencySet().count(&nodeB) == 1);
    }

    SECTION("Move Constructor") {
        // Triggers the Move Constructor instead
        TestNode nodeD(std::move(nodeA));

        // B should no longer point to A, it should point to D
        REQUIRE(nodeB.GetAdjacencySet().count(&nodeA) == 0);
        REQUIRE(nodeB.GetAdjacencySet().count(&nodeD) == 1);
        REQUIRE(nodeD.GetAdjacencySet().count(&nodeB) == 1);
    }
}

TEST_CASE("Node: Dead Node State Interactions", "[node]") {
    TestNode* nodeA = new TestNode(new TestVector({0.0, 0.0}, "Node A"));
    TestNode* nodeB = new TestNode(new TestVector({2.0, 2.0}, "Node B"));
    TestNode* nodeC = new TestNode(new TestVector({2.1, 2.1}, "Node C"));

    SECTION("Marking a node dead triggers proper state exceptions") {
        // Connect A and B. This gives B a connection so that calling MarkDead() 
        // does NOT instantly trigger `delete this;`. It stays in memory as a dead bridge!
        nodeA->AddConnection(nodeB);
        
        nodeB->MarkDead();
        
        REQUIRE(nodeB->IsDead() == true);
        REQUIRE(nodeA->IsDead() == false);

        // 1. Reading data from a dead node throws ThrowCannotGetFromDeadNode
        REQUIRE_THROWS_AS(nodeB->GetData(), std::logic_error);

        // 2. Modifying data in a dead node throws ThrowCannotEditDeadNode
        REQUIRE_THROWS_AS(nodeB->SetData("Hacked Payload"), std::logic_error);

        // 3. Connecting to a dead node throws ThrowCannotConnectToDeadNode
        REQUIRE_THROWS_AS(nodeC->AddConnection(nodeB), std::logic_error);
        REQUIRE_THROWS_AS(nodeB->AddConnection(nodeC), std::logic_error);

        // CLEANUP: Severing the connection drops both counts to 0.
        // Your SeverConnection function will now auto-delete both A and B!
        nodeA->SeverConnection(nodeB); 
        delete nodeC;
    }

    SECTION("Marking a 0-connection node dead instantly deletes it") {
        TestNode* nodeLonely = new TestNode(new TestVector({10.0, 10.0}, "Lonely Node"));
        
        // Because nodeLonely has 0 connections, your MarkDead() logic triggers `delete this;`.
        // We can't safely REQUIRE anything about the pointer after this line without risking 
        // undefined behavior, but running this test proves it completes without crashing!
        nodeLonely->MarkDead(); 
        
        // Cleanup the unused testing nodes
        delete nodeA;
        delete nodeB;
        delete nodeC;
    }
}