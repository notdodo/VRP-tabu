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

/** @brief ###Constructor of VRP.
 *
 * @param[in] g The graph generate from the input.
 * @param[in] n The number of Customers.
 * @param[in] v The number of vehicles.
 * @param[in] c The capacity of each vehicle.
 * @param[in] t The work time of each driver.
 * @param[in] flagTime If the service time is a constraint.
 * @param[in] costTravel Cost parameter for each travel.
 * @param[in] alphaParam Alpha parameter for router evaluation.
 * @param[in] aspiration Aspiration factor.
 */
VRP::VRP(Graph g, const int n, const int v, const int c, const float t, const bool flagTime,
        const float costTravel, const float alphaParam) {
    this->mtx = new std::mutex();
    this->graph = g;
    this->numVertices = n;
    this->vehicles = v;
    this->capacity = c;
    // if no service time is needed
    if (flagTime)
        this->workTime = t;
    else
        this->workTime = std::numeric_limits<int>::max();
    this->costTravel = costTravel;
    this->totalCost = 0;
    this->alphaParam = alphaParam;
    // The Tabu list of moves
    TabuList tabulist;
    this->numberOfCores = std::thread::hardware_concurrency() + 1;
    this->averageDistance = 0;
}

/** @brief ###This function creates the routes.
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
    // the iterator for the sorted customer
    Map::const_iterator it;
    /* distances of customers sorted from depot */
    Map dist = this->graph.sortV0();
    /* get the depot and remove it from the map */
    depot = dist.cbegin()->second;
    dist.erase(dist.cbegin());
    /* choosing a random customer, from 0 to numVertices-1 */
    start = rand() % (this->numVertices-1);
    Utils::Instance().logger(std::to_string(start), Utils::VERBOSE);
    it = dist.cbegin();
    std::advance(it, start);
    // for each customer try to insert it in a route, otherwise create a new route
    while(!dist.empty()) {
        bool stop = true;
        // create an empty route
        Route v(this->capacity, this->workTime, this->graph, this->costTravel, this->alphaParam);
        Customer from, to;
        // start the route with a depot
		if (!v.Travel(depot, it->second))
            throw std::runtime_error("Customer unreachable!");
		// if one customer is left add it to route
        if (dist.size() == 1) {
            v.CloseTravel(it->second);
            stop = false;
            dist.clear();
        }
        // while the route is not empty
        while (stop) {
            from = it->second;
            Map::const_iterator fallback = it;
            ++it;
            if (it == dist.cend())
                it = dist.cbegin();
            to = it->second;
            // add path from 'from' to 'to'
            if(from != to && !v.Travel(from, to)) {
                stop = false;
                // if cannot add the customer try to close the route
                if (!v.CloseTravel(from, depot))
                    v.CloseTravel(from);
            // if the customer is added and remain one customer add it to route
            }else if (dist.size() == 1) {
                if (!v.CloseTravel(to, depot))
                    v.CloseTravel(from);
                stop = false;
            }
            dist.erase(fallback);
        }
        this->routes.push_back(v);
    }
    int j = 0;
    if (this->routes.size() < (unsigned)this->vehicles)
        j = -1;
    else if (this->routes.size() == (unsigned)this->vehicles)
        j = 0;
    else
        j = 1;
    Utils::Instance().logger("Routes created", Utils::VERBOSE);
    return j;
}

/** @brief ###Return the routes.
 *
 * @return The pointer to the routes list
 */
std::list<Route>* VRP::GetRoutes() { return &this->routes; }

/** @brief ###Return the best configuration of routes.
 *
 * @return The pointer to the routes list
 */
std::list<Route>* VRP::GetBestRoutes() { this->UpdateBest(); return &this->bestRoutes; }

/** @brief ###Save the best configuration.
 *
 * Finds out if the actual configuration is the best and save it.
 */
void VRP::UpdateBest() {
	int tCost = 0;
	for (auto e : this->bestRoutes)
		tCost += e.GetTotalCost();
    this->GetTotalCost();
	if (this->totalCost < tCost || tCost == 0) {
		this->bestRoutes.clear();
		std::copy(this->routes.cbegin(), this->routes.cend(), std::back_inserter(this->bestRoutes));
		Utils::Instance().SaveResult(this->bestRoutes);
	}
}

