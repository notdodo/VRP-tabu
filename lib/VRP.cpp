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

#include "VRP.h"

/** @brief Constructor of VRP.
 *
 * @param g The graph generate from the input
 * @param n The number of Customers
 * @param v The number of vehicles
 * @param c The capacity of each vehicle
 * @param t The work time of each driver
 */
VRP::VRP(const Graph g, const int n, const int v, const int c, const int t) {
    this->graph = g;
    this->numVertices = n;
    this->vehicles = v;
    this->capacity = c;
    this->workTime = t;
}

/** @brief This function creates the routes.
 *
 *  This function creates the initial solution of the algorithm.
 *  Customers are sorted in increasing order of distance from the depot.
 *  Starting with a random customer the route is created inserting one customer
 *  at time. Whenever the insertion of the customer would lead to a violation of
 *  the capacity or the working time a new route is created.
 *  @return Error or Warning code.
 */
int VRP::InitSolutions() {
    srand((unsigned)time(NULL));
    int start;
    Customer depot, last;
    /* keyring of distances */
    std::vector<int> keys;
    // the iterator for the sorted customer
    Map::iterator it;
    /* distances of customers sorted from depot */
    Map dist = this->graph.sortV0();
    /* get the depot and remove it from the map */
    depot = dist.begin()->second;
    dist.erase(dist.begin());
    /* choosing a random customer, from 0 to numVertices-1 */
    start = rand() % (this->numVertices-1);
    std::cout << start << std::endl;
    it = dist.begin();
    // getting the index (customer) to start with
    std::advance(it, start);
    int j = 1;
    // preparing the route
    Route r(this->capacity, this->workTime, this->graph);
    // doing the first step, from to first customer
    Map::const_iterator m = this->InsertStep(depot, it, it, r, dist);
    // saving the route
    this->routes.push_back(r);
    // for all vehicles, or one the Map dist is empty
    while (j < this->vehicles && !dist.empty()) {
        Route v(this->capacity, this->workTime, this->graph);
        // if is remaing only one customer add it to route
        if (dist.size() == 1) {
            last = m->second;
            v.Travel(depot, last);
            v.CloseTravel(last);
            dist.clear();
        }else {
            m = this->InsertStep(depot, it, m, v, dist);
        }
        this->routes.push_back(v);
        // counting the vehicles
        j++;
    }
    if (j < this->vehicles)
        j = -1;
    else if (j == this->vehicles && dist.size() == 0)
        j = 0;
    else
        j = 1;
    return j;
}

/** @brief This function creates a route.
 *
 * Starting from a customer this function creates the route until
 * a constraint result invalid looping the list of sorted customers.
 * @param depot The depot
 * @param stop The last customer, in the list in the j-1, where j is the initial random customer
 * @param i The customer which starts the route
 * @param r The route to create
 * @param distances The sorted list of customers
 */
Map::const_iterator VRP::InsertStep(Customer depot, Map::iterator stop, Map::const_iterator i, Route &r, Map &distances) {
    Customer from, to;
    Map::const_iterator index = i;
    Map::const_iterator end = distances.cend(); end--;
    Map::const_iterator last = end;
    Map::const_iterator fallback = index;
    // init the route with the travel from depot to first customer
    to = index->second;
    if (!r.Travel(depot, to)) {
        // if the customer is unreachable stop the program
        throw std::string("Customer" + to.name + " is unreachable!!!");
    }
    // increment the index, computing the next customer
    if (index != last)
        index++;
    else
        index = distances.begin();
    // the route need only the last customer (last before the stop)
    if (index == distances.cbegin() && index == stop) {
        r.CloseTravel(to);
        // clear the map to stop the loop
        distances.clear();
    }
    while (index != stop && !distances.empty()) {
        // set up from and to customers
        from = to;
        // a fallback index to recover the state
        fallback = index;
        to = index->second;
        // incrementing (if possible) the index for the next step of the route
        index++;
        if (index == distances.cend()) {
            std::advance(index, -1);
            to = index->second;
            // if cannot add the customer, close the route and return
            if (!r.Travel(from, to)) {
                r.CloseTravel(from);
                return fallback;
            } else {
                // otherwise index move to the next customer (the first one)
                index = distances.begin();
            }
        } else {
            if (!r.Travel(from, to)) {
                r.CloseTravel(from);
                // the route is closed but if remains the last customer to serve, return it
                if (stop != distances.cbegin())
                    std::advance(stop, -1);
                if (stop->second == to) {
                    // cannot insert the last customer, need to create a new route, only for it
                    distances.clear();
                    // a map with only this customer
                    Map::iterator last = distances.insert({0, to});
                    fallback = last;
                }
                break;
            }else {
                // the customer is inserted
                if (stop != distances.cbegin())
                    std::advance(stop, -1);
                // but if remain only one customer to serve, try to serve it
                if (stop == fallback) {
                    // no customers left
                    if (r.CloseTravel(to, depot)) {
                        distances.clear();
                        break;
                    }
                } else
                    std::advance(stop, 1);
            }
        }
    }
    // return the index fallback to start a new route
    return fallback;
}

