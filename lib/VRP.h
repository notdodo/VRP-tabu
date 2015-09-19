#ifndef VRP_H
#define VRP_H

#include <chrono>
#include <algorithm>
#include "Graph.h"
#include "Route.h"

typedef std::multimap<int, Customer> Map;
typedef std::list<std::pair<Customer, int>> RouteList;

class VRP {
private:
    Graph graph;                /**< Graph of customers */
    std::list<Route> routes;    /**< List of all routes */
    int numVertices;            /**< Number of customers */
    int vehicles;               /**< Number of vehicles */
    int capacity;               /**< Capacity of each vehicle */
    int workTime;               /**< Work time for each driver */
    int totalCost;              /**< Total cost of the routes */
    Map::const_iterator InsertStep(Customer, Map::iterator, Map::const_iterator, Route &, Map &);
    bool SwapFromTo(Route &, Route &);
    bool Move1FromTo(Route &, Route &);
    bool AddRemoveFromTo(Route &, Route &, int, int);
    void CleanVoid();
public:
    VRP() {} //!< constructor
    VRP(const Graph g, const int n, const int v, const int c, const int t); //!< constructor
    int InitSolutions();
    std::list<Route>* GetRoutes();
    void OrderByCosts();
    bool Opt10();
    bool Opt01();
    bool Opt11();
    bool Opt21();
    bool Opt12();
    bool Opt22();
    int GetTotalCost() const;
    ~VRP(); //!< destructor
};

#endif /* VRP_H */