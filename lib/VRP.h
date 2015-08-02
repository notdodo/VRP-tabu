#ifndef VRP_H
#define VRP_H

#include "Graph.h"
#include "Route.h"

typedef std::multimap<int, Customer> Map;

class VRP {
private:
    Graph graph;
    std::list<Route> routes;
    int numVertices;
    int vehicles;
    int capacity;
    int workTime;
    Map::const_iterator InsertStep(Customer, Map::iterator, Map::const_iterator, Route &, Map &);
public:
    VRP() {}
    VRP(const Graph g, const int n, const int v, const int c, const int t);
    void InitSolutions();
    ~VRP();
};

#endif /* VRP_H */