/** @brief Return the routes.
 *
 * @return The pointer to the routes' list
 */
std::list<Route>* VRP::GetRoutes() {
    return &this->routes;
}

/** @brief Sort the list of routes by cost.
 *
 * This function sort the routes by costs in ascending order.
 */
void VRP::OrderByCosts() {
    this->routes.sort([](Route const &lhs, Route const &rhs) {
        return lhs.GetTotalCost() < rhs.GetTotalCost();
    });
}

/** @brief Remove all void routes. */
void VRP::CleanVoid() {
    this->routes.remove_if([](Route r){ return r.size() < 2; });
}

/** @brief Move a customer from a route to another.
 *
 * This opt function try to move, for every route, a customer
 * from a route to another and removes empty route.
 * @return True if the routes are improved
 */
bool VRP::Opt10() {
    bool flag = false;
    std::list<Route>::const_iterator it = this->routes.cbegin();
    // best combination of swap/move
    std::pair<Route, Route> bests = std::make_pair(*it, *it);
    int indexFrom, indexTo;
    // for each route choose one customer
    for (int i = 0; it != this->routes.cend(); ++it, ++i) {
        std::list<Route>::const_iterator jt = this->routes.cbegin();
        // try to move the chosen customer in every route
        for (int j = 0; jt != this->routes.cend(); ++jt, ++j) {
            if (jt != it) {
                Route tempFrom = *it;
                Route tempTo = *jt;
                int bestFrom = tempFrom.GetTotalCost();
                int bestTo = tempTo.GetTotalCost();
                // try to insert the customer in each position of the route and save the best combination
                if (Move1FromTo(tempFrom, tempTo, false) && tempFrom.GetTotalCost() < bestFrom && tempTo.GetTotalCost() < bestTo) {
                    std::get<0>(bests) = tempFrom;
                    std::get<1>(bests) = tempTo;
                    indexFrom = i;
                    indexTo = j;
                    flag = true;
                }
            }
        }
    }
    if (flag) {
        // update routes with the best movement
        std::list<Route>::iterator itFinal = this->routes.begin();
        std::advance(itFinal, indexFrom);
        *itFinal= std::get<0>(bests);
        itFinal = this->routes.begin();
        std::advance(itFinal, indexTo);
        *itFinal = std::get<1>(bests);
        this->CleanVoid();
    }
    return flag;
}

/** @brief Move a customer from a route to another.
 *
 * This opt function try to move, for every route, a customer
 * from a route to another and remove empty route.
 * @return True if the routes are improves
 */
