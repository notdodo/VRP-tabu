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

void Route::PrintRoute(std::list<StepType> r) {
    std::flush(std::cout);
    if (r.size() > 1) {
        for (StepType i : r) {
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

// Try to add a Customer to a route, return the best match
bool Route::AddElem(const Customer c) {
    // in this case 'this' need to be updated
    bool ret = false;
    int timeNext, travelCost, costNext, tCost, workT, capac;
    // the map is sorted by fitness
    std::map<float, Route> best;
    Customer depot = this->GetRoute()->front().first;
    // copy of the route, fallback
    Route r = *this;
    std::list<std::pair<Customer, int>> *newRoute = r.GetRoute();

    std::list<std::pair<Customer, int>>::iterator it = newRoute->begin();
    int iter = 0;
    capac = this->capacity - c.request;
    while(capac > 0 && it != this->route.cend()) {
        Route r = *this;
        newRoute = r.GetRoute();
        it = newRoute->begin();
        std::advance(it, iter);
        travelCost = this->graph.GetCosts(it->first, c).second;
        workT = this->workTime - (c.serviceTime + (travelCost * this->TRAVEL_COST));
        tCost = this->totalCost + travelCost;
        // update the travel cost
        it->second = travelCost;
        std::advance(it, 1);
        iter++;
        // if end of route, stop
        if (it->first == depot) break;
        // compute new costs
        costNext = this->graph.GetCosts(c, it->first).second;
        tCost += costNext;
        timeNext = costNext * this->TRAVEL_COST;
        workT -= timeNext;
        // if the customer is visitable add to route in position 'it'
        if (workT > 0 && capac > 0) {
            r.totalCost = tCost;
            r.capacity = capac;
            r.workTime = workT;
            newRoute->insert(it, {c, costNext});
            r.SetFitness();
            best.insert({r.fitness, r});
        }

    }
    // if the route is changed return the best match
    if (best.size() > 0) {
        auto top = best.cbegin();
        *this = top->second;
        ret = true;
    }else
        ret = false;
    return ret;
}

void Route::RemoveCustomer(std::list<StepType>::iterator &it) {
    // delete also request and servite time
    this->capacity += it->first.request;
    this->workTime += it->first.serviceTime;
    // go to the next customer
    std::advance(it, 1);
    Customer to = it->first;
    // return to the previous customer
    std::advance(it, -2);
    // update cost
    int cost = this->graph.GetCosts(it->first, to).second;
    this->totalCost -= cost;
    it->second = cost;
    // delete the customer from the route
    std::advance(it , 1);
    this->route.erase(it);
}