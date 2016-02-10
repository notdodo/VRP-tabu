#ifndef VRP_H
#define VRP_H

#include "Graph.h"
#include "Utils.h"
#include "TabuList.h"
#include "OptimalMove.h"
#include "TabuSearch.h"
#include "../lib/ThreadPool.h"

typedef std::multimap<int, Customer> Map;

class VRP {
private:
    Graph graph;                /**< Graph of customers */
    std::list<Route> routes;    /**< List of all routes */
    int numVertices;            /**< Number of customers */
    int vehicles;               /**< Number of vehicles */
    int capacity;               /**< Capacity of each vehicle */
    float workTime;             /**< Work time for each driver */
    float costTravel;           /**< Cost parameter for each travel */
    float alphaParam;           /**< Alpha parameter for route evaluation */
    float averageDistance;      /**< Average distance of all routes */
	std::list<Route> bestRoutes;/**< The best configuration of routes founded */
    int totalCost;              /**< Total cost of routes */
    bool CheckIntegrity();
    void OrderByCosts();
public:
    VRP() {};			                                                                                  //!< constructor
    VRP(const Graph &, const int, const int, const int, const float, const bool, const float, const float); //!< constructor
    int InitSolutions();
    void RunTabuSearch();
    bool RunOpts(int, bool);
    int GetTotalCost();
    int GetNumberOfCustomers() const;
    std::list<Route>* GetRoutes();
	std::list<Route>* GetBestRoutes();
	void UpdateBest();
	~VRP();						//!< destructor
};

#endif /* VRP_H */
