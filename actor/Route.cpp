/*****************************************************************************
    This file is part of VRP.

    VRP is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    VRP is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with VRP.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/

#include "Route.h"


Route::Route(const Route& r) {
    this->initialCapacity = r.initialCapacity;
    this->capacity = r.capacity;
    this->initialWorkTime = r.initialWorkTime;
    this->workTime = r.workTime;
    this->graph = r.graph;
    this->totalCost = r.totalCost;
    this->ALPHA = r.ALPHA;
    this->TRAVEL_COST = r.TRAVEL_COST;
    this->SetAverageCost();
    for (StepType i : r.route) {
        this->route.push_back({i.first, i.second});
    }
}

/** @brief ###Constructor of Route.
 *
 * @param[in] c Initial capacity of the vehicle.
 * @param[in] wt Initial work time of the driver.
 * @param[in] g Graph of the customers.
 * @param[in] costTravel Cost parameter for each travel.
 * @param[in] alphaParam Alpha parameter for router evalutation.
 */
Route::Route(int c, float wt, const Graph g, const float costTravel, const float alphaParam) {
    this->initialCapacity = this->capacity = c;
    this->initialWorkTime = this->workTime = wt;
    this->graph = g;
    this->totalCost = 0;
    // if the service time is not a constraint reset the cost of travelling
    if (wt == std::numeric_limits<int>::max())
        this->TRAVEL_COST = (float)0;
    else
        this->TRAVEL_COST = costTravel;
    this->ALPHA = alphaParam;
    this->SetAverageCost();
}

/** @brief ###Close a route.
 *
 * Whenever the capacity or the work time are inadequate
 * close the route: return to depot.
 * @param[in] c Last customer to visit
 */
void Route::CloseTravel(const Customer c) {
    Customer depot = this->route.cbegin()->first;
    int costToDepot = this->graph.GetCosts(c, depot).second;
    this->totalCost += costToDepot;
    this->workTime = costToDepot * this->TRAVEL_COST;
    this->route.push_back({c, costToDepot});
    this->route.push_back({depot, 0});
}

/** @brief ###Close a route with the last customer.
 *
 * When remaining only one customer to visit, visit it then return to depot.
 * @param[in] from The last customer to visit
 * @param[in] depot The depot
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

/** @brief ###Travel from one customer to another
 *
 *  Check if is possibile to travel from a customer to another
 *  observing the constraint of capacity and time, if possibile add the
 *  travel to the route
 *  @param[in] from The source customer
 *  @param[in] to The destination customer
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

/** @brief ###Clear a route. */
void Route::EmptyRoute(const Customer depot) {
    this->route.clear();
    this->route.push_back({depot, 0});
}

