#ifndef VRP_H
#define VRP_H

#include "Graph.h"
#include "Route.h"
#include "Utils.h"
#include "TabuList.h"
#include <thread>
#include <mutex>

typedef std::multimap<int, Customer> Map;
typedef std::list<StepType> RouteList;
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
    float costTravel;           /**< Cost parameter for each travel */
    float alphaParam;           /**< Alpha parameter for route evaluation */
    TabuList tabulist;          /**<List of all tabu moves */
    bool SwapFromTo(Route &, Route &);
    bool Move1FromTo(Route &, Route &, bool);
    bool AddRemoveFromTo(Route &, Route &, int, int);
    void CleanVoid();
    Route Opt2Swap(Route r, std::list<Customer>::iterator i , std::list<Customer>::iterator k);
    Route Opt3Swap(Route, std::list<Customer>::iterator, std::list<Customer>::iterator, std::list<Customer>::iterator, std::list<Customer>::iterator);
public:
    VRP() {}					//!< constructor
    VRP(Graph, const int, const int, const int, const float, const bool, const float, const float, const int); //!< constructor
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
    int GetNumberOfCustomers() const;
	void TabuSearch();
    int FindRouteFromCustomer(Customer);
    bool CheckIntegrity();
	~VRP();						//!< destructor
};

#endif /* VRP_H */
