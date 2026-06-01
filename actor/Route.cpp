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

namespace {
float TravelTime(int cost, float travelCost) { return static_cast<float>(cost) * travelCost; }
} // namespace

/** @brief Construct a route with vehicle constraints and scoring parameters.
 *
 * @param[in] c Initial capacity of the vehicle.
 * @param[in] wt Initial work time of the driver.
 * @param[in] g Graph of the customers.
 * @param[in] costTravel Cost parameter for each travel.
 * @param[in] alphaParam Alpha parameter for route evaluation.
 */
Route::Route(int c, float wt, const Graph& g, const float costTravel, const float alphaParam) {
    this->initialCapacity = this->capacity = c;
    this->initialWorkTime = this->workTime = wt;
    this->graph = &g;
    this->totalCost = 0;
    // if the service time is not a constraint reset the cost of travelling
    if (wt == std::numeric_limits<float>::max())
        this->TRAVEL_COST = 0.0f;
    else
        this->TRAVEL_COST = costTravel;
    this->ALPHA = alphaParam;
    this->SetAverageCost();
}

/** @brief Close a route from a specific customer to the depot.
 *
 * When remaining only one customer to visit, visit it then return to depot.
 * @param[in] from The last customer to visit
 * @param[in] depot The depot
 * @return True if the customer is visitable
 */
bool Route::CloseTravel(const Customer& from, const Customer& depot) {
    bool ret = true;
    // save the route state
    int tCost = this->totalCost;
    float workT = this->workTime;
    int travelCost = this->graph->GetCost(from, depot);
    tCost += travelCost;
    // must consider the time to return to depot
    workT -= TravelTime(travelCost, this->TRAVEL_COST);
    if (workT < 0) {
        ret = false;
    } else {
        // the travel can be added to the route
        this->totalCost = tCost;
        this->workTime = workT;
        this->route.emplace_back(from, travelCost);
        this->route.emplace_back(depot, 0);
    }
    return ret;
}

/** @brief Travel from one customer to another when constraints allow it.
 *
 *  Check if is possibile to travel from a customer to another
 *  observing the constraint of capacity and time, if possibile add the
 *  travel to the route
 *  @param[in] from The source customer
 *  @param[in] to The destination customer
 *  @return True if the travel is added to the route
 */
bool Route::Travel(const Customer& from, const Customer& to) {
    Customer depot;
    bool ret = true;
    // save the route state
    int tCost = this->totalCost;
    int capac = this->capacity;
    float workT = this->workTime;
    int travelCost = this->graph->GetCost(from, to);
    // get the depot
    if (this->route.empty()) {
        depot = from;
    } else {
        depot = this->route.cbegin()->first;
    }
    tCost += travelCost;
    capac -= to.request;
    // service + travel time
    workT -= static_cast<float>(to.serviceTime) + TravelTime(travelCost, this->TRAVEL_COST);
    // must consider the time to return to depot
    const float returnTime = TravelTime(this->graph->GetCost(to, depot), this->TRAVEL_COST);
    // after the travel if constraints fails
    if (capac < 0 || workT < returnTime) {
        // no time or capacity to serve the customer: return to depot
        ret = false;
    } else {
        // the travel can be added to the route
        this->totalCost = tCost;
        this->capacity = capac;
        this->workTime = workT;
        this->route.emplace_back(from, travelCost);
    }
    return ret;
}

/** @brief Clear this route and reset it to the depot.
 *
 * Resets both the route sequence and accumulated resource state. This is used
 * when the last customer is removed from a route; leaving the old cost or
 * capacity behind would make later route comparisons use stale values. The
 * route remains closed as depot -> depot so insertion helpers can reuse the
 * same logic they use for non-empty routes.
 */
void Route::EmptyRoute(const Customer& depot) {
    this->route.clear();
    this->capacity = this->initialCapacity;
    this->workTime = this->initialWorkTime;
    this->totalCost = 0;
    this->averageCost = 0.0F;
    this->route.emplace_back(depot, 0);
    this->route.emplace_back(depot, 0);
}