bool VRP::Opt01() {
    bool flag = false;
    std::list<Route>::const_iterator it = this->routes.cbegin();
    std::pair<Route, Route> bests = std::make_pair(*it, *it);
    int indexFrom, indexTo;
    for (int i = 0; it != this->routes.cend(); ++it, ++i) {
        std::list<Route>::const_iterator jt = this->routes.cbegin();
        for (int j = 0; jt != this->routes.cend(); ++jt, ++j) {
            if (jt != it) {
                Route tempFrom = *it;
                Route tempTo = *jt;
                int bestFrom = tempFrom.GetTotalCost();
                int bestTo = tempTo.GetTotalCost();
                if (Move1FromTo(tempTo, tempFrom, false) && tempFrom.GetTotalCost() < bestFrom && tempTo.GetTotalCost() < bestTo) {
                    std::get<0>(bests) = tempFrom;
                    std::get<1>(bests) = tempTo;
                    indexFrom = i;
                    indexTo = j;
                    flag = true;
                }
            }
        }
    }
    if (flag) {
        std::list<Route>::iterator itFinal = this->routes.begin();
        std::advance(itFinal, indexFrom);
        *itFinal= std::get<0>(bests);
        itFinal = this->routes.begin();
        std::advance(itFinal, indexTo);
        *itFinal = std::get<1>(bests);
        this->CleanVoid();
    }
    return flag;
}

/** @brief Move a customer from a route to another.
 *
 * This function try to move a customer from a route to another trying
 * in every possible position if and only if the movement results in
 * an improvement of the total cost in both routes.
 * @param source Route where to choose a random customer
 * @param dest Route destination
 * @param force Force the movement
 * @return True if the customer is moved.
 */
bool VRP::Move1FromTo(Route &source, Route &dest, bool force) {
    bool ret = false;
    bool flag = false;
    int best = dest.GetTotalCost();
    int sourceCost = source.GetTotalCost();
    int index = 0;
    Route bestRoute = dest.CopyRoute();
    RouteList::iterator itSource = source.GetRoute()->begin();
    // cannot move the depot
    std::advance(itSource, 1);
    // for each position in source route
    for (int i = 1; itSource != source.GetRoute()->cend(); ++itSource, ++i) {
        // cannot move the depot
        if ((unsigned)i < source.GetRoute()->size() - 1) {
            // copy the destination route to try some path configuration
            Route temp = dest.CopyRoute();
            // find the best position and update (if needed) the best route
            if (temp.AddElem(itSource->first) && (temp.GetTotalCost() < best || force)) {
                // copy the source route to check out if this configuration is valid and better
                Route copySource = source.CopyRoute();
                copySource.RemoveCustomer(itSource->first);
                if (copySource.size() <= 2 || copySource.GetTotalCost() < sourceCost || force) {
                    best = temp.GetTotalCost();
                    bestRoute = temp;
                    // index of customer to remove
                    index = i;
                    flag = true;
                }
            }
        }
    }
    // if the route is better than before, update
    if (best < dest.GetTotalCost() || flag) {
        itSource = source.GetRoute()->begin();
        std::advance(itSource, index);
        source.RemoveCustomer(itSource);
        dest = bestRoute;
        ret = true;
    }
    return ret;
}

/** @brief Swap two customers from routes.
 *
 * This opt function try to swap, for every route, a random customer
 * from a route with another random customer from the next.
 * @return True if the routes are improves
 */
 bool VRP::Opt11() {
    bool flag = false;
    std::list<Route>::const_iterator it = this->routes.cbegin();
    std::pair<Route, Route> bests = std::make_pair(*it, *it);
    int indexFrom, indexTo;
    for (int i = 0; it != this->routes.cend(); ++it, ++i) {
        std::list<Route>::const_iterator jt = this->routes.cbegin();
        for (int j = 0; jt != this->routes.cend(); ++jt, ++j) {
            if (jt != it) {
                Route tempFrom = *it;
                Route tempTo = *jt;
                int bestFrom = tempFrom.GetTotalCost();
                int bestTo = tempTo.GetTotalCost();
                // swap the customers from a route to another and save the movement if better
                if (SwapFromTo(tempFrom, tempTo) && tempFrom.GetTotalCost() < bestFrom && tempTo.GetTotalCost() < bestTo) {
                    std::get<0>(bests) = tempFrom;
                    std::get<1>(bests) = tempTo;
                    indexFrom = i;
                    indexTo = j;
                    flag = true;
                }
            }
        }
    }
    if (flag) {
        std::list<Route>::iterator itFinal = this->routes.begin();
        std::advance(itFinal, indexFrom);
        *itFinal= std::get<0>(bests);
        itFinal = this->routes.begin();
        std::advance(itFinal, indexTo);
        *itFinal = std::get<1>(bests);
    }
    return flag;
}

