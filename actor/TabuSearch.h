#ifndef TabuSearch_H
#define TabuSearch_H

#include "Route.h"
#include "TabuList.h"
#include <list>
#include <set>

typedef std::list<StepType> RouteList;
typedef std::list<Route> Routes;

class TabuSearch {
private:
	Graph graph;
	TabuList tabulist;          /**< List of all tabu moves */
	int numCustomers;
	float fitness;
    float lambda = 0.00011f;
    int FindRouteFromCustomer(Customer, Routes);
    float Evaluate(Routes);
    std::pair<Routes, float> IteratedLocalSearch(Routes &, int);
public:
	TabuSearch(const Graph &g, const int n) : graph(g), numCustomers(n) {};
	void Tabu(Routes &);
};

#endif /* TabuSearch_H */