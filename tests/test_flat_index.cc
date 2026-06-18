#include "catch.h"

#include "../include/hnsw/flat_index.h"
#include "../include/hnsw/vector_base.h"

#include <string>
#include <array>
#include <vector>

using namespace hnsw;
using namespace hnsw::flat_index;
using namespace hnsw::vector_base;

TEST_CASE("FlatIndex: Initialization and Basic Addition", "[flat_index]") {
    // We will use std::string as our DataType (Payload) and double for DistanceType
    FlatIndex<std::string, double, 2> index;

    REQUIRE(index.GetDimensions() == 2);
    REQUIRE(index.GetData().empty());

    // Create a vector with coordinates and string data
    std::array<double, 2> coords = {1.0, 2.0};
    VectorBase<std::string, double, 2> vec(coords, "Vector A");

    index.AddData(vec);

    REQUIRE(index.GetData().size() == 1);
    
    // Verify the data was inserted correctly by checking the payload
    REQUIRE(index.GetData()[0]->GetData() == "Vector A");
}

TEST_CASE("FlatIndex: K-Nearest Neighbor Search", "[flat_index]") {
    FlatIndex<std::string, double, 2> index;

    // Insert multiple points into our brute-force database
    index.AddData(VectorBase<std::string, double, 2>({0.0, 0.0}, "Origin"));
    index.AddData(VectorBase<std::string, double, 2>({0.0, 1.0}, "Up"));
    index.AddData(VectorBase<std::string, double, 2>({0.0, 5.0}, "Far Up"));
    index.AddData(VectorBase<std::string, double, 2>({1.0, 0.0}, "Right"));
    index.AddData(VectorBase<std::string, double, 2>({10.0, 10.0}, "Far Away"));

    SECTION("Search for exactly K elements") {
        // We will query slightly above the origin. 
        // Expectation: Origin (closest), Up (2nd), Right (3rd)
        std::array<double, 2> query = {0.0, 0.1}; 
        
        // Search should return our DataType payloads (std::string)
        auto results = index.Search(query, 3);
        
        REQUIRE(results.size() == 3);
        REQUIRE(results[0] == "Origin"); 
        REQUIRE(results[1] == "Up");     
        REQUIRE(results[2] == "Right");  
    }

    SECTION("Search for more elements than exist in the index") {
        std::array<double, 2> query = {0.0, 0.0};
        
        // Ask for 10 items, but the index only has 5
        auto results = index.Search(query, 10);
        
        REQUIRE(results.size() == 5); 
    }

    SECTION("Search an empty index") {
        FlatIndex<std::string, double, 2> empty_index;
        std::array<double, 2> query = {1.0, 1.0};
        
        auto results = empty_index.Search(query, 3);
        REQUIRE(results.empty());
    }
}

TEST_CASE("FlatIndex: Rule of Five (Memory Management)", "[flat_index]") {
    FlatIndex<std::string, double, 2> original_index;
    original_index.AddData(VectorBase<std::string, double, 2>({1.0, 1.0}, "Test Data"));

    SECTION("Copy Constructor creates a Deep Copy") {
        FlatIndex<std::string, double, 2> copied_index(original_index);
        
        REQUIRE(copied_index.GetData().size() == 1);
        REQUIRE(copied_index.GetData()[0]->GetData() == "Test Data");

        // MEMORY CHECK: Ensure the pointers point to different memory addresses!
        // If this fails, the index made a shallow copy, which causes double-free crashes.
        REQUIRE(copied_index.GetData()[0] != original_index.GetData()[0]);
    }

    SECTION("Move Constructor transfers ownership properly") {
        FlatIndex<std::string, double, 2> moved_index(std::move(original_index));
        
        REQUIRE(moved_index.GetData().size() == 1);
        REQUIRE(moved_index.GetData()[0]->GetData() == "Test Data");
        
        // The original index should now be empty after its array was stolen
        REQUIRE(original_index.GetData().empty());
    }
}