#ifndef VRP_H
#define VRP_H

#include "Graph.h"
#include "Route.h"

typedef std::multimap<int, Customer> Map;
typedef std::list<std::pair<Customer, int>> RouteList;

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
    // create the initial routes
    int InitSolutions();
    // return the routes
    std::list<Route>* GetRoutes();
    // order route by fitness
    void OrderFitness();
    bool SwapFromTo(Route &, Route &);
    bool Move1FromTo(Route &, Route &);
    void Opt10();
    void Opt11();
    void Opt21();
    void Opt12();
    void Opt22();
    ~VRP();
};

#endif /* VRP_H */