/** @brief Print this route. */
void Route::PrintRoute() const {
    std::flush(std::cout);
    if (this->route.size() > 1) {
        for (const StepType& i : this->route) {
            if (i.second > 0)
                std::cout << i.first << " -(" << i.second << ")-> ";
            else
                std::cout << i.first;
        }
        std::cout << '\n';
    } else {
        std::cout << "Void Route!\n";
    }
    std::flush(std::cout);
}

/** @brief Return the number of stored route steps. */
int Route::size() const { return static_cast<int>(this->route.size()); }

/** @brief Return a pointer to the route step list. */
RouteList* Route::GetRoute() { return &this->route; }
const RouteList* Route::GetRoute() const { return &this->route; }

/** @brief Add one customer to this route.
 *
 * This function add a customer in the best position of a route respecting
 * the constraints: add the customer in each possible position, then execute
 * the best insertion.
 * @param[in] c The customer to insert
 */
bool Route::AddElem(const Customer& c) {
    if (this->capacity < c.request || this->workTime < static_cast<float>(c.serviceTime) || this->route.size() < 2) {
        return false;
    }

    auto bestBefore = this->route.end();
    int bestTravelToCustomer = 0;
    int bestCustomerToNext = 0;
    int bestCost = 0;
    float bestWorkTime = 0.0F;
    bool found = false;

    for (auto before = this->route.begin(); std::next(before) != this->route.end(); ++before) {
        auto next = std::next(before);
        const int travelToCustomer = this->graph->GetCost(before->first, c);
        const int customerToNext = this->graph->GetCost(c, next->first);
        const int deltaCost = travelToCustomer + customerToNext - before->second;
        const int candidateCost = this->totalCost + deltaCost;
        const float candidateWorkTime =
            this->workTime - static_cast<float>(c.serviceTime) - TravelTime(deltaCost, this->TRAVEL_COST);
        if (candidateWorkTime < 0.0F) {
            continue;
        }
        if (!found || candidateCost < bestCost) {
            bestBefore = before;
            bestTravelToCustomer = travelToCustomer;
            bestCustomerToNext = customerToNext;
            bestCost = candidateCost;
            bestWorkTime = candidateWorkTime;
            found = true;
        }
    }

    if (!found) {
        return false;
    }

    this->capacity -= c.request;
    this->workTime = bestWorkTime;
    this->totalCost = bestCost;
    bestBefore->second = bestTravelToCustomer;
    this->route.insert(std::next(bestBefore), {c, bestCustomerToNext});
    return true;
}

/** @brief Add a customer sequence to this route.
 *
 * This function add a list of consecutive customers in the best position of a route respecting
 * the constraints: add the customer in each possible position, then execute
 * the best insertion.
 * @param[in] custs List of customers to insert
 */
bool Route::AddElem(const std::list<Customer>& custs) {
    if (custs.empty() || this->route.size() < 2) {
        return false;
    }

    int request = 0;
    int serviceTime = 0;
    int innerCost = 0;
    for (auto customer = custs.cbegin(); customer != custs.cend(); ++customer) {
        request += customer->request;
        serviceTime += customer->serviceTime;
        auto nextCustomer = std::next(customer);
        if (nextCustomer != custs.cend()) {
            innerCost += this->graph->GetCost(*customer, *nextCustomer);
        }
    }
    if (this->capacity < request || this->workTime < static_cast<float>(serviceTime)) {
        return false;
    }

    auto bestBefore = this->route.end();
    int bestCost = 0;
    int bestFirstArc = 0;
    int bestLastArc = 0;
    float bestWorkTime = 0.0F;
    bool found = false;
    const Customer& firstCustomer = custs.front();
    const Customer& lastCustomer = custs.back();

    for (auto before = this->route.begin(); std::next(before) != this->route.end(); ++before) {
        auto next = std::next(before);
        const int firstArc = this->graph->GetCost(before->first, firstCustomer);
        const int lastArc = this->graph->GetCost(lastCustomer, next->first);
        const int deltaCost = firstArc + innerCost + lastArc - before->second;
        const int candidateCost = this->totalCost + deltaCost;
        const float candidateWorkTime =
            this->workTime - static_cast<float>(serviceTime) - TravelTime(deltaCost, this->TRAVEL_COST);
        if (candidateWorkTime < 0.0F) {
            continue;
        }
        if (!found || candidateCost < bestCost) {
            bestBefore = before;
            bestCost = candidateCost;
            bestFirstArc = firstArc;
            bestLastArc = lastArc;
            bestWorkTime = candidateWorkTime;
            found = true;
        }
    }

    if (!found) {
        return false;
    }

    auto insertPosition = std::next(bestBefore);
    this->capacity -= request;
    this->workTime = bestWorkTime;
    this->totalCost = bestCost;
    bestBefore->second = bestFirstArc;
    auto customer = custs.cbegin();
    for (; customer != custs.cend(); ++customer) {
        auto nextCustomer = std::next(customer);
        const int outgoingCost =
            nextCustomer == custs.cend() ? bestLastArc : this->graph->GetCost(*customer, *nextCustomer);
        insertPosition = this->route.insert(insertPosition, {*customer, outgoingCost});
        ++insertPosition;
    }
    return true;
}

