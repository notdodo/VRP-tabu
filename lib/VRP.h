#ifndef VRP_H
#define VRP_H

#include "Graph.h"
#include "Route.h"
#include "Utils.h"
#include "TabuList.h"
#include "../lib/ThreadPool.h"
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
    float averageDistance;      /**< Average distance of all routes */
    TabuList tabulist;          /**< List of all tabu moves */
    unsigned numberOfCores;     /**< Number of cores */
	std::list<Route> bestRoutes;/**< The best configuration of routes founded */
    int totalCost;              /**< Total cost of routes */
    float fitness;
	bool SwapFromTo(Route &, Route &);
    bool Move1FromTo(Route &, Route &, bool);
    bool AddRemoveFromTo(Route &, Route &, int, int);
    void CleanVoid();
    Route Opt2Swap(Route r, Customer i , Customer k);
    Route Opt3Swap(Route, Customer, Customer, Customer, Customer);
    int FindRouteFromCustomer(Customer);
    bool CheckIntegrity();
    void UpdateDistanceAverage();
    void OrderByCosts();
public:
    VRP() {}					//!< constructor
    VRP(Graph, const int, const int, const int, const float, const bool, const float, const float); //!< constructor
    int InitSolutions();
    int Opt10(bool);
    int Opt01(bool);
    int Opt11(bool);
    int Opt21(bool);
    int Opt12(bool);
    int Opt22(bool);
    void RouteBalancer();
    bool Opt2();
    bool Opt3();
    int GetTotalCost();
    int GetNumberOfCustomers() const;
	void TabuSearch();
    std::list<Route>* GetRoutes();
	std::list<Route>* GetBestRoutes();
	void UpdateBest();
    void Evaluate();
	~VRP();						//!< destructor
};

#endif /* VRP_H */