/** @brief ###Print this route. */
void Route::PrintRoute() const {
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

/** @brief ###Return the size (number of customers) of a route. */
int Route::size() const {
    return this->route.size();
}

/** @brief ###Get the pointer to the route list. */
std::list<StepType>* Route::GetRoute() {
    return &this->route;
}

/** @brief ###Add a customer to this route.
 *
 * This function add a customer in the best position of a route respecting
 * the constraints: add the customer in each possible position, then execute
 * the best insertion.
 * @param[in] c The customer to insert
 */
bool Route::AddElem(const Customer c) {
    bool ret = false;
    // the map is sorted by costs
    std::map<int, Route> best;
    Customer depot = this->GetRoute()->front().first;
    std::list<StepType>::iterator it = this->route.begin();
    unsigned iter = 0;
    do {
        int travelCost, costNext, tCost, capac, fallbackCost;
        float workT;
        // work copy
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
        if (it == r.route.end() || (int)iter == r.size()) break;
        // compute next cost and time
        costNext = r.graph.GetCosts(c, it->first).second;
        tCost += costNext;
        workT -= costNext * r.TRAVEL_COST;
        fallbackCost = r.graph.GetCosts(before, it->first).second;
        tCost -= fallbackCost;
        workT += fallbackCost * r.TRAVEL_COST;
        // if the customer is visitable add to route in position 'it'
        if (workT >= 0) {
            r.totalCost = tCost;
            r.capacity = capac;
            r.workTime = workT;
            r.route.insert(it, {c, costNext});
            best.insert({r.GetTotalCost(), r});
        }
    } while (it != this->route.cend() && iter < (this->route.size() - 1));
    // if the route is changed return the best match
    if (best.size() > 0) {
        *this = (*best.begin()).second;
        ret = true;
    }else
        ret = false;
    return ret;
}

/** @brief ###Add a customer to this route.
 *
 * This function add a list of consecutive customers in the best position of a route respecting
 * the constraints: add the customer in each possible position, then execute
 * the best insertion.
 * @param[in] custs List of customers to insert
 */
bool Route::AddElem(const std::list<Customer> &custs) {
    // in this case 'this' need to be updated
    bool ret = false;
    // the map is sorted by costs
    std::map<int, Route> best;
    std::list<std::pair<Customer, int>>::iterator it = this->route.begin();
    unsigned iter = 0;
    do {
        int travelCost, costNext, tCost, capac, fallbackCost;
        int custsRequest, custsServiceTime;
        float workT;
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
    } while (it != this->route.cend() && (unsigned)iter < (this->route.size() - 1));
    // if the route is changed return the best match
    if (best.size() > 0) {
        *this = (*best.begin()).second;
        ret = true;
    }else
        ret = false;
    return ret;
}

/** @brief ###Remove a customer from a route.
 *
 * Remove a customer in a position in the route.
 * @param[in] it The position of the customer to remove
 * @return The state of the operation
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

/** @brief ###Remove a customer.
 *
 * Find and remove a customer from the route.
 * @param[in] c The customer to remove
 */
bool Route::RemoveCustomer(const Customer c) {
    if (c != this->route.front().first && c != this->route.back().first) {
        for (auto it = this->route.begin(); it != this->route.cend(); ++it) {
            if (it->first == c) {
                this->RemoveCustomer(it);
                return true;
            }
        }
    }
    return false;
}

/** @brief ###Return the cost of the route
 *
 *  @return Cost of the route
 */
int Route::GetTotalCost() const {
    return this->totalCost;
}

/** @brief ###Compute the average cost of all paths */
void Route::SetAverageCost() {
    this->averageCost = (float)this->totalCost / (this->route.size() - 1);
}

/** @brief ###Get the average cost of all paths */
float Route::GetAverageCost() const {
    return this->averageCost;
}

/** @brief ###List all customers with cost path lower the average.
 *
 * Create a list of customers which have a cost path lower than the average cost
 * of the route.
 * @param[in] customers Initial list of customers
 */
void Route::GetUnderAverageCustomers(std::list<Customer> &customers) {
    this->SetAverageCost();
    customers.clear();
    std::list<std::pair<Customer, int>>::const_iterator i = this->route.cbegin();
    for (; i != this->route.cend(); ++i) {
        if (i->second > 0 && i->second <= this->averageCost) {
            std::advance(i, 1);
            if (i->first != this->route.back().first) {
                customers.push_back(i->first);
                --i;
            }else {
                --i;
                customers.push_back(i->first);
            }
        }
    }
}

/** @brief ###Get the distance from two routes
 *
 * The distance from two routes is defined as the minimum distance from
 * each customers of the routes.
 * @param[in] r The route to compare with
 * @return      The distance from the two routes.
 */
float Route::GetDistanceFrom(Route r) {
    if (r == *this)
        return 0;
    std::list<StepType>::const_iterator it = this->route.cbegin();
    std::vector<float> min;
    // for each customers of each route (except the depot)
    for (++it; it->first != this->route.front().first; ++it) {
        std::list<StepType>::iterator ir = r.GetRoute()->begin();
        for (++ir; ir->first != r.GetRoute()->front().first; ++ir) {
            // compute the distance and update the min
            float v = std::sqrt(std::pow(it->first.x - ir->first.x,2) + std::pow(it->first.y - ir->first.y,2));
            min.push_back(v);
        }
    }
    return *min_element(min.begin(), min.end());
}

/** @brief ###Find a customer in the route
 *
 * Search for a customer in the route, if it is present return True,
 * otherwise False.
 * @param[in] c The customer to search for
 * @return The result
 */
bool Route::FindCustomer(const Customer c) {
	auto findIter = std::find_if(this->route.begin(), this->route.end(), [c](const auto e) { return e.first == c; });
    return findIter != this->route.end();
}

/** @brief ###Rebuild the route starting from a list of customers
 *
 * From a list of customers this function rebuild the route (checking all constraint).
 * @return True if the new route is valid
 */
bool Route::RebuildRoute(std::list<Customer> cust) {
    this->route.clear();
    this->totalCost = 0;
    this->capacity = this->initialCapacity;
    this->workTime = this->initialWorkTime;
    std::list<Customer>::iterator i = cust.begin();
    std::list<Customer>::iterator k = i;
    std::advance(k, 1);
    Customer depot = cust.front();
    for (; k != cust.end(); ++i, ++k) {
        int returnTime, tCost, capac, travelCost;
        float workT;
        // save the route state
        tCost = this->totalCost;
        capac = this->capacity;
        workT = this->workTime;
        travelCost = this->graph.GetCosts(*i, *k).second;
        tCost += travelCost;
        capac -= (*k).request;
        // service + travel time
        workT -= (*k).serviceTime + (travelCost * this->TRAVEL_COST);
        // must consider the time to return to depot
        returnTime = (this->graph.GetCosts(*k, depot).second) * this->TRAVEL_COST;
        // after the travel if constraints fails
        if (capac < 0 || workT < returnTime) {
            // no time or capacity to serve the customer: return to depot
            return false;
        } else {
            // the travel can be added to the route
            this->totalCost = tCost;
            this->capacity = capac;
            this->workTime = workT;
            this->route.push_back({*i, travelCost});
        }
    }
    this->route.push_back({depot, 0});
    return true;
}

/** @brief ###Quality assessment of the route.
 *
 * This function evaluate the 'quality' of the route checking the occupancy
 * in capacity and time terms.
 * Less is better.
 * @return The value of the assessment.
 */
float Route::Evaluate() {
    if (this->size() <= 2)
        return 0;
    return this->totalCost;
    /*float percLoad = (float(this->capacity) / this->initialCapacity) * 100;
    if (this->TRAVEL_COST == 0) {
        return (std::sqrt(percLoad) / this->totalCost) * 100;
    }
    float percTime = (float(this->workTime) / this->initialWorkTime) * 100;
    return (std::sqrt(percLoad * percTime) / this->totalCost) * 100;*/
}
