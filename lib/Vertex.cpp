#include "Vertex.h"

using edge_pair = std::pair<Customer, Edge>;

/* constructor */
Vertex::Vertex(const ConstructionToken &) {}

void Vertex::InsertEdge(const Customer &end_point, const int &weight) {
    Edge new_edge { Edge::ConstructionToken{}, weight };
    edge_pair temp(end_point, new_edge);
    edges.insert(temp);
}

void Vertex::RemoveEdge(const Customer &edge) {
    edges.erase(edge);
}

const std::vector<Customer> Vertex::copy_edges() const {
    std::vector<Customer> keys;
    for(auto& pair : edges) {
        keys.push_back(pair.first);
    }
    return keys;
}

const int Vertex::GetWeight(const Customer &c) const {
    return edges.find(c)->second.weight;
}

const std::map<Customer, Edge> Vertex::GetEdges() const {
    return this->edges;
}