/** @brief Swap two random customer from two routes.
 *
 * This function swap two customers from two routes.
 * @param source First route
 * @param dest Second route
 * @return True is the swap is successful
 */
bool VRP::SwapFromTo(Route &source, Route &dest) {
    bool ret = false;
    int bestSourceCost = source.GetTotalCost();
    int bestDestCost = dest.GetTotalCost();
    int sourceCost = source.GetTotalCost();
    int removeSource = 0;
    int removeDest = 0;
    Route bestRouteSource = source.CopyRoute();
    Route bestRouteDest = dest.CopyRoute();
    RouteList::iterator itSource = source.GetRoute()->begin();
    RouteList::iterator itDest;
    // cannot move the depot (start)
    std::advance(itSource, 1);
    // for each customer in the source route try to move it to the next route
    for (int i = 1; itSource != source.GetRoute()->cend(); std::advance(itSource, 1), i++) {
        // cannot move the depot (end)
        if ((unsigned) i < source.GetRoute()->size() - 1) {
            // copy the destination route to try some path configuration
            Route temp = dest.CopyRoute();
            // find the best position and update (if needed) the best route
            if (temp.AddElem(itSource->first) && temp.GetTotalCost() < bestDestCost) {
                // copy the source route to check out if this configuration is valid and better
                Route copySource = source.CopyRoute();
                copySource.RemoveCustomer(itSource->first);
                if (copySource.GetTotalCost() < sourceCost) {
                    bestDestCost = temp.GetTotalCost();
                    bestRouteDest = temp;
                    // index of customer to remove from the source route
                    removeSource = i;
                    ret = true;
                }
            }
        }
    }
    if (ret) {
        ret = false;
        // copy of the final source route (customer removed)
        Route copySourceTemp = source.CopyRoute();
        itSource = copySourceTemp.GetRoute()->begin();
        std::advance(itSource, removeSource);
        copySourceTemp.RemoveCustomer(itSource);
        bestSourceCost = copySourceTemp.GetTotalCost();
        // copy of the final dest route (customer added)
        Route copyDestTemp = bestRouteDest;
        bestDestCost = copyDestTemp.GetTotalCost();
        itDest = copyDestTemp.GetRoute()->begin();
        std::advance(itDest, 1);
        // for each customer in the destination route try to move it to the source route
        for (int i = 1; itDest != copyDestTemp.GetRoute()->cend(); std::advance(itDest, 1), i++) {
            // cannot move the depot (end)
            if ((unsigned) i < copyDestTemp.GetRoute()->size() - 1) {
                // copy the destination route to try some path configuration
                Route temp = copySourceTemp.CopyRoute();
                // find the best position and update (if needed) the best route
                if (temp.AddElem(itDest->first) && temp.GetTotalCost() < bestSourceCost) {
                    // copy the source route to check out if this configuration is valid and better
                    Route copyDest = copyDestTemp.CopyRoute();
                    copyDest.RemoveCustomer(itDest->first);
                    if (copyDest.GetTotalCost() < bestDestCost) {
                        bestSourceCost = temp.GetTotalCost();
                        bestRouteSource = temp;
                        // index of customer to remove from the source route
                        removeDest = i;
                        ret = true;
                    }
                }
            }
        }
    }
    // if the routes are better than before, update
    if (bestDestCost < dest.GetTotalCost() && bestSourceCost < source.GetTotalCost() && ret) {
        // remove the customer from the dest route
        itDest = bestRouteDest.GetRoute()->begin();
        std::advance(itDest, removeDest);
        bestRouteDest.RemoveCustomer(itDest);
        // update the routes
        dest = bestRouteDest;
        source = bestRouteSource;
    }
    return ret;
}

/** @brief This function combine Opt01 and Opt11.
 *
 * This function swap two customers from the routes and moves one
 * customer from the second to the first route.
 * @return True if the routes are improves
 */