/** @brief ###Sort the list of routes by cost.
 *
 * This function sort the routes by costs in ascending order.
 */
void VRP::OrderByCosts() {
    this->routes.sort([](Route const &lhs, Route const &rhs) {
        return lhs.GetTotalCost() < rhs.GetTotalCost();
    });
}

/** @brief ###Remove all void routes. */
void VRP::CleanVoid() { this->routes.remove_if([](Route r){ return r.size() < 2; }); }

/** @brief ###Evaluate the assessment of the solution
 *
 * This function assess the quality of the solution
 */
void VRP::Evaluate() {
    std::list<Route>::iterator i = this->routes.begin();
    float tot = 0;
    for (; i != this->routes.end(); ++i) {
        tot += i->Evaluate();
    }
    this->fitness = tot / this->routes.size();
}

/** @brief ###Primary function, search for better solution and updates the tabu list
 *
 * This function run an iterated local search for a customer and try to insert neighbors
 * customers in the route. If the move isn't legal, updates the tabu list.
 * If the move is tabu but improve then update the route.
 * If no improvement are made choose the best of the worst.
 */
void VRP::TabuSearch() {
    // number of neighbors to consider
    int N = this->GetNumberOfCustomers() / this->routes.size() / 2;
    // route to insert the customer
	std::list<Route>::iterator itDest = this->routes.begin();
    std::list<Route>::iterator itSource = this->routes.begin();
    Customer depot;
    // {sourceCustomer, destCustomer}
    Move bestMove;
	float bestFitness = this->fitness;
	int bestCost = this->GetTotalCost();
	int initCost = this->GetTotalCost(), iteration = 0;
	while (initCost == this->GetTotalCost() && iteration < 20) {
		iteration++;
		// for each route
		for (; itDest != this->routes.end(); ++itDest) {
			// save the route
			Route destRoute = *itDest;
			RouteList::const_iterator ic = itDest->GetRoute()->cbegin();
			depot = ic->first;
			// null move: depot, depot
			bestMove = {depot, depot};
			this->Evaluate();
			// choose a customer to start from
			//std::advance(ic, ((rand() % (itDest->GetRoute()->size()-2)) + 1));
			++ic;
			for (; ic->first != depot; ++ic) {
				// find the neighborhood of the customer
				std::multimap<int, Customer> neigh = this->graph.GetNeighborhood(ic->first);
				std::multimap<int,Customer>::iterator i = neigh.begin();
				// for each customer in the neighborhood
				for (int iter = 0; i != neigh.end() && iter < N; ++i, ++iter) {
					// if the customer is not in the route
					if (!itDest->FindCustomer(i->second)) {
						destRoute = *itDest;
						Customer c = i->second;
						// source route
						itSource = this->routes.begin();
						// find the route which the customer belongs
						std::advance(itSource, this->FindRouteFromCustomer(c));
						Route sourceRoute = *itSource;
						// this a move
						Move pair = {ic->first, c};
						// swap customers
						if (destRoute.RemoveCustomer(ic->first) && sourceRoute.RemoveCustomer(c)
								&& destRoute.AddElem(c) && sourceRoute.AddElem(ic->first)) {
							Route originalSource = *itSource;
							Route originalDest = *itDest;
							*itSource = sourceRoute;
							*itDest = destRoute;
							this->Evaluate();
							float tempFitness = this->fitness;
							int tempCost = this->GetTotalCost();
							*itSource = originalSource;
							*itDest = originalDest;
							// choose the best candidate {non tabu and best fitness}:
							if (!this->tabulist.FindCheck(pair) || (tempFitness < bestFitness && tempCost < bestCost)) {
								bestMove = pair;
								bestFitness = tempFitness;
								bestCost = tempCost;
							}else {
								this->tabulist.AddNonBest(pair, tempFitness);
							}
						}
					}	
				}
			}
		}
		if (std::get<0>(bestMove) != depot && std::get<1>(bestMove) != depot) {
			// perform the move
			itDest = this->routes.begin();
			std::advance(itDest, this->FindRouteFromCustomer(std::get<0>(bestMove)));
			itSource = this->routes.begin();
			std::advance(itSource, this->FindRouteFromCustomer(std::get<1>(bestMove)));
			Route tempSource = *itSource;
			Route tempDest = *itDest;
			if (tempDest.RemoveCustomer(std::get<0>(bestMove)) && tempSource.RemoveCustomer(std::get<1>(bestMove)) &&
					tempDest.AddElem(std::get<1>(bestMove)) && tempSource.AddElem(std::get<0>(bestMove))) {
				this->tabulist.AddTabu(bestMove);
				*itSource = tempSource;
				*itDest = tempDest;
				this->Evaluate();
				this->CleanVoid();
			}
		}else {
			// choose a non best solution
			std::vector<std::pair<Move, float>> nonbest = this->tabulist.GetNonBest();
			for (auto i = nonbest.cbegin(); i != nonbest.cend(); ++i) {
				bestMove = i->first;
				itDest = this->routes.begin();
				std::advance(itDest, this->FindRouteFromCustomer(std::get<0>(bestMove)));
				itSource = this->routes.begin();
				std::advance(itSource, this->FindRouteFromCustomer(std::get<1>(bestMove)));
				Route tempSource = *itSource;
				Route tempDest = *itDest;
				if (tempDest.RemoveCustomer(std::get<0>(bestMove)) && tempSource.RemoveCustomer(std::get<1>(bestMove)) &&
						tempDest.AddElem(std::get<1>(bestMove)) && tempSource.AddElem(std::get<0>(bestMove))) {
					this->tabulist.AddTabu(bestMove);
					*itSource = tempSource;
					*itDest = tempDest;
					this->Evaluate();
					this->CleanVoid();
				}
			}
		}
	}
    this->tabulist.Clean();
    // round finished, unblock some moves
    if (!this->CheckIntegrity()) throw std::runtime_error("Ops! Something went wrong!!");
}

