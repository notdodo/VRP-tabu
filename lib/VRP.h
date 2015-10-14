#ifndef VRP_H
#define VRP_H

#include "Graph.h"
#include "Route.h"
#include "Utils.h"
#include <thread>
#include <mutex>

typedef std::multimap<int, Customer> Map;
typedef std::list<std::pair<Customer, int>> RouteList;
// contains: the index of the two routes and the two routes
typedef std::list<std::pair<std::pair<int, int>, std::pair<Route, Route>>> ResultList;

class VRP {
private:
    std::mutex *mtx;
    Graph graph;                /**< Graph of customers */
    std::list<Route> routes;    /**< List of all routes */
    int numVertices;            /**< Number of customers */
    int vehicles;               /**< Number of vehicles */
    int capacity;               /**< Capacity of each vehicle */
    float workTime;             /**< Work time for each driver */
    int totalCost;              /**< Total cost of the routes */
    bool verbose;               /**< Flag for verbose prints */
    Map::const_iterator InsertStep(Customer, Map::iterator, Map::const_iterator, Route &, Map &);
    bool SwapFromTo(Route &, Route &);
    bool Move1FromTo(Route &, Route &, bool);
    bool AddRemoveFromTo(Route &, Route &, int, int);
    void CleanVoid();
    Route Opt2Swap(Route, std::list<Customer>::iterator, std::list<Customer>::iterator);
    Route Opt3Swap(Route, std::list<Customer>::iterator, std::list<Customer>::iterator, std::list<Customer>::iterator, std::list<Customer>::iterator);
public:
    VRP() {} //!< constructor
    VRP(const Graph, const int, const int, const int, const float, const bool); //!< constructor
    int InitSolutions();
    std::list<Route>* GetRoutes();
    void OrderByCosts();
    bool Opt10();
    bool Opt01();
    bool Opt11();
    bool Opt21();
    bool Opt12();
    bool Opt22();
    void RouteBalancer();
    bool Opt2();
    bool Opt3();
    int GetTotalCost() const;
    ~VRP(); //!< destructor
};

#endif /* VRP_H */