bool VRP::Opt12() {
    bool flag = false;
    std::list<Route>::const_iterator it = this->routes.cbegin();
    std::pair<Route, Route> bests = std::make_pair(*it, *it);
    int indexFrom, indexTo;
    for (int i = 0; it != this->routes.cend(); std::advance(it, 1), ++i) {
        std::list<Route>::const_iterator jt = this->routes.cbegin();
        for (int j = 0; jt != this->routes.cend(); std::advance(jt, 1), ++j) {
            if (jt != it) {
                Route tempFrom = *it;
                Route tempTo = *jt;
                int bestFrom = tempFrom.GetTotalCost();
                int bestTo = tempTo.GetTotalCost();
                if (AddRemoveFromTo(tempFrom, tempTo, 1, 2) && tempFrom.GetTotalCost() < bestFrom && tempTo.GetTotalCost() < bestTo) {
                    std::get<0>(bests) = tempFrom;
                    std::get<1>(bests) = tempTo;
                    indexFrom = i;
                    indexTo = j;
                    flag = true;
                }
            }
        }
    }
    if (flag) {
        std::list<Route>::iterator itFinal = this->routes.begin();
        std::advance(itFinal, indexFrom);
        *itFinal= std::get<0>(bests);
        itFinal = this->routes.begin();
        std::advance(itFinal, indexTo);
        *itFinal = std::get<1>(bests);
        this->CleanVoid();
    }
    return flag;
}

/** @brief Move some customers from the routes.
 *
 * This function moves a number of customers from two routes and find the best
 * moves checking all possibile route configurations.
 * @param source Route where to choose a random customer
 * @param dest Route destination
 * @param nInsert Number of customer to move from 'source' to 'dest'
 * @param nRemove Number of customer to mvoe from 'dest' to 'source'
 * @return True if the customer is moved.
 */
bool VRP::AddRemoveFromTo(Route &source, Route &dest, int nInsert, int nRemove) {
    bool bestFound = false;
    RouteList::iterator itSource = source.GetRoute()->begin();
    RouteList::iterator itDest;
    std::list<Customer> custsInsert;
    std::list<Customer> custsRemove;
    int bestFrom = source.GetTotalCost();
    int bestTo = dest.GetTotalCost();
    std::pair<Route, Route> bests = std::make_pair(source, dest);
    if ((int)source.GetRoute()->size() > (nInsert + 2) && (int)dest.GetRoute()->size() > (nRemove + 2)) {
        // do not try to remove/insert the depot
        std::advance(itSource, 1);
        for (; itSource != source.GetRoute()->cend(); std::advance(itSource, 1)) {
            custsInsert.clear();
            Route copyFrom = source.CopyRoute();
            auto copyit = itSource;
            for (int i = 0; i < nInsert; i++) {
                custsInsert.push_back(copyit->first);
                // remove nInsert customers from 'source'
                copyFrom.RemoveCustomer(copyit->first);
                std::advance(copyit, 1);
                if (copyit == source.GetRoute()->cend()) break;
            }
            if (copyit == source.GetRoute()->cend()) break;
            itDest = dest.GetRoute()->begin();
            // do not try to remove/insert the depot
            std::advance(itDest, 1);
            for (; itDest != dest.GetRoute()->cend(); std::advance(itDest, 1)) {
                custsRemove.clear();
                // copy route is the clean route (with customers removed)
                Route tempFrom = copyFrom.CopyRoute();
                Route copyTo = dest.CopyRoute();
                auto copyit = itDest;
                for (int i = 0; i < nRemove; i++) {
                    custsRemove.push_back(copyit->first);
                    // remove nRemove customers from 'dest'
                    copyTo.RemoveCustomer(copyit->first);
                    std::advance(copyit, 1);
                    if (copyit == dest.GetRoute()->cend()) break;
                }
                if (copyit == dest.GetRoute()->cend()) break;
                if (tempFrom.AddElem(custsRemove) && copyTo.AddElem(custsInsert) &&
                        tempFrom.GetTotalCost() < bestFrom && copyTo.GetTotalCost() < bestTo) {
                    bestFound = true;
                    bestFrom = tempFrom.GetTotalCost();
                    bestTo = copyTo.GetTotalCost();
                    std::get<0>(bests) = tempFrom;
                    std::get<1>(bests) = copyTo;
                }
            }
        }
        if (bestFound) {
            source =  std::get<0>(bests);
            dest = std::get<1>(bests);
        }
    }
    return bestFound;
}

