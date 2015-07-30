#include "Route.h"

/* constructor */
Route::Route(const int& c, const int& wt, const Graph &g) {
    this->capacity = c;
    this->workTime = wt;
    this->graph = g;
    this->totalCost = 0;
}

/* capacity 600 and worktime 480 */
bool Route::CheckViolations(const StepType &p, const Customer &c) {
    Customer depot;
    /* if empty the first is the depot */
    if (this->route.empty())
        depot = p.first;
    else {
        depot = this->route.begin()->first;
    }
    if (c == depot) {
        Customer last = this->route.back().first;
        if (last == depot)
            return false;
        this->route.back().second = this->graph.GetCosts(last, depot).second;
        /* stop the route with a 0 */
        this->route.push_back({depot, 0});
        return true;
    }
    /* saving the state */
    int tCost = this->totalCost;
    int capac = this->capacity;
    int workT = this->workTime;
    /* doing work */
    tCost += p.second;
    capac -= c.request;
    /* service time plus travel time */
    workT -= c.serviceTime + (tCost * 0.39);
    /* travel cost from c to depot */
    int returnTime = (this->graph.GetCosts(c, depot).second) * 0.39;
    /* check violation of capacity or working time */
    if (capac < 0 || workT < returnTime) {
        /* if the time/request exceed, revert and return to the depot (in time) */
        Customer last = this->route.back().first;
        /* change the last costs, point to depot */
        if (last != depot) {
            this->route.back().second = this->graph.GetCosts(last, depot).second;
            /* stop the route with a 0 */
            this->route.push_back({depot, 0});
        }
        return false;
    } else {
        /* update data */
        this->totalCost = tCost;
        this->capacity = capac;
        this->workTime = workT;
        this->route.push_back({p});
        return true;
    }
}

bool Route::InsertStep(StepType p, Customer c) {
    return this->CheckViolations(p, c);
}