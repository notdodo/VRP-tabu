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
 */
VRP::VRP(const Graph &g, const int n, const int v, const int c, const float t, const bool flagTime,
        const float costTravel, const float alphaParam) {
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
    TabuList tabulist();
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
    std::random_device rd;
    std::mt19937 g(rd());
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
    //start = rand() % (this->numVertices-1);
    std::uniform_int_distribution<int> dis(0, (this->numVertices - 2));
    start = dis(g);
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

void VRP::RunTabuSearch() {
    TabuSearch s(this->graph, this->numVertices);
    s.Tabu(this->routes);
}

/** @brief ###Run optimal functions.
 *
 * Runs all the optimal functions to achieve a better optimization of the routes.
 * When the routine do not improve the routes, stops and try to balance the routes.
 * @param[in] times Number of iteration
 * @param[in] flag  Force or not the moves
 * @return          If the routine made some improvements.
 */
bool VRP::RunOpts(int times, bool flag) {
    OptimalMove opt;
    int i = 0, duration = 0, result;
    bool optxx = true;
    // start time
    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
    // do at least 4 iteration in less than 30 minutes
    while (i < times && !(i > 3 && duration >= 25)) {
        Utils::Instance().logger("Round " + std::to_string(i+1) + " of " + std::to_string(times), Utils::VERBOSE);
        if (optxx) {
            result = opt.Opt10(this->routes, flag);
            /*if (result < 0)
                result = opt.Opt01(this->routes, flag);*/
            if (result <= 0 || flag)
                result = opt.Opt11(this->routes, flag);
            if (result <= 0 || flag)
                result = opt.Opt12(this->routes, flag);
            /*if (result < 0)
                result = this->opt->Opt21(this->routes, flag);*/
            if (result <= 0 || flag)
                result = opt.Opt22(this->routes, flag);
        }
        // if no more improvements run only 2-opt and 3-opt
        if (result < 0)
            optxx = false;
        if (opt.Opt2(this->routes))
            optxx = true;
        if (opt.Opt3(this->routes))
            optxx = true;
        if (result < 0 && !optxx)
            break;
        i++;
        // partial time
        std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::minutes>(t2 - t1).count();
    }
    opt.RouteBalancer(this->routes);
    // if no improvements return false;
    return !(result <= 0 && !optxx);
}

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


/** @brief ###Check the integrity of the system
 *
 * Each customer have to be visited only once, this function checks if
 * this constraint is valid in the system. Used for debugging.
 * @return The status of the constraint
 */
bool VRP::CheckIntegrity() {
    std::list<Customer> checked;
    int custs = 1;
    for(auto ir = this->routes.begin(); ir != this->routes.end(); ++ir) {
        RouteList::const_iterator ic = ir->GetRoute()->cbegin();
        Customer depot = ic->first;
        ++ic;
        for(; ic->first != ir->GetRoute()->back().first; ++ic) {
            custs++;
            std::list<Customer>::const_iterator findIter = std::find(checked.cbegin(), checked.cend(), ic->first);
            if (findIter == checked.cend())
                checked.push_back(ic->first);
            else
                return false;
        }
    }
    if (this->numVertices != custs)
        return false;
    return true;
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

/* destructor */
VRP::~VRP() {
}
