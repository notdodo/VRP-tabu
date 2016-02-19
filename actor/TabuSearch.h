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
	int numCustomers;			/**< Number of customers */
	float fitness;
    float lambda = 0.0001f;     /**< Parameter for penalization of moves */
    int FindRouteFromCustomer(Customer, Routes);
    float Evaluate(Routes);
public:
	TabuSearch(const Graph &g, const int n) : graph(g), numCustomers(n) {};
	void Tabu(Routes &, int);
};

#endif /* TabuSearch_H */