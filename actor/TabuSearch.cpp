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

#include "TabuSearch.h"

/** @brief ###Primary function, search for better solution and updates the tabu list
 *
 * This function run an iterated local search for a customer and try to insert neighbors
 * customers in the route. If the move isn't legal, updates the tabu list.
 * If the move is tabu but improve then update the route.
 * If no improvement are made choose the best of the worst.
 */
void TabuSearch::Tabu(Routes &routes) {
    /*int N = (int)(this->numCustomers / routes.size());
    int iterations = 0, stop = 0;
    float bestFitness = 0, worstFitness = 0;
    Routes bestSolution(routes.begin(), routes.end());
    Routes workingRoutes(routes.begin(), routes.end());
    Routes worstSolution(routes.begin(), routes.end());
    while (iterations < 30) {
        iterations++;
        std::pair<Routes, float> result = this->IteratedLocalSearch(workingRoutes, N);
        Routes r = std::get<0>(result);
        std::cout << std::get<1>(result) << std::endl;
        if (bestFitness == 0 || std::get<1>(result) < bestFitness) {
            bestFitness = std::get<1>(result);
            bestSolution = r;
        }
        if (std::get<1>(result) > worstFitness) {
            worstFitness = std::get<1>(result);
            worstSolution = r;
        }
        if (this->Evaluate(workingRoutes) == this->Evaluate(r))
            stop++;
        workingRoutes = r;
        if (stop >= 2) {
            workingRoutes = worstSolution;
            stop = 0;
        }
        if (stop >= 2)
            break;
        this->tabulist.Clean();
    }
    routes = bestSolution;*/
    // number of neighbors to consider
    int N = (int)(this->numCustomers / routes.size());
    // copy all routes in a local list
    std::list<Route> s(routes.begin(), routes.end());
    // best solution
    std::list<Route> sbest(routes.begin(), routes.end());
    std::set<std::pair<float, std::list<Route>>> nonBest;
    float bestFitness = 0;
    int iterations = 0;
    while (iterations < (int)(50 / routes.size())) {
        iterations++;
        // create a local working copy of solutions
        std::list<Route>::iterator its = s.begin();
        float fitnessBestCandidate = 0;
        std::list<Route> bestCandidate;
        std::vector<Move> bestMoves;
        std::list<Route> temp(s.begin(), s.end());
        // for each route find the local best solution
        for (int r = 0; its != s.end(); ++its, r++) {
            RouteList::iterator itc = its->GetRoute()->begin();
            Customer depot = its->GetRoute()->front().first;
            ++itc;
            // for each customer in the route
            for (int cc = 0; itc->first != depot; ++itc, ++cc) {
                std::vector<Move> candidateMoves;
                float penalization = 0;
                // get the neighborhood of the customer
                auto neigh = this->graph.GetNeighborhood(itc->first);
                // for N customer try to add them to the route
                auto in = neigh.begin();
                for (int i = 0; in != neigh.end() && i < N; ++in, i++) {
                    std::list<Route>::iterator itDest = temp.begin();
                    std::advance(itDest, r);
                    if (in->second != depot && !itDest->FindCustomer(in->second)) {
                        int routeIndex = this->FindRouteFromCustomer(in->second, temp);
                        std::list<Route>::iterator itFrom = temp.begin();
                        std::advance(itFrom, routeIndex);
                        Move m = {{in->second, r}, routeIndex};
                        candidateMoves.push_back(m);
                        float itDestFitness = itDest->Evaluate();
                        Route fallbackFrom = *itFrom;
                        Route fallbackDest = *itDest;
                        // move in->second to r from routeIndex
                        if (itFrom->RemoveCustomer(in->second) && itDest->AddElem(in->second)) {
                            // if is tabu or not improve
                            if (this->tabulist.Find(m) && (itDest->Evaluate() > 0 && itDest->Evaluate() < itDestFitness)) {
                                //restore
                                *itFrom = fallbackFrom;
                                *itDest = fallbackDest;
                            }else {
                                penalization += this->tabulist.Check(m);
                            }
                        }else {
                            *itFrom = fallbackFrom;
                            *itDest = fallbackDest;
                        }
                        // remove empty routes
                        temp.remove_if([](Route r){ return r.size() <= 2; });
                    }
                }
                float asses = this->Evaluate(temp);
                float p = this->lambda * asses * std::sqrt(this->numCustomers * temp.size()) * penalization;
                if ((asses + p) < fitnessBestCandidate || fitnessBestCandidate == 0) {
                    fitnessBestCandidate = asses;
                    bestCandidate = temp;
                    if (!bestMoves.empty()) bestMoves.clear();
                    bestMoves = candidateMoves;
                }else if (temp != routes) {
                    nonBest.insert({asses, temp});
                }
                temp = s;
            }
        }
        // update the solution (local best)
        s = bestCandidate;
        auto last = std::unique(bestMoves.begin(), bestMoves.end());
        bestMoves.erase(last, bestMoves.end());
        auto nb = nonBest.begin();
        if (fitnessBestCandidate < bestFitness || bestFitness == 0) {
            sbest = bestCandidate;
            bestFitness = fitnessBestCandidate;
        }else if (fitnessBestCandidate == bestFitness) {
            std::advance(nb, nonBest.size() - 1);
            s = nb->second;
            nonBest.erase(nb);
        }
        // add moves to tabulist
        std::for_each(bestMoves.begin(), bestMoves.end(), [this](Move &m){ this->tabulist.AddTabu(m, this->numCustomers); });
        this->tabulist.Clean();
        nb = nonBest.begin();
        if (nonBest.size() > 50) {
            std::advance(nb, 50);
            nonBest.erase(nonBest.begin(), nb);
        }
    }
    if (sbest == routes && !nonBest.empty()) {
        auto nb = nonBest.begin();
        //std::advance(nb, nonBest.size()-1);
        routes = nb->second;
        nonBest.erase(nb);
    }else
        routes = sbest;
}

