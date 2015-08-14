#include "Route.h"

/** @brief Constructor of Route.
 *
 * @param c Initial capacity of the vehicle
 * @param wt Initial work time of the driver
 * @param g Graph of the customers
 */
Route::Route(int c, int wt, const Graph g) {
    this->initialCapacity = this->capacity = c;
    this->initialWorkTime = this->workTime = wt;
    this->graph = g;
    this->totalCost = 0;
}

/** @brief Close a route.
 *
 * Whenever the capacity or the work time are inadequate
 * close the route: return to depot.
 * @param c Last customer to visit
 */
void Route::CloseTravel(const Customer c) {
    Customer depot = this->route.cbegin()->first;
    int costToDepot = this->graph.GetCosts(c, depot).second;
    this->totalCost += costToDepot;
    this->route.push_back({c, costToDepot});
    this->route.push_back({depot, 0});
}

/** @brief Close a route with the last customer.
 *
 * When remaining only one customer to visit, visit it then return to depot.
 * @param from The last customer to visit
 * @param depot The depot
 * @return True if the customer is visitable
 */
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

/** @brief Travel from one customer to another
 *
 *  Check if is possibile to travel from a customer to another
 *  observing the constraint of capacity and time, if possibile add the
 *  travel to the route
 *  @param from The source customer
 *  @param to The destination customer
 *  @return True if the travel is added to the route
 */
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

/** @brief Clear a route. */
void Route::EmptyRoute(const Customer depot) {
    this->route.clear();
    this->route.push_back({depot, 0});
    this->SetFitness();
}

/** @brief Print this route. */
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

/** @brief Print a route.
 *
 * @param r The route to print
 */
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

/** @brief Estimate the goodness of a route.
 *
 * Compute the percent weighted arithmeic mean of a route.
 */
void Route::SetFitness() {
    // route with 1, is perfect
    float cRate = 1 - (float)this->capacity / (float)this->initialCapacity;
    float tRate = 1 - (float)this->workTime / (float)this->initialWorkTime;
    this->fitness = (cRate * cWeight + tRate * tWeight) / (cWeight + tWeight);
}

/** @brief Return the size (number of customers) of a route. */
int Route::size() const {
    return this->route.size();
}

/** @brief Get the pointer to this route. */
std::list<StepType>* Route::GetRoute() {
    return &this->route;
}

/** @brief Add a customer to this route.
 *
 * This function add a customer in the best position of a route respecting
 * the constraints: add the customer in each possible position, then execute
 * the best insertion.
 * @param c The customer to insert
 */
bool Route::AddElem(const Customer c) {
    // in this case 'this' need to be updated
    bool ret = false;
    int timeNext, travelCost, costNext, tCost, workT, capac, fallbackCost;
    // the map is sorted by fitness
    std::map<float, Route> best;
    Customer depot = this->GetRoute()->front().first;
    std::list<std::pair<Customer, int>>::iterator it;
    int iter = 0;
    do {
        Route r = *this;
        it = r.route.begin();
        capac = r.capacity - c.request;
        workT = r.workTime - c.serviceTime;
        if (capac <= 0 || workT <= 0) break;
        tCost = r.totalCost;
        std::advance(it, iter);
        Customer before = it->first;
        travelCost = r.graph.GetCosts(before, c).second;
        workT -= travelCost * r.TRAVEL_COST;
        tCost += travelCost;
        // update travel cost
        it->second = travelCost;
        std::advance(it, 1);
        iter++;
        // if end of route, stop
        if (it->first == depot) break;
        // compute next cost and time
        costNext = r.graph.GetCosts(c, it->first).second;
        tCost += costNext;
        fallbackCost = r.graph.GetCosts(c, it->first).second;
        tCost -= fallbackCost;
        timeNext = costNext * r.TRAVEL_COST;
        workT -= timeNext;
        workT += fallbackCost * r.TRAVEL_COST;
        // if the customer is visitable add to route in position 'it'
        if (workT >= 0) {
            r.totalCost = tCost;
            r.capacity = capac;
            r.workTime = workT;
            r.route.insert(it, {c, costNext});
            // calculate fitness
            r.SetFitness();
            best.insert({r.fitness, r});
        }
    }while (it != this->route.cend());
    // if the route is changed return the best match
    if (best.size() > 0 && this->fitness <= (*best.cbegin()).second.fitness) {
        *this = (*best.cbegin()).second;
        ret = true;
    }else
        ret = false;
    return ret;
}