/** @brief This function combine Opt01 and Opt11.
 *
 * This function swap one customers from each routes and moves one
 * customer from the first to the second.
 * @return True if the routes are improves
 */
bool VRP::Opt21() {
    bool flag = false;
    std::list<Route>::const_iterator it = this->routes.cbegin();
    std::pair<Route, Route> bests = std::make_pair(*it, *it);
    int indexFrom, indexTo;
    for (int i = 0; it != this->routes.cend(); std::advance(it, 1), ++i) {
        std::list<Route>::const_iterator jt = this->routes.cbegin();
        for (int j = 0; jt != this->routes.cend(); std::advance(jt, 1), ++j) {
            if (jt != it) {
                Route tempFrom = *it;
                Route tempTo = *jt;
                int bestFrom = tempFrom.GetTotalCost();
                int bestTo = tempTo.GetTotalCost();
                if (AddRemoveFromTo(tempFrom, tempTo, 2, 1) && tempFrom.GetTotalCost() < bestFrom && tempTo.GetTotalCost() < bestTo) {
                    std::get<0>(bests) = tempFrom;
                    std::get<1>(bests) = tempTo;
                    indexFrom = i;
                    indexTo = j;
                    flag = true;
                }
            }
        }
    }
    if (flag) {
        std::list<Route>::iterator itFinal = this->routes.begin();
        std::advance(itFinal, indexFrom);
        *itFinal= std::get<0>(bests);
        itFinal = this->routes.begin();
        std::advance(itFinal, indexTo);
        *itFinal = std::get<1>(bests);
        this->CleanVoid();
    }
    return flag;
}

/** @brief This function combines Opt01 and Opt11.
 *
 * This function swaps two customers from the first route
 * with two customers from the second.
 * @return True if the routes are improved
 */
bool VRP::Opt22() {
    bool flag = false;
    std::list<Route>::const_iterator it = this->routes.cbegin();
    std::pair<Route, Route> bests = std::make_pair(*it, *it);
    int indexFrom, indexTo;
    for (int i = 0; it != this->routes.cend(); std::advance(it, 1), ++i) {
        std::list<Route>::const_iterator jt = this->routes.cbegin();
        for (int j = 0; jt != this->routes.cend(); std::advance(jt, 1), ++j) {
            if (jt != it) {
                Route tempFrom = *it;
                Route tempTo = *jt;
                int bestFrom = tempFrom.GetTotalCost();
                int bestTo = tempTo.GetTotalCost();
                if (AddRemoveFromTo(tempFrom, tempTo, 2, 2) && tempFrom.GetTotalCost() < bestFrom && tempTo.GetTotalCost() < bestTo) {
                    std::get<0>(bests) = tempFrom;
                    std::get<1>(bests) = tempTo;
                    indexFrom = i;
                    indexTo = j;
                    flag = true;
                }
            }
        }
    }
    if (flag) {
        std::list<Route>::iterator itFinal = this->routes.begin();
        std::advance(itFinal, indexFrom);
        *itFinal= std::get<0>(bests);
        itFinal = this->routes.begin();
        std::advance(itFinal, indexTo);
        *itFinal = std::get<1>(bests);
        this->CleanVoid();
    }
    return flag;
}

/** @brief Compute the total cost of routes.
 *
 * Compute the amount of costs for each route.
 * The cost is used to check improvement on paths.
 * @return The total cost
 */
int VRP::GetTotalCost() const {
    int tCost = 0;
    for (auto e : this->routes) {
        tCost += e.GetTotalCost();
    }
    return tCost;
}


/** @brief Try to balance the routes.
 *
 * Find route with one customer and move it to another route:
 * execute a Opt10.
 */
