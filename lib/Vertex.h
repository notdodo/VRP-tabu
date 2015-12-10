#ifndef Vertex_H
#define Vertex_H

#include <map>
#include <vector>
#include "Edge.h"

class Vertex {
public:
    // Only Graph can create an object of type Vertex
    class ConstructionToken {
    private:
        ConstructionToken() = default;
        friend class Graph;
    };

    Vertex(ConstructionToken &);          //!< constructor
    void InsertEdge(Customer &, int);
    void RemoveEdge(Customer &);
    int GetWeight(Customer &);
    std::map<Customer, Edge> GetEdges();
private:
    std::map<Customer, Edge> edges;             /**< List of all the edges from customer */
};

#endif /* Vertex_H */
