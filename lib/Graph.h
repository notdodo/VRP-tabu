#ifndef Graph_H
#define Graph_H

#include "Vertex.h"

class Graph {
public:
    Graph() = default;

    void InsertVertex(Customer &);
    void InsertEdge(Customer &, Customer &, int);
    void RemoveEdge(Customer &, Customer &);
    /* multimap allow same keys */
    std::multimap<int, Customer> sortV0();
    std::multimap<int, Customer> GetNeighborhood(const Customer) const;
    std::pair<Customer, int> GetCosts(const Customer&, const Customer&) const;
protected:
    void InsertVertex(Customer &, Vertex &);

private:
    std::map<Customer, Vertex> vertexes;            /**< Map of vertexes: for each customer the vertex in input or output */
};

#endif /* Graph_H */