/** @brief ###Find the route to which a customer belongs
 *
 * This function finds the route to which a customer belongs and return the
 * index of the route.
 * @param[in] c The customer to find
 * @return The index of the route
 */
int VRP::FindRouteFromCustomer(Customer c) {
    std::list<Route>::iterator it = this->routes.begin();
    int ret = -1;
    bool flag = false;
    for(; it != this->routes.end() && !flag; ++it, ++ret) {
        RouteList::const_iterator l = it->GetRoute()->cbegin();
        for(; l != it->GetRoute()->cend() && !flag; ++l) {
            if (l->first == c)
                flag = true;
		}
	}
    return ret;
}

/** @brief ###Check the integrity of the system
 *
 * Each customer have to be visited only once, this function checks if
 * this constraint is valid in the system. Used for debugging.
 * @return The status of the constraint
 */
bool VRP::CheckIntegrity() {
    std::list<Customer> checked;
    std::list<Route>::iterator ir = this->routes.begin();
    for(; ir != this->routes.end(); ++ir) {
        RouteList::const_iterator ic = ir->GetRoute()->cbegin();
        Customer depot = ic->first;
        ++ic;
        for(; ic != ir->GetRoute()->cend(); ++ic) {
            if (ic->first != depot) {
                std::list<Customer>::const_iterator findIter = std::find(checked.cbegin(), checked.cend(), ic->first);
                if (findIter == checked.cend())
                    checked.push_back(ic->first);
                else
                    return false;
            }
        }
    }
    return true;
}

void VRP::UpdateDistanceAverage() {
    int count = std::pow(this->routes.size(), 2);
    float sum = 0;
    std::list<Route>::iterator i = this->routes.begin();
    ThreadPool* pool = new ThreadPool((int)this->numberOfCores);
    for (; i != this->routes.end(); ++i) {
        std::list<Route>::const_iterator j = this->routes.cbegin();
        for (; j != this->routes.cend(); ++j)
            if (i != j){
                auto result = pool->enqueue([i, j]() {
                    return i->GetDistanceFrom(*j);
                });
                sum += result.get();
            }
    }
    delete pool;
    this->averageDistance = sum / count;
}

/** @brief ###Move a customer from a route to another.
 *
 * This opt function try to move, for every route, a customer
 * from a route to another and removes empty route.
 * @return True if the routes are improved
 */
