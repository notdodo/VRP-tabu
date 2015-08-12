#ifndef Graph_H
#define Graph_H

#include <iostream>
#include "Vertex.h"

class Graph {
public:
    Graph() = default;

    void InsertVertex(Customer);
    void InsertEdge(Customer, Customer, int);
    void RemoveEdge(Customer, Customer);
    /* multimap allow same keys */
    std::multimap<int, Customer> sortV0();
    std::string ToPrint();
    std::pair<Customer, int> GetCosts(const Customer&, const Customer&);
protected:
    void InsertVertex(Customer, Vertex);
    void InsertEdge(Customer, Edge, int);

private:
    std::map<Customer, Vertex> vertexes;            /**< Map of vertexes: for each customer the vertex in input or output */

    /** @brief overriding "<<" operator to print the graph
    friend std::ostream& operator<<(std::ostream& out, const Graph& g) {
        std::vector<Customer> end_points;
        // for each pair Customer-Vertex
        for(auto&  pair : g.vertexes) {
            end_points = pair.second.copy_edges();
            out << pair.first << " : "; // Customer name
            for(auto& edge : end_points) {
                int n = pair.second.GetWeight(edge); // edge weight
                out << "\t -(" << n << ")-> " << edge;
                out << "\r\n";
            }
            out << std::endl;
        }
        return out;
    }*/
};

#endif /* Graph_H */