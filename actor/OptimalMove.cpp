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

#include "OptimalMove.h"

float OptimalMove::UpdateDistanceAverage(Routes routes) {
    int count = std::pow(routes.size(), 2);
    float sum = 0;
    std::list<Route>::iterator i = routes.begin();
    for (; i != routes.end(); ++i) {
        std::list<Route>::const_iterator j = routes.cbegin();
        for (; j != routes.cend(); ++j)
            sum += i->GetDistanceFrom(*j);
    }
    return (sum / count);
}

/** @brief ###Remove all void routes. */
void OptimalMove::CleanVoid(Routes &routes) { routes.remove_if([](Route r){ return r.size() <= 2; }); }

/** @brief ###Move a customer from a route to another.
 *
 * This opt function try to move, for every route, a customer
 * from a route to another and removes empty route.
 * @return True if the routes are improved
 */
int OptimalMove::Opt10(Routes &routes, bool force) {
    float avg = this->UpdateDistanceAverage(routes);
    int diffCost = -1;
    std::list<Route>::iterator it = routes.begin();
    // list of best results from threads
    BestResult best = {{-1, -1}, {*it, *it}};
    // pool of threads
    ThreadPool pool(this->cores);
    for (int i = 0; it != routes.end(); std::advance(it, 1), ++i) {
        std::list<Route>::const_iterator jt = routes.cbegin();
        for (int j = 0; jt != routes.cend(); std::advance(jt, 1), ++j) {
            // if the route are close
            if (jt != it && it->GetDistanceFrom(*jt) < avg) {
                // create a thread to run Move1FromTo function and save the result in l list
                pool.AddJob([force, it, jt, i, j, &best, this]() {
                    Route tFrom = *it;
                    Route tTo = *jt;
                    // if the swap is done and the cost of routes is less than before
                    if (Move1FromTo(tFrom, tTo, force)) {
                        // wait until the lock is unlocked from an other thread, which is terminated
                        std::lock_guard<std::mutex> guard(this->mtx);
                        // LOCK acquired, if the cost of routes is better than the actual best, update the best
                        if ((tFrom.GetTotalCost() < best.second.first.GetTotalCost()
                                && tTo.GetTotalCost() < best.second.second.GetTotalCost()) || best.first.first == -1
                                || (force && tFrom.GetTotalCost() > best.second.first.GetTotalCost()
                                    && tTo.GetTotalCost() > best.second.second.GetTotalCost()))
                            best = std::make_pair(std::make_pair(i, j), std::make_pair(tFrom, tTo));
                    }
                });
            }
        }
    }
    // wait to finish all threads
    pool.JoinAll();
    // if some improvements are made update the routes
    if (best.first.first != -1) {
        std::list<Route>::iterator itFinal = routes.begin();
        int indexFrom = best.first.first;
        int indexTo = best.first.second;
        std::advance(itFinal, indexFrom);
        diffCost = itFinal->GetTotalCost();
        *itFinal = best.second.first;
        diffCost -= itFinal->GetTotalCost();
        itFinal = routes.begin();
        std::advance(itFinal, indexTo);
        diffCost += itFinal->GetTotalCost();
        *itFinal = best.second.second;
        diffCost -= itFinal->GetTotalCost();
        this->CleanVoid(routes);
        Utils::Instance().logger("opt10 improved: " + std::to_string(diffCost), Utils::VERBOSE);
    }else
        Utils::Instance().logger("opt10 no improvement", Utils::VERBOSE);
    return diffCost;
}

/** @brief ###Move a customer from a route to another.
 *
 * This opt function try to move, for every route, a customer
 * from a route to another and remove empty route.
 * @return True if the routes are improves
 */
