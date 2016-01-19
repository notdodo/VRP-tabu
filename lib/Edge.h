#ifndef Edge_H
#define Edge_H

#include "Customer.h"

class Edge {
public:
    // Only class Vertex can create an object Edge
    class ConstructionToken {
    private:
        ConstructionToken();
        friend class Vertex;
    };

    Edge(const Edge &);
    Edge(const ConstructionToken &, int); 	//!< constructor
    int weight;         					/**< Weight of the Edge */
private:
};

#endif /* Edge_H */
