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
void TabuSearch::Tabu(Routes &routes, int times) {
    unsigned MAX = 50;
    // 0.80 521
    float TABUTIME = this->numCustomers * 0.80;
    // number of neighbors to consider
    int N = (int)(this->numCustomers / routes.size());
    // copy all routes in a local list
    std::list<Route> s(routes.begin(), routes.end());
    // best solution
    std::list<Route> sbest(routes.begin(), routes.end());
    auto nonBestComp = [](const std::pair<float, Routes> &l, const std::pair<float, Routes> &r)->bool { return l.first < r.first; };
    std::set<std::pair<float, Routes>, decltype(nonBestComp)> nonBest(nonBestComp);
    float bestFitness = 0;
    int iterations = 0;
    while (iterations < times) {
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
                std::multimap<int, Customer> neigh = this->graph.GetNeighborhood(itc->first);
                // for N customer try to add them to the route
                auto in = neigh.begin();
                for (int i = 0; in != neigh.end() && i < N; ++in, i++) {
                    std::list<Route>::iterator itDest = temp.begin();
                    std::advance(itDest, r);
                    if (itDest != temp.end() && !itDest->FindCustomer(in->second)) {
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
                            if (this->tabulist.Find(m) && itDest->Evaluate() > itDestFitness) {
                                //restore
                                *itFrom = fallbackFrom;
                                *itDest = fallbackDest;
                            }else {
                                penalization += this->tabulist.Check(m);
                            }
                            // remove empty routes
                            temp.remove_if([](Route r){ return r.size() <= 2; });
                        }else {
                            *itFrom = fallbackFrom;
                            *itDest = fallbackDest;
                        }
                    }
                }
                float asses = this->Evaluate(temp);
                float p = this->lambda * asses * std::sqrt(this->numCustomers * temp.size()) * penalization;
                if ((asses + p) < fitnessBestCandidate || fitnessBestCandidate == 0) {
                    fitnessBestCandidate = asses;
                    bestCandidate = temp;
                    bestMoves = candidateMoves;
                }else if (temp != routes) {
                    nonBest.insert({asses, temp});
                }
                temp = s;
            }
        }
        // update the solution (local best)
        s = bestCandidate;
        bestMoves.erase(std::unique(bestMoves.begin(), bestMoves.end()), bestMoves.end());
        if (fitnessBestCandidate < bestFitness || bestFitness == 0) {
            sbest = bestCandidate;
            bestFitness = fitnessBestCandidate;
        }else if (fitnessBestCandidate == bestFitness && !nonBest.empty()) {
            auto nb = nonBest.begin();
            std::advance(nb, nonBest.size() - 1);
            s = nb->second;
            nonBest.erase(nb);
        }
        // add moves to tabulist
        std::for_each(bestMoves.begin(), bestMoves.end(), [this, TABUTIME](Move &m){ this->tabulist.AddTabu(m, TABUTIME); });
        this->tabulist.Clean();
        if (nonBest.size() > MAX) {
            auto nb = nonBest.begin();
            std::advance(nb, MAX);
            nonBest.erase(nonBest.begin(), nb);
        }
    }
    if (this->Evaluate(sbest) == this->Evaluate(routes) && !nonBest.empty()) {
        auto nb = nonBest.begin();
        std::advance(nb, nonBest.size() - 1);
        routes = nb->second;
        nonBest.erase(nb);
        this->tabulist.IncrementSize();
    }else {
        routes = sbest;
        this->tabulist.DecrementSize();
    }
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
