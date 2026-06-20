#ifndef HNSW_INCLUDE_LAYER_H_
#define HNSW_INCLUDE_LAYER_H_

#include "node.h"
#include "vector_base.h"

#include <unordered_set>
#include <array>
#include <vector>
#include <stdexcept>
#include <queue>

namespace hnsw {
namespace layer {

namespace exceptions {
    void ThrowNodeWithCoordExist() {
        throw std::logic_error(
            "Node with coordinate already exists");
    }
}

template <typename DataType, typename DistanceType, std::size_t Dimensions,
    std::size_t MaxConnections>
class Layer {
    using NodeType = node::Node<DataType, DistanceType, Dimensions, MaxConnections>;

    public:
        /**
         * Default Constructor:
         * Initialized head node to null pointer
         */
        Layer() : head_(nullptr) {};

        /**
         * Destructor:
         * Deletes all nodes
         */
        ~Layer() {
            
        }

        Layer(const Layer& other) = delete;
        Layer& operator=(const Layer& other) = delete;
        Layer(Layer&& other) noexcept = delete;
        Layer& operator=(Layer&& other) noexcept;

        /**
         * Find Nearest One Node:
         * Greedily finds the nearest node to the queried coordinate
         */
        node::Node<DataType, DistanceType, Dimensions, MaxConnections>* FindNearestNode(
            const std::array<DistanceType, Dimensions>& query_coordinates) {
                if (head_ == nullptr) {
                    return nullptr;
                }

                NodeType* current_node = head_;
                std::unordered_set<NodeType*> node_set;
                
                while (true) {
                    double current_distance = (current_node -> GetData()).EuclideanDistanceTo(query_coordinates);
                    NodeType* nearest_in_adjacent = nullptr;

                    for (NodeType* n : current_node -> GetAdjacencySet()) {
                        double distance_to_query = (n -> GetData()).EuclideanDistanceTo(query_coordinates);

                        if (node_set.count(n) == 0) {
                            if (distance_to_query < current_distance) {
                                current_distance = distance_to_query;
                                nearest_in_adjacent = n;
                            }
                        }
                    }

                    if (nearest_in_adjacent == nullptr) {
                        return current_node;
                    } else {
                        node_set.insert(current_node);
                        current_node = nearest_in_adjacent;
                    }
                }

                return nullptr;
        }

        /**
         * Find Nearest K Nodes:
         * Greedily finds the nearest K nodes to the given coordinates and 
         * returns a vector of such pointers.
         */
        std::vector<node::Node<DataType, DistanceType, Dimensions, MaxConnections>*> FindNearestKNodes(
            const std::array<DistanceType, Dimensions>& query_coordinates, const size_t k) {
                std::vector<NodeType*> results;

                if (k == 0 || head_ == nullptr) {
                    return results;
                }

                results.reserve(k);

                std::priority_queue<std::pair<double, NodeType*>> nearest_nodes_queue;
                nearest_nodes_queue.push({(head_ -> GetData()).EuclideanDistanceTo(query_coordinates), head_});

                NodeType* current_node = head_;
                std::unordered_set<NodeType*> node_set;
                node_set.insert(head_);
                
                while (true) {
                    double current_distance = (current_node -> GetData()).EuclideanDistanceTo(query_coordinates);
                    NodeType* nearest_in_adjacent = nullptr;

                    for (NodeType* n : current_node -> GetAdjacencySet()) {
                        if (node_set.count(n) == 0) {
                            double distance_to_query = (n -> GetData()).EuclideanDistanceTo(query_coordinates);
                            nearest_nodes_queue.push({distance_to_query, n});

                            if (nearest_nodes_queue.size() > k) {
                                nearest_nodes_queue.pop();
                            }

                            if (distance_to_query < current_distance) {
                                current_distance = distance_to_query;
                                nearest_in_adjacent = n;
                            }

                            node_set.insert(n);
                        }
                    }

                    if (nearest_in_adjacent == nullptr) {
                        break;
                    } else {
                        current_node = nearest_in_adjacent;
                    }
                }

                while (!nearest_nodes_queue.empty()) {
                    results.push_back(nearest_nodes_queue.top().second);
                    nearest_nodes_queue.pop();
                }

                std::reverse(results.begin(), results.end());

                return results;
        }

    private:
        /**
         * Start Node:
         * Serves as the default entry node for the layer, makes sure that all 
         * nodes trace back to this node
         */
        node::Node<DataType, DistanceType, Dimensions, MaxConnections>* head_;


};

}
}

#endif