std::pair<Routes, float> TabuSearch::IteratedLocalSearch(Routes &routes, int N) {
    Routes temp(routes.begin(), routes.end());
    Routes::iterator itroutes = temp.begin();
    float penalization = 0;
    std::vector<Move> candidateMoves;
    // for each route find the local best solution
    for (int r = 0; itroutes != temp.end(); ++itroutes, r++) {
        RouteList::iterator itc = itroutes->GetRoute()->begin();
        // get the depot
        Customer depot = itroutes->GetRoute()->front().first;
        ++itc;
        // for each customer in the route (no depot)
        for (int cc = 0; itc->first != depot && itc != itroutes->GetRoute()->end(); ++itc, cc++) {
            // get the neighborhood of the customer
            auto neigh = this->graph.GetNeighborhood(itc->first);
            // for N customer try to add them to the route
            auto in = neigh.begin();
            for (int i = 0; in != neigh.end() && i < N; ++in, i++) {
                Routes::iterator itDest = temp.begin();
                std::advance(itDest, r);
                if (in->second != depot && !itDest->FindCustomer(in->second)) {
                    //  find the route which the customer belongs
                    int routeIndex = this->FindRouteFromCustomer(in->second, temp);
                    Routes::iterator itFrom = temp.begin();
                    std::advance(itFrom, routeIndex);
                    Move m = {{in->second, r}, routeIndex};
                    float itDestFitness = itDest->Evaluate();
                    Route fallbackFrom = *itFrom;
                    Route fallbackDest = *itDest;
                    // move in->second to r from routeIndex
                    if (itFrom->RemoveCustomer(in->second) && itDest->AddElem(in->second)) {
                        // if is not tabu or improve
                        if (!this->tabulist.Find(m) || (itDest->Evaluate() > 0 && itDest->Evaluate() < itDestFitness)) {
                            penalization += this->tabulist.Check(m);
                            candidateMoves.push_back(m);
                        }else {
                            //restore
                            *itFrom = fallbackFrom;
                            *itDest = fallbackDest;
                        }
                    }else {
                        *itFrom = fallbackFrom;
                        *itDest = fallbackDest;
                    }
                    // remove empty routes
                    if (itFrom->size() <= 2) {
                        temp.erase(itFrom);
                        itroutes = temp.begin();
                        std::advance(itroutes, r);
                        itc = itroutes->GetRoute()->begin();
                        std::advance(itc, cc + 1);
                    }
                }
            }
        }
    }
    float p = this->lambda * this->Evaluate(temp) * std::sqrt(this->numCustomers * temp.size()) * penalization;
    std::for_each(candidateMoves.begin(), candidateMoves.end(), [this](Move &m){ this->tabulist.AddTabu(m, this->numCustomers); });
    return {temp, this->Evaluate(temp) + p};
}

/** @brief ###Find the route to which a customer belongs
 *
 * This function finds the route to which a customer belongs and return the
 * index of the route.
 * @param[in] c The customer to find
 * @param[in] l The list where find the customer
 * @return The index of the route
 */
int TabuSearch::FindRouteFromCustomer(Customer c, Routes l) {
    std::list<Route>::iterator it = l.begin();
    int ret = -1;
    bool flag = false;
    for(; it != l.end() && !flag; ++it, ++ret) {
        RouteList::const_iterator l = it->GetRoute()->cbegin();
        for(; l != it->GetRoute()->cend() && !flag; ++l) {
            if (l->first == c)
                flag = true;
        }
    }
    return ret;
}

/** @brief ###Evaluate the assessment of the solution
 *
 * This function assess the quality of the solution
 */
float TabuSearch::Evaluate(Routes l) {
    std::list<Route>::iterator i = l.begin();
    float tot = 0;
    for (; i != l.end(); ++i) {
        tot += i->Evaluate();
    }
    return (float)(tot);
}