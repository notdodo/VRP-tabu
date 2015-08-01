#include "Route.h"

/* constructor */
Route::Route(int c, int wt, const Graph g) {
    this->capacity = c;
    this->workTime = wt;
    this->graph = g;
    this->totalCost = 0;
}

void Route::CloseTravel(Customer c) {
    Customer depot = this->route.cbegin()->first;
    int costToDepot = this->graph.GetCosts(c, depot).second;
    this->route.push_back({c, costToDepot});
    this->route.push_back({depot, 0});
}

bool Route::CloseTravel(Customer from, Customer depot) {
    // ho una route incompleta
    // devo inserire però l'ultimo cliente
    // ho già inserito v9->v7
    // devo controllare se posso inserire v7->v0
    bool ret = true;
    int returnTime = 0;
    // save the route state
    int tCost = this->totalCost;
    int workT = this->workTime;
    // check constraints
    int travelCost = this->graph.GetCosts(from, depot).second;
    tCost += travelCost;
    // missing the travel time
    workT -= (travelCost * this->TRAVEL_COST);
    // must consider the time to return to depot
    // after the travel if constraints fails
    if (workT < 0) {
        // no time or capacity to serve the customer: return to depot
        ret = false;
    } else {
        // the travel can be added to the route
        this->totalCost = tCost;
        this->workTime = workT;
        this->route.push_back({from, travelCost});
        this->route.push_back({depot, 0});
    }
    return ret;
}

bool Route::Travel(Customer from, Customer to) {
    Customer depot;
    bool ret = true;
    int returnTime = 0;
    // save the route state
    int tCost = this->totalCost;
    int capac = this->capacity;
    int workT = this->workTime;
    // check constraints
    int travelCost = this->graph.GetCosts(from, to).second;

    if (this->route.empty()) {
        depot = from;
    }else {
        depot = this->route.cbegin()->first;
    }

    tCost += travelCost;
    capac -= to.request;
    // missing the travel time
    workT -= to.serviceTime + (travelCost * this->TRAVEL_COST);
    // must consider the time to return to depot
    returnTime = (this->graph.GetCosts(to, depot).second) * this->TRAVEL_COST;
    // after the travel if constraints fails
    if (capac < 0 || workT < returnTime) {
        // no time or capacity to serve the customer: return to depot
        ret = false;
    } else {
        // the travel can be added to the route
        this->totalCost = tCost;
        this->capacity = capacity;
        this->workTime = workT;
        this->route.push_back({from, travelCost});
    }
    return ret;
}

int Route::ReplaceLastWithDepot(Customer c, Customer depot) {

}

void Route::PrintRoute() {
    std::flush(std::cout);
    if (this->route.size() > 0) {
        for (StepType i : this->route) {
            if (i.second > 0)
                std::cout << i.first << " -(" << i.second << ")-> ";
            else
                std::cout << i.first;
        }
        std::cout << std::endl;
    }else {
        std::cout << "Void Route!" << std::endl;
    }
    std::flush(std::cout);
}