#include "Vertex.h"

using edge_pair = std::pair<Customer, Edge>;

/** @brief Constructor of Vertex */
Vertex::Vertex(const ConstructionToken &) {}

/** @brief Insert an Edge.
 *
 * Insert a weighted edge which end to a customer.
 * @param end_point The destination customer
 * @param weight The weight of the edge
 */
void Vertex::InsertEdge(const Customer &end_point, const int &weight) {
    Edge new_edge { Edge::ConstructionToken{}, weight };
    edge_pair temp(end_point, new_edge);
    edges.insert(temp);
}

/** @brief Remove an edge.
 *
 * @param edge The customer which the edge end
 */
void Vertex::RemoveEdge(const Customer &edge) {
    edges.erase(edge);
}

/*const std::vector<Customer> Vertex::copy_edges() const {
    std::vector<Customer> keys;
    for(auto& pair : edges) {
        keys.push_back(pair.first);
    }
    return keys;
}*/

/** @brief Get the weight of and edge.
 *
 * @param c Customer at the end of the edge
 * @return The weight of the customer
 */
const int Vertex::GetWeight(const Customer &c) const {
    return edges.find(c)->second.weight;
}

/** @brief Get the map of the edges */
const std::map<Customer, Edge> Vertex::GetEdges() const {
    return this->edges;
}