#include "Route.h"

/* constructor */
Route::Route(int c, int wt, const Graph g) {
    this->initialCapacity = this->capacity = c;
    this->initialWorkTime = this->workTime = wt;
    this->graph = g;
    this->totalCost = 0;
}

void Route::CloseTravel(const Customer c) {
    Customer depot = this->route.cbegin()->first;
    int costToDepot = this->graph.GetCosts(c, depot).second;
    this->totalCost += costToDepot;
    this->route.push_back({c, costToDepot});
    this->route.push_back({depot, 0});
}

bool Route::CloseTravel(const Customer from, const Customer depot) {
    bool ret = true;
    // save the route state
    int tCost = this->totalCost;
    int workT = this->workTime;
    int travelCost = this->graph.GetCosts(from, depot).second;
    tCost += travelCost;
    // must consider the time to return to depot
    workT -= (travelCost * this->TRAVEL_COST);
    if (workT < 0) {
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

bool Route::Travel(const Customer from, const Customer to) {
    Customer depot;
    bool ret = true;
    int returnTime;
    // save the route state
    int tCost = this->totalCost;
    int capac = this->capacity;
    int workT = this->workTime;
    int travelCost = this->graph.GetCosts(from, to).second;
    // get the depot
    if (this->route.empty()) {
        depot = from;
    }else {
        depot = this->route.cbegin()->first;
    }
    tCost += travelCost;
    capac -= to.request;
    // service + travel time
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
        this->capacity = capac;
        this->workTime = workT;
        this->route.push_back({from, travelCost});
    }
    return ret;
}

void Route::EmptyRoute(const Customer depot) {
    this->route.clear();
    this->route.push_back({depot, 0});
}

void Route::PrintRoute() {
    std::flush(std::cout);
    if (this->route.size() > 1) {
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

// how good is a route
void Route::SetFitness() {
    // route with 1, is perfect
    float cWeight = 6.0f;
    float tWeight = 8.0f;
    float cRate = 1.0f - (float)this->capacity / (float)this->initialCapacity;
    float tRate = 1.0f - (float)this->workTime / (float)this->initialWorkTime;
    this->fitness = (cRate * cWeight + tRate * tWeight) / (cWeight + tWeight);
}

int Route::size() const {
    return this->route.size();
}

std::list<StepType>* Route::GetRoute() {
    return &this->route;
}

bool Route::MoveCustomer(const Customer c) {
    bool ret = true;
    return ret;
}