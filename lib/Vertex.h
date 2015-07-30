#ifndef Vertex_H
#define Vertex_H

#include <iterator>
#include <map>
#include <vector>
#include "Edge.h"
#include "../actor/Customer.h"

class Vertex {
public:
    // Only Graph can create an object of type Vertex
    class ConstructionToken {
    private:
        ConstructionToken() = default;
        friend class Graph;
    };

    Vertex(const ConstructionToken &);

    const std::vector<Customer> copy_edges() const;
    void InsertEdge(const Customer &, const int &);
    void RemoveEdge(const Customer &);
    const int GetWeight(const Customer &) const;
    const std::map<Customer, Edge> GetEdges() const;
private:
    std::map<Customer, Edge> edges;

    friend std::ostream& operator<<(std::ostream& out, const Vertex& v) {
        for(auto& e : v.edges)
            std::cout << e.second.weight << " ";
        return out;
    }
};

#endif /* Vertex_H */