void VRP::RouteBalancer() {
    bool flag = false;
    std::list<Route>::const_iterator it = this->routes.cbegin();
    std::pair<Route, Route> bests = std::make_pair(*it, *it);
    int indexFrom, indexTo;
    // for each route
    for (int i = 0; it != this->routes.cend(); ++it, ++i) {
        if (it->size() == 3) {
            std::list<Route>::const_iterator jt = this->routes.cbegin();
            for (int j = 0; jt != this->routes.cend(); ++jt, ++j) {
                if (jt != it && jt->size() > 3) {
                    Route tempFrom = *it;
                    Route tempTo = *jt;
                    if (Move1FromTo(tempFrom, tempTo, true)) {
                        std::get<0>(bests) = tempFrom;
                        std::get<1>(bests) = tempTo;
                        indexFrom = i;
                        indexTo = j;
                        flag = true;
                    }
                }
            }
        }
    }
    if (flag) {
        std::list<Route>::iterator itFinal = this->routes.begin();
        std::advance(itFinal, indexFrom);
        *itFinal= std::get<0>(bests);
        itFinal = this->routes.begin();
        std::advance(itFinal, indexTo);
        *itFinal = std::get<1>(bests);
        this->CleanVoid();
    }
}

/** @brief Swap two customer for each route.
 *
 * For every route swap two customer with distance lower than the average distance
 * of the route and save the best solution.
 * @return True if routes are improved
 */
bool VRP::Opt2() {
    // per ogni route provo a fare 2opt:
    // ritorno true se c'è un miglioramento
    // per ogni route calcolo la media delle distanze e prendo i tratti minori o uguali alla media
    // per ogni punto trovato lo scambio con un altro compatibile 2optSwap(route, i, k) e salvo la combinazione migliore
    // 2optSwap(route, i, k):
    // la nuova route è costruita da route[1] a route[i-1], route[i], route[k], route[k+1] fino alle fine
    bool ret = false;
    std::list<Route>::iterator it = this->routes.begin();
    // Customers with short path (less than average)
    std::list<Customer> custs;
    int bestCost;
    // for each route
    for (; it != this->routes.end(); ++it) {
        Route bestRoute = *it;
        // get customers eligible to be swapped
        it->GetUnderAverageCustomers(custs);
        // need at least two customers
        if (custs.size() > 1) {
            bestCost = it->GetTotalCost();
            std::list<Customer>::iterator i = custs.begin();
            // for each pair of customers with min cost swap them e get the best result
            for (; *i != custs.back(); ++i) {
                std::list<Customer>::iterator k = i;
                for (k++; k != custs.end(); ++k) {
                    // swap customers
                    Route tempRoute = this->Opt2Swap(*it, i, k);
                    if (tempRoute.GetTotalCost() <= bestCost) {
                        bestCost = tempRoute.GetTotalCost();
                        bestRoute = tempRoute;
                    }
                }
            }
            // update the route
            *it = bestRoute;
        }
    }
    return ret;
}

/** @brief Swap two customer in a route
 *
 * This function swap two customers in a route.
 * @param route The route to work with
 * @param i The first customer to swap
 * @param k The second customer to swap
 * @return The new route with customers swapped
 */
Route VRP::Opt2Swap(Route route, std::list<Customer>::iterator i, std::list<Customer>::iterator k) {
    std::list<Customer> cust;
    Route tempRoute = route;
    RouteList::iterator it = tempRoute.GetRoute()->begin();
    // from start to i-1
    while((*it).first != *i) {
        cust.push_back((*it).first);
        ++it;
    }
    it = tempRoute.GetRoute()->end();
    it--;
    while((*it).first != *k) {
        --it;
    }
    // from i to k in reverse order
    while((*it).first != *i) {
        cust.push_back((*it).first);
        --it;
    }
    // push i
    cust.push_back(*i);
    it = route.GetRoute()->begin();
    while((*it).first != *k) {
        ++it;
    }
    ++it;
    // from k to end
    while(it != route.GetRoute()->end()) {
        cust.push_back((*it).first);
        ++it;
    }
    // rebuild route
    if (tempRoute.RebuildRoute(cust))
        return tempRoute;
    else
        return route;
}

/* destructor */
VRP::~VRP() {
}