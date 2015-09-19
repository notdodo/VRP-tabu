#include "Route.h"

/** @brief Constructor of Route.
 *
 * @param c Initial capacity of the vehicle
 * @param wt Initial work time of the driver
 * @param g Graph of the customers
 */
Route::Route(int c, int wt, const Graph g) {
    this->initialCapacity = this->capacity = c;
    this->initialWorkTime = this->workTime = (float)wt;
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
    this->workTime = costToDepot * this->TRAVEL_COST;
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
    float workT = this->workTime;
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
    float workT = this->workTime;
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

/** @brief Return the size (number of customers) of a route. */
int Route::size() const {
    return this->route.size();
}

/** @brief Get the pointer to the route list. */
std::list<StepType>* Route::GetRoute() {
    return &this->route;
}

/** @brief Get a copy of the route class. */
Route Route::CopyRoute() {
    return *this;
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
    int travelCost, costNext, tCost, capac, fallbackCost;
    float workT;
    // the map is sorted by costs
    std::map<int, Route> best;
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
        if (iter == r.size()) break;
        // compute next cost and time
        costNext = r.graph.GetCosts(c, it->first).second;
        tCost += costNext;
        fallbackCost = r.graph.GetCosts(before, it->first).second;
        tCost -= fallbackCost;
        workT -= costNext * r.TRAVEL_COST;
        workT += fallbackCost * r.TRAVEL_COST;
        // if the customer is visitable add to route in position 'it'
        if (workT >= 0) {
            r.totalCost = tCost;
            r.capacity = capac;
            r.workTime = workT;
            r.route.insert(it, {c, costNext});
            best.insert({r.GetTotalCost(), r});
        }
    }while (it != this->route.cend() && (unsigned)iter < this->route.size());
    // if the route is changed return the best match
    if (best.size() > 0 && this->totalCost <= (*best.cbegin()).second.totalCost) {
        *this = (*best.cbegin()).second;
        ret = true;
    }else
        ret = false;
    return ret;
}

/** @brief Add a customer to this route.
 *
 * This function add a list of consecutive customers in the best position of a route respecting
 * the constraints: add the customer in each possible position, then execute
 * the best insertion.
 * @param custs List of customers to insert
 */