int OptimalMove::Opt01(Routes &routes, bool force) {
    float avg = this->UpdateDistanceAverage(routes);
    int diffCost = -1;
    std::list<Route>::iterator it = routes.begin();
    BestResult best = {{-1, -1}, {*it, *it}};
    ThreadPool pool(this->cores);
    for (int i = 0; it != routes.end(); std::advance(it, 1), ++i) {
        std::list<Route>::const_iterator jt = routes.cbegin();
        for (int j = 0; jt != routes.cend(); std::advance(jt, 1), ++j) {
            if (jt != it && it->GetDistanceFrom(*jt) < avg) {
                pool.AddJob([force, it, jt, i, j, &best, this]() {
                    Route tFrom = *it;
                    Route tTo = *jt;
                    if (Move1FromTo(tTo, tFrom, force)) {
                        std::lock_guard<std::mutex> guard(this->mtx);
                        // LOCK acquired, if the cost of routes is better than the actual best, update the best
                        if ((tFrom.GetTotalCost() < best.second.first.GetTotalCost()
                                && tTo.GetTotalCost() < best.second.second.GetTotalCost()) || best.first.first == -1
                                || (force && tFrom.GetTotalCost() > best.second.first.GetTotalCost()
                                    && tTo.GetTotalCost() > best.second.second.GetTotalCost()))
                            best = std::make_pair(std::make_pair(i, j), std::make_pair(tFrom, tTo));
                    }
                });
            }
        }
    }
    pool.JoinAll();
    if (best.first.first != -1) {
        std::list<Route>::iterator itFinal = routes.begin();
        int indexFrom = best.first.first;
        int indexTo = best.first.second;
        std::advance(itFinal, indexFrom);
        diffCost = itFinal->GetTotalCost();
        *itFinal= best.second.first;
        diffCost -= itFinal->GetTotalCost();
        itFinal = routes.begin();
        std::advance(itFinal, indexTo);
        diffCost += itFinal->GetTotalCost();
        *itFinal = best.second.second;
        diffCost -= itFinal->GetTotalCost();
        this->CleanVoid(routes);
        Utils::Instance().logger("opt01 improved: " + std::to_string(diffCost), Utils::VERBOSE);
    }else
        Utils::Instance().logger("opt01 no improvement", Utils::VERBOSE);
    return diffCost;
}

/** @brief ###Move a customer from a route to another.
 *
 * This function tries to move a customer from a route to another trying
 * in every possible position if and only if the movement results in
 * an improvement of the total cost in both routes.
 * @param[in] source Route where to choose a random customer
 * @param[in] dest Route destination
 * @param[in] force Force the movement
 * @return True if the customer is moved.
 */