/** @brief Remove a customer at an iterator position.
 *
 * Remove a customer in a position in the route.
 * @param[in] it The position of the customer to remove
 * @return The state of the operation
 */
void Route::RemoveCustomer(RouteList::iterator& it) {
    // if the route is depot -> customer -> depot delete the route
    if (this->route.size() > 3) {
        Customer del = it->first;
        // delete also request and service time
        this->capacity += del.request;
        this->workTime += static_cast<float>(del.serviceTime);
        // go to the next customer
        std::advance(it, 1);
        Customer to = it->first;
        int after = this->graph->GetCost(del, to);
        this->workTime += TravelTime(after, this->TRAVEL_COST);
        this->totalCost -= after;
        // return to the previous customer
        std::advance(it, -2);
        // restore cost and work time
        int before = this->graph->GetCost(it->first, del);
        this->workTime += TravelTime(before, this->TRAVEL_COST);
        this->totalCost -= before;
        // StepType::second stores the outgoing arc cost, so the previous step
        // must be rewired directly to the customer after the deleted one.
        int cost = this->graph->GetCost(it->first, to);
        this->workTime -= TravelTime(cost, this->TRAVEL_COST);
        this->totalCost += cost;
        it->second = cost;
        // delete the customer from the route
        std::advance(it, 1);
        this->route.erase(it);
    } else {
        const Customer depot = this->route.front().first;
        this->EmptyRoute(depot);
    }
}

/** @brief Remove a customer.
 *
 * Find and remove a customer from the route.
 * @param[in] c The customer to remove
 */