/** @brief Remove a customer from a route.
 *
 * Remove a customer in a position in the route.
 * @param it The position of the customer to remove
 */
void Route::RemoveCustomer(std::list<StepType>::iterator &it) {
    Customer del = it->first;
    // delete also request and servite time
    this->capacity += del.request;
    this->workTime += del.serviceTime;
    // go to the next customer
    std::advance(it, 1);
    Customer to = it->first;
    int after = this->graph.GetCosts(del, to).second;
    this->workTime += after * this->TRAVEL_COST;
    this->totalCost -= after;
    // return to the previous customer
    std::advance(it, -2);
    // update cost and time
    int cost = this->graph.GetCosts(it->first, to).second;
    this->totalCost += cost;
    this->workTime -= cost * this->TRAVEL_COST;
    it->second = cost;
    int before = this->graph.GetCosts(it->first, del).second;
    this->totalCost -= before;
    this->workTime += before * this->TRAVEL_COST;
    // delete the customer from the route
    std::advance(it , 1);
    this->route.erase(it);
    // if a route is empty delete it
    if (this->route.size() <= 2)
        this->EmptyRoute(this->route.front().first);
    else
        this->SetFitness();
}

/** @brief Remove a customer.
 *
 * Find and remove a customer from the route.
 * @param c The customer to remove
 */
void Route::RemoveCustomer(const Customer c) {
    std::list<StepType>::iterator it;
    for (it = this->route.begin(); it != this->route.cend(); it++) {
        if (it->first == c) {
            this->RemoveCustomer(it);
            break;
        }
    }
}

/** @brief Add a customer and remove another.
 *
 *  Same as  Route::AddElem(const Customer c) but this function
 *  remove a customer before trying to add the customer.
 *  @param c Customer to add
 *  @param rem Customer to remove
 */
bool Route::AddElem(const Customer c, const Customer rem) {
    // in this case 'this' need to be updated
    bool ret = false;
    int timeNext, travelCost, costNext, tCost, workT, capac, fallbackCost;
    // the map is sorted by fitness
    std::map<float, Route> best;
    Customer depot = this->GetRoute()->front().first;
    std::list<std::pair<Customer, int>>::iterator it;
    int iter = 0;
    do {
        Route r = *this;
        it = r.route.begin();
        r.RemoveCustomer(rem);
        capac = r.capacity - c.request;
        workT = r.workTime - c.serviceTime;
        if (capac <= 0 || workT <= 0) break;
        tCost = r.totalCost;
        std::advance(it, iter);
        Customer before = it->first;
        travelCost = r.graph.GetCosts(before, c).second;
        workT -= travelCost * r.TRAVEL_COST;
        tCost += travelCost;
        // update travel cost
        it->second = travelCost;
        std::advance(it, 1);
        iter++;
        // if end of route, stop
        if (it->first == depot) break;
        // compute next cost and time
        costNext = r.graph.GetCosts(c, it->first).second;
        tCost += costNext;
        fallbackCost = r.graph.GetCosts(c, it->first).second;
        tCost -= fallbackCost;
        timeNext = costNext * r.TRAVEL_COST;
        workT -= timeNext;
        workT += fallbackCost * r.TRAVEL_COST;
        // if the customer is visitable add to route in position 'it'
        if (workT >= 0) {
            r.totalCost = tCost;
            r.capacity = capac;
            r.workTime = workT;
            r.route.insert(it, {c, costNext});
            // calculate fitness
            r.SetFitness();
            best.insert({r.fitness, r});
        }
    }while (it != this->route.cend());
    // if the route is changed return the best match
    if (best.size() > 0 && this->fitness <= (*best.cbegin()).second.fitness) {
        *this = (*best.cbegin()).second;
        ret = true;
    }else
        ret = false;
    return ret;
}