int VRP::Opt10(bool force) {
    this->UpdateDistanceAverage();
	int diffCost = -1;
    std::list<Route>::iterator it = this->routes.begin();
    // list of best results from threads
    ResultList l;
    // pool of threads
    ThreadPool* pool = new ThreadPool((int)this->numberOfCores);
    for (int i = 0; it != this->routes.end(); std::advance(it, 1), ++i) {
        std::list<Route>::const_iterator jt = this->routes.cbegin();
        for (int j = 0; jt != this->routes.cend(); std::advance(jt, 1), ++j) {
            // if the route are close
            if (jt != it && it->GetDistanceFrom(*jt) < this->averageDistance) {
                // create a thread to run Move1FromTo function and save the result in l list
                pool->enqueue([force, it, jt, i, j, &l, this]() {
                    Route tFrom = *it;
                    Route tTo = *jt;
                    // if the swap is done and the cost of routes is less than before
                    if (Move1FromTo(tFrom, tTo, force)) {
                        // wait until the lock is unlocked from an other thread, which is terminated
                        std::lock_guard<std::mutex> lock(*this->mtx);
                        // LOCK acquired, if the cost of routes is better than the actual best, update the list
                        if ((tFrom.GetTotalCost() < l.front().second.first.GetTotalCost() &&
                                tTo.GetTotalCost() < l.front().second.second.GetTotalCost()) ||
                                l.size() == 0 || force)
                            l.push_front(std::make_pair(std::make_pair(i, j), std::make_pair(tFrom, tTo)));
                    }
                });
            }
        }
    }
    // wait to finish all threads
    delete pool;
    // if some improvements are made update the routes
    if (l.size() > 0) {
        std::list<Route>::iterator itFinal = this->routes.begin();
        int indexFrom = l.front().first.first;
        int indexTo = l.front().first.second;
        std::advance(itFinal, indexFrom);
        diffCost = itFinal->GetTotalCost();
        *itFinal = l.front().second.first;
        diffCost -= itFinal->GetTotalCost();
        itFinal = this->routes.begin();
        std::advance(itFinal, indexTo);
        diffCost += itFinal->GetTotalCost();
        *itFinal = l.front().second.second;
        diffCost -= itFinal->GetTotalCost();
        this->CleanVoid();	
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
int VRP::Opt01(bool force) {
    this->UpdateDistanceAverage();
	int diffCost = -1;
    std::list<Route>::iterator it = this->routes.begin();
    ResultList l;
    ThreadPool* pool = new ThreadPool((int)this->numberOfCores);
    for (int i = 0; it != this->routes.end(); std::advance(it, 1), ++i) {
        std::list<Route>::const_iterator jt = this->routes.cbegin();
        for (int j = 0; jt != this->routes.cend(); std::advance(jt, 1), ++j) {
            if (jt != it && it->GetDistanceFrom(*jt) < this->averageDistance) {
                pool->enqueue([force, it, jt, i, j, &l, this]() {
                    Route tFrom = *it;
                    Route tTo = *jt;
                    if (Move1FromTo(tTo, tFrom, force)) {
						std::lock_guard<std::mutex> lock(*this->mtx);
                        if ((tFrom.GetTotalCost() < l.front().second.first.GetTotalCost() &&
                                tTo.GetTotalCost() < l.front().second.second.GetTotalCost())
                                || l.size() == 0 || force)
                            l.push_front(std::make_pair(std::make_pair(i, j), std::make_pair(tFrom, tTo)));
                    }
                });
            }
        }
    }
    delete pool;
    if (l.size() > 0) {
        std::list<Route>::iterator itFinal = this->routes.begin();
        int indexFrom = l.front().first.first;
        int indexTo = l.front().first.second;
        std::advance(itFinal, indexFrom);
        diffCost = itFinal->GetTotalCost();
        *itFinal= l.front().second.first;
        diffCost -= itFinal->GetTotalCost();
        itFinal = this->routes.begin();
        std::advance(itFinal, indexTo);
        diffCost += itFinal->GetTotalCost();
        *itFinal = l.front().second.second;
        diffCost -= itFinal->GetTotalCost();
        this->CleanVoid();
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
int VRP::Opt11(bool force) {
    this->UpdateDistanceAverage();
	int diffCost = -1;
    std::list<Route>::iterator it = this->routes.begin();
    ResultList l;
    ThreadPool* pool = new ThreadPool((int)this->numberOfCores);
    for (int i = 0; it != this->routes.end(); std::advance(it, 1), ++i) {
        std::list<Route>::const_iterator jt = this->routes.cbegin();
        for (int j = 0; jt != this->routes.cend(); std::advance(jt, 1), ++j) {
            if (jt != it && it->GetDistanceFrom(*jt) < this->averageDistance) {
                pool->enqueue([force, it, jt, i, j, &l, this]() {
                    Route tFrom = *it;
                    Route tTo = *jt;
                    if (SwapFromTo(tFrom, tTo)) {
                        std::lock_guard<std::mutex> lock(*this->mtx);
                        if ((tFrom.GetTotalCost() < l.front().second.first.GetTotalCost()
                                && tTo.GetTotalCost() < l.front().second.second.GetTotalCost())
                                || l.size() == 0 || force)
                            l.push_front(std::make_pair(std::make_pair(i, j), std::make_pair(tFrom, tTo)));
                    }
                });
            }
        }
    }
    delete pool;
    if (l.size() > 0) {
        std::list<Route>::iterator itFinal = this->routes.begin();
        int indexFrom = l.front().first.first;
        int indexTo = l.front().first.second;
        std::advance(itFinal, indexFrom);
        int diffCost = itFinal->GetTotalCost();
        *itFinal= l.front().second.first;
        diffCost -= itFinal->GetTotalCost();
        itFinal = this->routes.begin();
        std::advance(itFinal, indexTo);
        diffCost += itFinal->GetTotalCost();
        *itFinal = l.front().second.second;
        diffCost -= itFinal->GetTotalCost();
        this->CleanVoid();
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
bool VRP::SwapFromTo(Route &source, Route &dest) {
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
int VRP::Opt12(bool force) {
    this->UpdateDistanceAverage();
	int diffCost = -1;
    std::list<Route>::iterator it = this->routes.begin();
    ResultList l;
    ThreadPool* pool = new ThreadPool((int)this->numberOfCores);
    for (int i = 0; it != this->routes.end(); std::advance(it, 1), ++i) {
        std::list<Route>::const_iterator jt = this->routes.cbegin();
        for (int j = 0; jt != this->routes.cend(); std::advance(jt, 1), ++j) {
            if (jt != it && it->GetDistanceFrom(*jt) < this->averageDistance) {
				pool->enqueue([force, it, jt, i, j, &l, this]() {
                    Route tFrom = *it;
                    Route tTo = *jt;
                    if (AddRemoveFromTo(tFrom, tTo, 1, 2)) {
						std::lock_guard<std::mutex> lock(*this->mtx);
                        if ((tFrom.GetTotalCost() < l.front().second.first.GetTotalCost()
                                && tTo.GetTotalCost() < l.front().second.second.GetTotalCost())
                                || l.size() == 0 || force)
                            l.push_front(std::make_pair(std::make_pair(i, j), std::make_pair(tFrom, tTo)));
                    }
                });
            }
        }
    }
    delete pool;
    if (l.size() > 0) {
        std::list<Route>::iterator itFinal = this->routes.begin();
        int indexFrom = l.front().first.first;
        int indexTo = l.front().first.second;
        std::advance(itFinal, indexFrom);
        int diffCost = itFinal->GetTotalCost();
        *itFinal= l.front().second.first;
        diffCost -= itFinal->GetTotalCost();
        itFinal = this->routes.begin();
        std::advance(itFinal, indexTo);
        diffCost += itFinal->GetTotalCost();
        *itFinal = l.front().second.second;
        diffCost -= itFinal->GetTotalCost();
        this->CleanVoid();
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
                // remove nInsert consecutive customers from 'source'
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
int VRP::Opt21(bool force) {
    this->UpdateDistanceAverage();
    int diffCost = -1;
	std::list<Route>::iterator it = this->routes.begin();
    ResultList l;
    ThreadPool* pool = new ThreadPool((int)this->numberOfCores);
    for (int i = 0; it != this->routes.end(); std::advance(it, 1), ++i) {
        std::list<Route>::const_iterator jt = this->routes.cbegin();
        for (int j = 0; jt != this->routes.cend(); std::advance(jt, 1), ++j) {
            if (jt != it && it->GetDistanceFrom(*jt) < this->averageDistance) {
                pool->enqueue([force, it, jt, i, j, &l, this]() {
                    Route tFrom = *it;
                    Route tTo = *jt;
                    if (AddRemoveFromTo(tFrom, tTo, 2, 1)) {
                        std::lock_guard<std::mutex> lock(*this->mtx);
                        if ((tFrom.GetTotalCost() < l.front().second.first.GetTotalCost() &&
                                tTo.GetTotalCost() < l.front().second.second.GetTotalCost())
                                || l.size() == 0 || force)
                            l.push_front(std::make_pair(std::make_pair(i, j), std::make_pair(tFrom, tTo)));
                    }
                });
            }
        }
    }
    delete pool;
    if (l.size() > 0) {
        std::list<Route>::iterator itFinal = this->routes.begin();
        int indexFrom = l.front().first.first;
        int indexTo = l.front().first.second;
        std::advance(itFinal, indexFrom);
        int diffCost = itFinal->GetTotalCost();
        *itFinal = l.front().second.first;
        diffCost -= itFinal->GetTotalCost();
        itFinal = this->routes.begin();
        std::advance(itFinal, indexTo);
        diffCost += itFinal->GetTotalCost();
        *itFinal = l.front().second.second;
        diffCost -= itFinal->GetTotalCost();
        this->CleanVoid();
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
int VRP::Opt22(bool force) {
    this->UpdateDistanceAverage();
    int diffCost = -1;
	std::list<Route>::iterator it = this->routes.begin();
    ResultList l;
    ThreadPool* pool = new ThreadPool((int)this->numberOfCores);
    for (int i = 0; it != this->routes.end(); std::advance(it, 1), ++i) {
        std::list<Route>::const_iterator jt = this->routes.cbegin();
        for (int j = 0; jt != this->routes.cend(); std::advance(jt, 1), ++j) {
            if (jt != it && it->GetDistanceFrom(*jt) < this->averageDistance) {
                pool->enqueue([force, it, jt, i, j, &l, this]() {
                    Route tFrom = *it;
                    Route tTo = *jt;
                    if (AddRemoveFromTo(tFrom, tTo, 2, 2)) {
                        std::lock_guard<std::mutex> lock(*this->mtx);
                        if ((tFrom.GetTotalCost() < l.front().second.first.GetTotalCost() &&
                                tTo.GetTotalCost() < l.front().second.second.GetTotalCost())
                                || l.size() == 0 || force)
                            l.push_front(std::make_pair(std::make_pair(i, j), std::make_pair(tFrom, tTo)));
                    }
                });
            }
        }
    }
    delete pool;
    if (l.size() > 0) {
        std::list<Route>::iterator itFinal = this->routes.begin();
        int indexFrom = l.front().first.first;
        int indexTo = l.front().first.second;
        std::advance(itFinal, indexFrom);
        int diffCost = itFinal->GetTotalCost();
        *itFinal = l.front().second.first;
        diffCost -= itFinal->GetTotalCost();
        itFinal = this->routes.begin();
        std::advance(itFinal, indexTo);
        diffCost += itFinal->GetTotalCost();
        *itFinal = l.front().second.second;
        diffCost -= itFinal->GetTotalCost();
        this->CleanVoid();
        Utils::Instance().logger("opt22 improved: " + std::to_string(diffCost), Utils::VERBOSE);
    }else
        Utils::Instance().logger("opt22 no improvement", Utils::VERBOSE);
    return diffCost;
}

/** @brief ###Compute the total cost of routes.
 *
 * Compute the amount of costs for each route.
 * The cost is used to check improvement on paths.
 * @return The total cost
 */
int VRP::GetTotalCost() {
    this->totalCost = 0;
    for (auto e : this->routes) {
        this->totalCost += e.GetTotalCost();
    }
    return this->totalCost;
}

/** @brief ###Number of customers. */
int VRP::GetNumberOfCustomers() const { return numVertices; }

/** @brief ###Try to balance the routes.
 *
 * Find route with one or two customers and move it/them to another route:
 * execute an Opt10 to balance the route and get more occupancy.
 */
void VRP::RouteBalancer() {
    std::list<Route>::const_iterator it = this->routes.cbegin();
    ResultList l;
    for (int i = 0; it != this->routes.cend(); std::advance(it, 1), ++i) {
        if (it->size() == 3 || it->size() == 4) {
            std::list<Route>::const_iterator jt = this->routes.cbegin();
            for (int j = 0; jt != this->routes.cend(); std::advance(jt, 1), ++j) {
                if (jt != it && jt->size() > 3) {
                    Route tFrom = *it;
                    Route tTo = *jt;
                    if (Move1FromTo(tFrom, tTo, true)) {
                        if ((tFrom.GetTotalCost() < l.front().second.first.GetTotalCost() && tTo.GetTotalCost() < l.front().second.second.GetTotalCost()) || l.size() == 0)
                            l.push_front(std::make_pair(std::make_pair(i, j), std::make_pair(tFrom, tTo)));
                    }
                }
            }
        }
    }
    if (l.size() > 0) {
        std::list<Route>::iterator itFinal = this->routes.begin();
        int indexFrom = l.front().first.first;
        int indexTo = l.front().first.second;
        std::advance(itFinal, indexFrom);
        int diffCost = itFinal->GetTotalCost();
        *itFinal= l.front().second.first;
        diffCost -= itFinal->GetTotalCost();
        itFinal = this->routes.begin();
        std::advance(itFinal, indexTo);
        diffCost += itFinal->GetTotalCost();
        *itFinal = l.front().second.second;
        diffCost -= itFinal->GetTotalCost();
        this->CleanVoid();
        Utils::Instance().logger("Routes balancing: " + std::to_string(diffCost), Utils::VERBOSE);
    }else {
        Utils::Instance().logger("No routes balancing", Utils::VERBOSE);
    }
}

/** @brief ###Reorder the customers of route to delete cross over path.
 *
 * For every route swap two customer with distance lower than the average distance
 * of the route and save the best solution.
 * @return True if routes are improved
 */
bool VRP::Opt2() {
    bool ret = false;
    std::list<Route>::iterator it = this->routes.begin();
    int diffCost = 0;
    // for each route
    for (; it != this->routes.end(); ++it) {
        Route bestRoute = *it;
		ThreadPool* pool = new ThreadPool((int)this->numberOfCores);
		int bestCost = it->GetTotalCost();
		std::list<StepType>::iterator i = it->GetRoute()->begin();
		Customer depot = i->first;
		for (; i != it->GetRoute()->end(); i++) {
			if (i->first != depot) {
				std::list<StepType>::iterator k = i;
				for (++k; k != it->GetRoute()->end(); ++k) {
					if (k->first != depot) {
						pool->enqueue([i, k, it, this, &bestCost, &bestRoute, &ret]() {
							// swap customers
							Route tempRoute = this->Opt2Swap(*it, i->first, k->first);
							std::lock_guard<std::mutex> lock(*this->mtx);
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
		delete pool;
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
Route VRP::Opt2Swap(Route route, Customer i, Customer k) {
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
bool VRP::Opt3() {
    bool ret = false;
    std::list<Route>::iterator it = this->routes.begin();
    // Customers with short path (less than average)
    int diffCost = 0;
    // for each route
    for (; it != this->routes.end(); ++it) {
        Route bestRoute = *it;
		ThreadPool* pool = new ThreadPool((int)this->numberOfCores);
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
									pool->enqueue([i, k, l, m, it, &bestCost, &bestRoute, &ret, this]() {
										// swap customers
										Route tempRoute = this->Opt3Swap(*it, i->first, k->first, l->first, m->first);
										std::lock_guard<std::mutex> lock(*this->mtx);
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
		delete pool;
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
Route VRP::Opt3Swap(Route route, Customer i, Customer k, Customer l, Customer m) {
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

/* destructor */
VRP::~VRP() {
}