bool OptimalMove::Move1FromTo(Route &source, Route &dest, bool force) {
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
            if (temp.AddElem(itSource->first) && (temp.GetTotalCost() <= best || force)) {
                // copy the source route to check out if this configuration is valid and better
                Route copySource = source.CopyRoute();
                copySource.RemoveCustomer(itSource->first);
                if (copySource.size() <= 2 || copySource.GetTotalCost() <= sourceCost || force) {
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

/** @brief ###Swap two customers from routes.
 *
 * This opt function try to swap, for every route, a random customer
 * from a route with another random customer from the next.
 * @return True if the routes are improves
 */
int OptimalMove::Opt11(Routes &routes, bool force) {
    float avg = this->UpdateDistanceAverage(routes);
    int diffCost = -1;
    std::list<Route>::iterator it = routes.begin();
    BestResult best = {{-1, -1}, {*it, *it}};
    ThreadPool pool(this->cores);
    for (int i = 0; it != routes.end(); std::advance(it, 1), ++i) {
        std::list<Route>::const_iterator jt = routes.cbegin();
        for (int j = 0; jt != routes.cend(); std::advance(jt, 1), ++j) {
            if (jt != it && it->GetDistanceFrom(*jt) < avg) {
                pool.AddJob([force, it, jt, i, j, &best, this]() {
                    Route tFrom = *it;
                    Route tTo = *jt;
                    if (SwapFromTo(tFrom, tTo)) {
                        std::lock_guard<std::mutex> guard(this->mtx);
                        // LOCK acquired, if the cost of routes is better than the actual best, update the best
                        if ((tFrom.GetTotalCost() < best.second.first.GetTotalCost()
                                && tTo.GetTotalCost() < best.second.second.GetTotalCost()) || best.first.first == -1
                                || (force && tFrom.GetTotalCost() > best.second.first.GetTotalCost()
                                    && tTo.GetTotalCost() > best.second.second.GetTotalCost()))
                            best = std::make_pair(std::make_pair(i, j), std::make_pair(tFrom, tTo));
                    }
                });
            }
        }
    }
    pool.JoinAll();
    if (best.first.first != -1) {
        std::list<Route>::iterator itFinal = routes.begin();
        int indexFrom = best.first.first;
        int indexTo = best.first.second;
        std::advance(itFinal, indexFrom);
        diffCost = itFinal->GetTotalCost();
        *itFinal= best.second.first;
        diffCost -= itFinal->GetTotalCost();
        itFinal = routes.begin();
        std::advance(itFinal, indexTo);
        diffCost += itFinal->GetTotalCost();
        *itFinal = best.second.second;
        diffCost -= itFinal->GetTotalCost();
        this->CleanVoid(routes);
        Utils::Instance().logger("opt11 improved: " + std::to_string(diffCost), Utils::VERBOSE);
    }else
        Utils::Instance().logger("opt11 no improvement", Utils::VERBOSE);
    return diffCost;
}

/** @brief ###Swap two random customer from two routes.
 *
 * This function swap two customers from two routes.
 * @param[in] source First route
 * @param[in] dest Second route
 * @return True is the swap is successful
 */
bool OptimalMove::SwapFromTo(Route &source, Route &dest) {
    bool ret = false;
    int bestSourceCost = source.GetTotalCost(), bestDestCost = dest.GetTotalCost();
    int sourceCost = source.GetTotalCost();
    int removeSource = 0, removeDest = 0;
    Route bestRouteSource = source.CopyRoute(), bestRouteDest = dest.CopyRoute();
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
            if (temp.AddElem(itSource->first) && temp.GetTotalCost() <= bestDestCost) {
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
                if (temp.AddElem(itDest->first) && temp.GetTotalCost() <= bestSourceCost) {
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
    if (ret) {
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

/** @brief ###This function combine Opt01 and Opt11.
 *
 * This function swap two customers from the routes and moves one
 * customer from the second to the first route.
 * @return True if the routes are improves
 */
int OptimalMove::Opt12(Routes &routes, bool force) {
    float avg = this->UpdateDistanceAverage(routes);
    int diffCost = -1;
    std::list<Route>::iterator it = routes.begin();
    BestResult best = {{-1, -1}, {*it, *it}};
    ThreadPool pool(this->cores);
    for (int i = 0; it != routes.end(); std::advance(it, 1), ++i) {
        std::list<Route>::const_iterator jt = routes.cbegin();
        for (int j = 0; jt != routes.cend(); std::advance(jt, 1), ++j) {
            if (jt != it && it->GetDistanceFrom(*jt) < avg) {
                pool.AddJob([force, it, jt, i, j, &best, this]() {
                    Route tFrom = *it;
                    Route tTo = *jt;
                    if (AddRemoveFromTo(tFrom, tTo, 1, 2)) {
                        std::lock_guard<std::mutex> guard(this->mtx);
                        // LOCK acquired, if the cost of routes is better than the actual best, update the best
                        if ((tFrom.GetTotalCost() < best.second.first.GetTotalCost()
                                && tTo.GetTotalCost() < best.second.second.GetTotalCost()) || best.first.first == -1
                                || (force && tFrom.GetTotalCost() > best.second.first.GetTotalCost()
                                    && tTo.GetTotalCost() > best.second.second.GetTotalCost()))
                            best = std::make_pair(std::make_pair(i, j), std::make_pair(tFrom, tTo));
                    }
                });
            }
        }
    }
    pool.JoinAll();
    if (best.first.first != -1) {
        std::list<Route>::iterator itFinal = routes.begin();
        int indexFrom = best.first.first;
        int indexTo = best.first.second;
        std::advance(itFinal, indexFrom);
        diffCost = itFinal->GetTotalCost();
        *itFinal= best.second.first;
        diffCost -= itFinal->GetTotalCost();
        itFinal = routes.begin();
        std::advance(itFinal, indexTo);
        diffCost += itFinal->GetTotalCost();
        *itFinal = best.second.second;
        diffCost -= itFinal->GetTotalCost();
        this->CleanVoid(routes);
        Utils::Instance().logger("opt12 improved: " + std::to_string(diffCost), Utils::VERBOSE);
    }else
        Utils::Instance().logger("opt12 no improvement", Utils::VERBOSE);
    return diffCost;
}

/** @brief ###Move some customers from the routes.
 *
 * This function moves a number of customers from two routes and find the best
 * move checking all possible route configurations.
 * @param[in] source Route where to choose a random customer
 * @param[in] dest Route destination
 * @param[in] nInsert Number of customer to move from 'source' to 'dest'
 * @param[in] nRemove Number of customer to move from 'dest' to 'source'
 * @return True if the customer is moved.
 */
bool OptimalMove::AddRemoveFromTo(Route &source, Route &dest, int nInsert, int nRemove) {
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
            for (int i = 0; i < nInsert && copyit != source.GetRoute()->cend(); i++, ++copyit) {
                custsInsert.push_back(copyit->first);
                // remove nInsert consecutive customers from 'source'
                copyFrom.RemoveCustomer(copyit->first);
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
                for (int i = 0; i < nRemove && copyit != dest.GetRoute()->cend(); i++, ++copyit) {
                    custsRemove.push_back(copyit->first);
                    // remove nRemove customers from 'dest'
                    copyTo.RemoveCustomer(copyit->first);
                }
                if (copyit == dest.GetRoute()->cend()) break;
                if (tempFrom.AddElem(custsRemove) && copyTo.AddElem(custsInsert) &&
                        tempFrom.GetTotalCost() <= bestFrom && copyTo.GetTotalCost() <= bestTo) {
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

/** @brief ###This function combine Opt01 and Opt11.
 *
 * This function swap one customers from each routes and moves one
 * customer from the first to the second.
 * @return True if the routes are improves
 */
int OptimalMove::Opt21(Routes &routes, bool force) {
    float avg = this->UpdateDistanceAverage(routes);
    int diffCost = -1;
    std::list<Route>::iterator it = routes.begin();
    BestResult best = {{-1, -1}, {*it, *it}};
    ThreadPool pool(this->cores);
    for (int i = 0; it != routes.end(); std::advance(it, 1), ++i) {
        std::list<Route>::const_iterator jt = routes.cbegin();
        for (int j = 0; jt != routes.cend(); std::advance(jt, 1), ++j) {
            if (jt != it && it->GetDistanceFrom(*jt) < avg) {
                pool.AddJob([force, it, jt, i, j, &best, this]() {
                    Route tFrom = *it;
                    Route tTo = *jt;
                    if (AddRemoveFromTo(tFrom, tTo, 2, 1)) {
                        std::lock_guard<std::mutex> guard(this->mtx);
                        // LOCK acquired, if the cost of routes is better than the actual best, update the best
                        if ((tFrom.GetTotalCost() < best.second.first.GetTotalCost()
                                && tTo.GetTotalCost() < best.second.second.GetTotalCost()) || best.first.first == -1
                                || (force && tFrom.GetTotalCost() > best.second.first.GetTotalCost()
                                    && tTo.GetTotalCost() > best.second.second.GetTotalCost()))
                            best = std::make_pair(std::make_pair(i, j), std::make_pair(tFrom, tTo));
                    }
                });
            }
        }
    }
    pool.JoinAll();
    if (best.first.first != -1) {
        std::list<Route>::iterator itFinal = routes.begin();
        int indexFrom = best.first.first;
        int indexTo = best.first.second;
        std::advance(itFinal, indexFrom);
        diffCost = itFinal->GetTotalCost();
        *itFinal = best.second.first;
        diffCost -= itFinal->GetTotalCost();
        itFinal = routes.begin();
        std::advance(itFinal, indexTo);
        diffCost += itFinal->GetTotalCost();
        *itFinal = best.second.second;
        diffCost -= itFinal->GetTotalCost();
        this->CleanVoid(routes);
        Utils::Instance().logger("opt21 improved: " + std::to_string(diffCost), Utils::VERBOSE);
    }else
        Utils::Instance().logger("opt21 no improvement", Utils::VERBOSE);
    return diffCost;
}

/** @brief ###This function combines Opt01 and Opt11.
 *
 * This function swaps two customers from the first route
 * with two customers from the second.
 * @return True if the routes are improved
 */
int OptimalMove::Opt22(Routes &routes, bool force) {
    float avg = this->UpdateDistanceAverage(routes);
    int diffCost = -1;
    std::list<Route>::iterator it = routes.begin();
    BestResult best = {{-1, -1}, {*it, *it}};
    ThreadPool pool(this->cores);
    for (int i = 0; it != routes.end(); std::advance(it, 1), ++i) {
        std::list<Route>::const_iterator jt = routes.cbegin();
        for (int j = 0; jt != routes.cend(); std::advance(jt, 1), ++j) {
            if (jt != it && it->GetDistanceFrom(*jt) < avg) {
                pool.AddJob([force, it, jt, i, j, &best, this]() {
                    Route tFrom = *it;
                    Route tTo = *jt;
                    if (AddRemoveFromTo(tFrom, tTo, 2, 2)) {
                        std::lock_guard<std::mutex> guard(this->mtx);
                        // LOCK acquired, if the cost of routes is better than the actual best, update the best
                        if ((tFrom.GetTotalCost() < best.second.first.GetTotalCost()
                                && tTo.GetTotalCost() < best.second.second.GetTotalCost()) || best.first.first == -1
                                || (force && tFrom.GetTotalCost() > best.second.first.GetTotalCost()
                                    && tTo.GetTotalCost() > best.second.second.GetTotalCost()))
                            best = std::make_pair(std::make_pair(i, j), std::make_pair(tFrom, tTo));
                    }
                });
            }
        }
    }
    pool.JoinAll();
    if (best.first.first != -1) {
        std::list<Route>::iterator itFinal = routes.begin();
        int indexFrom = best.first.first;
        int indexTo = best.first.second;
        std::advance(itFinal, indexFrom);
        diffCost = itFinal->GetTotalCost();
        *itFinal = best.second.first;
        diffCost -= itFinal->GetTotalCost();
        itFinal = routes.begin();
        std::advance(itFinal, indexTo);
        diffCost += itFinal->GetTotalCost();
        *itFinal = best.second.second;
        diffCost -= itFinal->GetTotalCost();
        this->CleanVoid(routes);
        Utils::Instance().logger("opt22 improved: " + std::to_string(diffCost), Utils::VERBOSE);
    }else
        Utils::Instance().logger("opt22 no improvement", Utils::VERBOSE);
    return diffCost;
}

/** @brief ###Try to balance the routes.
 *
 * Find route with one or two customers and move it/them to another route:
 * execute an Opt10 to balance the route and get more occupancy.
 */
void OptimalMove::RouteBalancer(Routes &routes) {
    Routes temp = routes;
    bool stop = false;
    while(!stop) {
        bool reset = false;
        Routes::iterator it = temp.begin();
        for (int i = 0; it != temp.end() && !reset; ++it, i++) {
            if (it->size() == 3) {
                Routes::iterator jt = temp.begin();
                for (int j = 0; jt != temp.end(); ++jt, j++) {
                    if (jt != it && jt->size() > 4) {
                        if (this->Move1FromTo(*it, *jt, true)) {
                            this->CleanVoid(temp);
                            reset = true;
                            break;
                        }
                    }
                }
            }
        }
        if (reset || it == temp.end())
            stop = true;
    }
    routes = temp;
    Utils::Instance().logger("Routes balancing...", Utils::VERBOSE);
}

/** @brief ###Reorder the customers of route to delete cross over path.
 *
 * For every route swap two customer with distance lower than the average distance
 * of the route and save the best solution.
 * @return True if routes are improved
 */
bool OptimalMove::Opt2(Routes &routes) {
    bool ret = false;
    Routes::iterator it = routes.begin();
    int diffCost = 0;
    // for each route
    for (; it != routes.end(); ++it) {
        Route bestRoute = *it;
        ThreadPool pool(this->cores);
        int bestCost = it->GetTotalCost();
        std::list<StepType>::iterator i = it->GetRoute()->begin();
        std::advance(i, 1);
        for (; i->first != it->GetRoute()->back().first; ++i) {
            std::list<StepType>::iterator k = i;
            for (++k; k->first != it->GetRoute()->back().first; ++k) {
                pool.AddJob([i, k, it, &bestCost, &bestRoute, &ret, this]() {
                    // swap customers
                    Route tempRoute = this->Opt2Swap(*it, i->first, k->first);
                    std::lock_guard<std::mutex> guard(this->mtx);
                    if (tempRoute.GetTotalCost() <= bestCost) {
                        bestCost = tempRoute.GetTotalCost();
                        bestRoute = tempRoute;
                        ret = true;
                    }
                });
            }
        }
        pool.JoinAll();
        if (ret) {
            diffCost += it->GetTotalCost();
            *it = bestRoute;
            diffCost -= bestRoute.GetTotalCost();
        }
    }
    if (diffCost != 0) {
        Utils::Instance().logger("2-Opt improved: " + std::to_string(diffCost), Utils::VERBOSE);
    }else {
        ret = false;
        Utils::Instance().logger("2-Opt no improvement", Utils::VERBOSE);
    }
    return ret;
}

/** @brief ###Swap two customer in a route
 *
 * This function swap two customers in a route.
 * @param[in] route The route to work with
 * @param[in] i The first customer to swap
 * @param[in] k The second customer to swap
 * @return The new route with customers swapped
 */
Route OptimalMove::Opt2Swap(Route route, Customer i, Customer k) {
    std::list<Customer> cust;
    Route tempRoute = route.CopyRoute();
    RouteList::const_iterator it = tempRoute.GetRoute()->cbegin();
    // from start to i-1
    while(it->first != i) {
        cust.push_back(it->first);
        ++it;
    }
    it = tempRoute.GetRoute()->cend();
    while(it == tempRoute.GetRoute()->cend() || it->first != k) {
        --it;
    }
    // from i to k in reverse order
    while(it->first != i) {
        cust.push_back(it->first);
        --it;
    }
    // push i
    cust.push_back(i);
    // from k+1 to end
    it = route.GetRoute()->begin();
    while(it->first != k) {
        ++it;
    }
    ++it;
    while(it != route.GetRoute()->cend()) {
        cust.push_back(it->first);
        ++it;
    }
    // rebuild route
    Route ret = route.CopyRoute();
    if ((unsigned)tempRoute.size() == cust.size() && tempRoute.RebuildRoute(cust))
        ret = tempRoute;
    return ret;
}

/** @brief ###Reorder the customers of route to delete cross over path.
 *
 * For every route swap two customer with distance lower than the average distance
 * (the route crosses over itself) of the route and save the best solution.
 * @return True if routes are improved
 */
bool OptimalMove::Opt3(Routes &routes) {
    bool ret = false;
    Routes::iterator it = routes.begin();
    // Customers with short path (less than average)
    int diffCost = 0;
    // for each route
    for (; it != routes.end(); ++it) {
        Route bestRoute = *it;
        ThreadPool pool(this->cores);
        int bestCost = it->GetTotalCost();
        std::list<StepType>::iterator i = it->GetRoute()->begin();
        Customer depot = i->first;
        std::advance(i, it->GetRoute()->size() - 2);
        Customer lastK = i->first;
        std::advance(i, -1);
        Customer lastI = i->first;
        i = it->GetRoute()->begin();
        for (; i->first != lastI; ++i) {
            if (i->first != depot) {
                std::list<StepType>::iterator k = i;
                for (++k; k->first != lastK; ++k) {
                    if (k->first != depot) {
                        std::list<StepType>::iterator l = k;
                        for (++l; l != it->GetRoute()->end(); ++l) {
                            if (l->first != depot) {
                                std::list<StepType>::iterator m = l;
                                for (++m; m != it->GetRoute()->end(); ++m) {
                                    pool.AddJob([i, k, l, m, it, &bestCost, &bestRoute, &ret, this]() {
                                        // swap customers
                                        Route tempRoute = this->Opt3Swap(*it, i->first, k->first, l->first, m->first);
                                        std::lock_guard<std::mutex> guard(this->mtx);
                                        if (tempRoute.GetTotalCost() <= bestCost) {
                                            bestCost = tempRoute.GetTotalCost();
                                            bestRoute = tempRoute;
                                            ret = true;
                                        }
                                    });
                                }
                            }
                        }
                    }
                }
            }
        }
        pool.JoinAll();
        if (ret) {
            diffCost += it->GetTotalCost();
            *it = bestRoute;
            diffCost -= bestRoute.GetTotalCost();
        }
    }
    if (ret && diffCost != 0) {
        Utils::Instance().logger("3-Opt improved: " + std::to_string(diffCost), Utils::VERBOSE);
    }else {
        ret = false;
        Utils::Instance().logger("3-Opt no improvement", Utils::VERBOSE);
    }
    return ret;
}

/** @brief ###Swap three customer in a route
 *
 * This function swap two customers in a route.
 * @param[in] route The route to work with
 * @param[in] i The first customer to swap
 * @param[in] k The second customer to swap
 * @param[in] l The third customer to swap
 * @param[in] m The fourth customer to swap
 * @return The new route with customers swapped
 */
Route OptimalMove::Opt3Swap(Route route, Customer i, Customer k, Customer l, Customer m) {
    std::list<Customer> cust;
    Route tempRoute = route;
    RouteList::const_iterator it = tempRoute.GetRoute()->cbegin();
    // from start to i-1
    while(it->first != i) {
        cust.push_back(it->first);
        ++it;
    }
    // from i to k in reverse order
    it = tempRoute.GetRoute()->cend();
    while(it == tempRoute.GetRoute()->cend() || it->first != k) {
        --it;
    }
    while(it->first != i) {
        cust.push_back(it->first);
        --it;
    }
    // push i
    cust.push_back(i);
    // from k+1 to l-1
    while(it->first != k)
        ++it;
    ++it;
    while(it->first != l) {
        if (it != tempRoute.GetRoute()->cend())
            cust.push_back(it->first);
        ++it;
    }
    // from l to m in reverse order
    it = tempRoute.GetRoute()->cend();
    while(it == tempRoute.GetRoute()->cend() || it->first != m) {
        --it;
    }
    while (it->first != l) {
        if (it != tempRoute.GetRoute()->cend())
            cust.push_back(it->first);
        --it;
    }
    // push l
    cust.push_back(l);
    // from m+1 to end
    it = route.GetRoute()->cbegin();
    while(it->first != m && it != tempRoute.GetRoute()->cend()) {
        ++it;
    }
    if (it != tempRoute.GetRoute()->cend())
        ++it;
    while(it != route.GetRoute()->cend()) {
        cust.push_back(it->first);
        ++it;
    }
    // rebuild route
    Route ret = route;
    if ((unsigned)tempRoute.size() == cust.size() && tempRoute.RebuildRoute(cust))
        ret = tempRoute;
    return ret;
}