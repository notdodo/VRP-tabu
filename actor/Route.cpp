#include "Route.h"

/* constructor */
Route::Route(const int& c, const int& wt, const Graph &g) {
    this->capacity = c;
    this->workTime = wt;
    this->graph = g;
    this->totalCost = 0;
}
// typedef std::pair<Customer, int> StepType;
bool Route::check(Customer from, Customer to, Customer depot) {
    bool ret = true;
    int returnTime = 0;
    // save the route state
    int tCost = this->totalCost;
    int capac = this->capacity;
    int workT = this->workTime;
    // check constraints
    int travelCost = this->graph.GetCosts(from, to).second;
    tCost += travelCost;
    capac -= to.request;
    // missing the travel time
    workT -= to.serviceTime + (tCost * this->TRAVEL_COST);
    // must consider the time to return to depot
    returnTime = (this->graph.GetCosts(to, depot).second) * this->TRAVEL_COST;
    // after the travel if constraints fails
    if ((capac < 0 || workT < returnTime)) {
        // no time or capacity to serve the customer: return to depot
        if (to != depot) {
            Customer last = this->route.back().first;
            this->route.back().second = this->graph.GetCosts(last, depot).second;
            this->route.push_back({depot, 0});
        }
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

bool Route::Travel(Customer from, Customer to) {
    Customer depot;
    if (this->route.empty())
        depot = from;
    else {
        depot = this->route.begin()->first;
    }
    if (to == depot) {
        Customer last = this->route.back().first;
        if (last != depot) {
            int costTravel = this->graph.GetCosts(to, depot).second;
            int returnTime = costTravel * this->TRAVEL_COST;
            if ((this->workTime - returnTime) > 0) {
                this->workTime -= returnTime;
                this->totalCost += costTravel;
                this->route.push_back({to, costTravel});
                this->route.push_back({depot, 0});
            } else {
                return false;
            }
            return true;
        }
    }
    return this->check(from, to, depot);
}

int Route::ReplaceLastWithDepot(Customer c, Customer depot) {
    int ret = 0;
    Customer last = this->route.back().first;
    if (last != depot) {
        this->route.pop_back();
        int costTravel = this->graph.GetCosts(last, depot).second;
        int returnTime = costTravel * this->TRAVEL_COST;
        // dal nuovo ultimo (ex penultimo) posso tornare a casa ci vado
        if ((this->workTime - returnTime) > 0) {
            this->workTime -= returnTime;
            this->totalCost += costTravel;
            this->route.push_back({depot, 0});
            ret++;
            return ret;
        } else {
            ret = this->ReplaceLastWithDepot(c, depot);
        }
    }
    return ret;
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
    }
    std::flush(std::cout);
}