bool Route::AddElem(const std::list<Customer> custs) {
    // in this case 'this' need to be updated
    bool ret = false;
    int travelCost, costNext, tCost, capac, fallbackCost;
    int custsRequest, custsServiceTime;
    float workT;
    // the map is sorted by costs
    std::map<int, Route> best;
    std::list<std::pair<Customer, int>>::iterator it;
    int iter = 0;
    do {
        Route r = *this;
        it = r.route.begin();
        custsRequest = 0;
        custsServiceTime = 0;
        for (auto i = custs.cbegin(); i != custs.cend(); ++i) {
            custsRequest += (*i).request;
            custsServiceTime += (*i).serviceTime;
        }
        capac = r.capacity - custsRequest;
        workT = r.workTime - custsServiceTime;
        if (capac <= 0 || workT <= 0) break;
        tCost = r.totalCost;
        std::advance(it, iter);
        Customer before = it->first;
        Customer firstList = custs.front();
        // compute all the customers costs
        travelCost = r.graph.GetCosts(before, firstList).second;
        workT -= travelCost * r.TRAVEL_COST;
        tCost += travelCost;
        // update the new cost
        it->second = travelCost;
        Customer nextList = firstList;
        // compute work time and cost from all customers in list
        for (auto i = custs.cbegin(); i != custs.cend(); ++i) {
            if (nextList == *i) ++i;
            if (i == custs.cend()) break;
            travelCost = r.graph.GetCosts(nextList, *i).second;
            workT -= travelCost * r.TRAVEL_COST;
            tCost += travelCost;
            nextList = *i;
        }
        Customer lastList = nextList;
        // compute the next customer after the list (from the original route)
        std::advance(it, 1);
        iter++;
        // if end of route, stop
        if (iter == r.size()) break;
        // compute next cost and time
        Customer nextCustomer = it->first;
        costNext = r.graph.GetCosts(lastList, nextCustomer).second;
        tCost += costNext;
        fallbackCost = r.graph.GetCosts(before, nextCustomer).second;
        tCost -= fallbackCost;
        workT -= costNext * r.TRAVEL_COST;
        workT += fallbackCost * r.TRAVEL_COST;
        // if the customer is visitable add to route in position 'it'
        if (workT >= 0) {
            r.totalCost = tCost;
            r.capacity = capac;
            r.workTime = workT;
            // insert all the customers in the list
            auto i = custs.cbegin();
            std::advance(i, 1);
            nextList = custs.front();
            r.route.insert(it, {nextList, r.graph.GetCosts(custs.back(), nextCustomer).second});
            for (; i != custs.cend(); ++i) {
                if (nextList == *i) ++i;
                if (i == custs.cend()) break;
                r.route.insert(it, {*i, r.graph.GetCosts(nextList, *i).second});
                nextList = *i;
            }
            best.insert({r.GetTotalCost(), r});
        }
    }while (it != this->route.cend() && (unsigned)iter < this->route.size());
    // if the route is changed return the best match
    if (best.size() > 0 && this->totalCost < (*best.cbegin()).second.totalCost) {
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
    // if the route is depot -> customer -> depot delete the route
    if (this->route.size() > 3) {
        Customer del = it->first;
        // delete also request and service time
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
        // restore cost and work time
        int before = this->graph.GetCosts(it->first, del).second;
        this->workTime += before * this->TRAVEL_COST;
        this->totalCost -= before;
        // update cost and time skipping the 'del' customer
        int cost = this->graph.GetCosts(it->first, to).second;
        this->workTime -= cost * this->TRAVEL_COST;
        this->totalCost += cost;
        it->second = cost;
        // delete the customer from the route
        std::advance(it , 1);
        this->route.erase(it);
    } else
        this->EmptyRoute(this->route.front().first);
}

/** @brief Remove a customer.
 *
 * Find and remove a customer from the route.
 * @param c The customer to remove
 */
void Route::RemoveCustomer(const Customer c) {
    std::list<StepType>::iterator it;
    if (c != this->route.front().first || c != this->route.back().first) {
        for (it = this->route.begin(); it != this->route.cend(); it++) {
            if (it->first == c) {
                this->RemoveCustomer(it);
                break;
            }
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
    int travelCost, costNext, tCost, capac, fallbackCost;
    float workT;
    // the map is sorted by totalcost
    std::map<int, Route> best;
    Customer depot = this->GetRoute()->front().first;
    std::list<std::pair<Customer, int>>::iterator it;
    int iter = 0;
    do {
        Route r = *this;
        it = r.route.begin();
        // remove the customer, free resources
        r.RemoveCustomer(rem);
        capac = r.capacity - c.request;
        workT = r.workTime - c.serviceTime;
        if (capac < 0 || workT <= 0) break;
        tCost = r.totalCost;
        std::advance(it, iter);
        Customer before = it->first;
        // add the customer in 'it' position, the depot will be always the first
        travelCost = r.graph.GetCosts(before, c).second;
        // update travel cost
        it->second = travelCost;
        // remove the time to travel from it->fist to c
        workT -= travelCost * r.TRAVEL_COST;
        tCost += travelCost;
        // find the next customer
        std::advance(it, 1);
        iter++;
        // if end of route, stop
        if (iter == r.size()) break;
        // compute next cost and time
        costNext = r.graph.GetCosts(c, it->first).second;
        tCost += costNext;
        fallbackCost = r.graph.GetCosts(before, it->first).second;
        tCost -= fallbackCost;
        workT -= costNext * r.TRAVEL_COST;
        workT += fallbackCost * r.TRAVEL_COST;
        // if the customer is visitable add to route in position 'it'
        if (workT >= 0) {
            r.totalCost = tCost;
            r.capacity = capac;
            r.workTime = workT;
            r.route.insert(it, {c, costNext});
            best.insert({r.totalCost, r});
        }
    }while (it != this->route.cend() && (unsigned)iter < this->route.size());
    // if the route is changed return the best match
    if (best.size() > 0 && this->totalCost <= (*best.cbegin()).second.totalCost) {
        *this = (*best.cbegin()).second;
        ret = true;
    }else
        ret = false;
    return ret;
}

int Route::GetTotalCost() const {
    return this->totalCost;
}