bool Route::RemoveCustomer(const Customer& c) {
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

/** @brief Return the cost of the route.
 *
 *  @return Cost of the route
 */
int Route::GetTotalCost() const { return this->totalCost; }

/** @brief Return the vehicle capacity this route was created with. */
int Route::GetInitialCapacity() const { return this->initialCapacity; }

/** @brief Return the travel cost between two customers. */
int Route::GetTravelCost(const Customer& from, const Customer& to) const { return this->graph->GetCost(from, to); }

/** @brief Compute the average cost of all route arcs. */
void Route::SetAverageCost() {
    this->averageCost = this->route.size() <= 1
                            ? 0.0F
                            : static_cast<float>(this->totalCost) / static_cast<float>(this->route.size() - 1);
}

/** @brief Get the average cost of all route arcs. */
float Route::GetAverageCost() const { return this->averageCost; }

/** @brief List all customers reached by a below-average-cost path.
 *
 * Create a list of customers which have a cost path lower than the average cost
 * of the route.
 * @param[in] customers Initial list of customers
 */
void Route::GetUnderAverageCustomers(std::list<Customer>& customers) {
    this->SetAverageCost();
    customers.clear();
    RouteList::const_iterator i = this->route.cbegin();
    for (; i != this->route.cend(); ++i) {
        if (i->second > 0 && static_cast<float>(i->second) <= this->averageCost) {
            std::advance(i, 1);
            if (i->first != this->route.back().first) {
                customers.push_back(i->first);
                --i;
            } else {
                --i;
                customers.push_back(i->first);
            }
        }
    }
}

/** @brief Compute the distance between two routes.
 *
 * The distance from two routes is defined as the minimum distance from
 * each customers of the routes.
 * @param[in] r The route to compare with
 * @return      The distance from the two routes.
 */
float Route::GetDistanceFrom(const Route& r) const {
    if (r == *this)
        return 0;
    RouteList::const_iterator it = this->route.cbegin();
    std::vector<float> min;
    // for each customers of each route (except the depot)
    for (++it; it->first != this->route.front().first; ++it) {
        RouteList::const_iterator ir = r.GetRoute()->cbegin();
        for (++ir; ir->first != r.GetRoute()->front().first; ++ir) {
            // compute the distance and update the min
            const auto xDelta = static_cast<double>(it->first.x - ir->first.x);
            const auto yDelta = static_cast<double>(it->first.y - ir->first.y);
            float v = static_cast<float>(std::sqrt((xDelta * xDelta) + (yDelta * yDelta)));
            min.push_back(v);
        }
    }
    return *std::ranges::min_element(min);
}

/** @brief Find a customer in the route.
 *
 * Search for a customer in the route, if it is present return True,
 * otherwise False.
 * @param[in] c The customer to search for
 * @return The result
 */
bool Route::FindCustomer(const Customer& c) const {
    auto findIter = std::ranges::find_if(this->route, [c](const auto& e) { return e.first == c; });
    return findIter != this->route.cend();
}

/** @brief Rebuild the route from a list of customers.
 *
 * From a list of customers this function rebuild the route (checking all constraint).
 * @return True if the new route is valid
 */
bool Route::RebuildRoute(const std::list<Customer>& cust) {
    this->route.clear();
    this->totalCost = 0;
    this->capacity = this->initialCapacity;
    this->workTime = this->initialWorkTime;
    std::list<Customer>::const_iterator i = cust.begin();
    std::list<Customer>::const_iterator k = i;
    std::advance(k, 1);
    Customer depot = cust.front();
    for (; k != cust.end(); ++i, ++k) {
        int tCost, capac, travelCost;
        float workT;
        // save the route state
        tCost = this->totalCost;
        capac = this->capacity;
        workT = this->workTime;
        travelCost = this->graph->GetCost(*i, *k);
        tCost += travelCost;
        capac -= (*k).request;
        // service + travel time
        workT -= static_cast<float>((*k).serviceTime) + TravelTime(travelCost, this->TRAVEL_COST);
        // must consider the time to return to depot
        const float returnTime = TravelTime(this->graph->GetCost(*k, depot), this->TRAVEL_COST);
        // after the travel if constraints fails
        if (capac < 0 || workT < returnTime) {
            // no time or capacity to serve the customer: return to depot
            return false;
        } else {
            // the travel can be added to the route
            this->totalCost = tCost;
            this->capacity = capac;
            this->workTime = workT;
            // Store the source customer with the cost of the arc to the next customer.
            this->route.emplace_back(*i, travelCost);
        }
    }
    this->route.emplace_back(depot, 0);
    return true;
}

/** @brief Return the route quality assessment.
 *
 * This function evaluate the 'quality' of the route checking the occupancy
 * in capacity and time terms.
 * Less is better.
 * @return The value of the assessment.
 */
float Route::Evaluate() const {
    if (this->size() <= 2)
        return 0;
    return static_cast<float>(this->totalCost);
    /*float percLoad = (float(this->capacity) / this->initialCapacity) * 100;
    if (this->TRAVEL_COST == 0) {
        return (std::sqrt(percLoad) / this->totalCost) * 100;
    }
    float percTime = (float(this->workTime) / this->initialWorkTime) * 100;
    return (std::sqrt(percLoad * percTime) / this->totalCost) * 100;*/
}
