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
        const float costTravel, const float alphaParam) : graph(g) {
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

/** @brief ###This function creates the routes. */
int VRP::InitSolutions() {
    // Create the best solution (i.e. E-n51-k5)
    /*Map dist = this->graph.sortV0();
    Customer depot = dist.cbegin()->second;
    std::set<Customer> custs;
    for (auto &e : dist)
        custs.emplace(e.second);
    this->CreateBest(custs, {5, 49, 10, 39, 33, 45, 15, 44, 37, 17, 12}, depot);
    this->CreateBest(custs, {47, 4, 42, 19, 40, 41, 13, 18}, depot);
    this->CreateBest(custs, {46, 32, 1, 22, 20, 35, 36, 3, 28, 31, 26, 8}, depot);
    this->CreateBest(custs, {6, 14, 25, 24, 43, 7, 23, 48, 27}, depot);
    this->CreateBest(custs, {11, 16, 2, 29, 21, 50, 34, 30, 9, 38}, depot);*/

    int best = 0, ret = 0;
    std::list<Route> b;
    for (int i = 0; i < this->numVertices -1; i++) {
        int r = this->init(i);
        int cost = this->GetTotalCost();
        if (best == 0 || cost < best) {
            b = this->routes;
            best = cost;
            ret = r;
        }
    }
    OptimalMove opt;
    opt.Opt2(b);
    opt.Opt3(b);
    this->routes = b;
    return ret;
}

/** @brief ###Create a personalize routes.
 *
 * From a list of indexes of customers creates the route.
 * Created for testing a visualizing the best routes for a known problem.
 * @param[in] custs Set of all customers
 * @param[in] best  List of ordered customer forming the route
 * @param[in] depot The depot
 */
void VRP::CreateBest(std::set<Customer> custs, std::list<int> best, Customer depot) {
    Route v(this->capacity, this->workTime, this->graph, this->costTravel, this->alphaParam);
    auto it = custs.cbegin();
    auto from = custs.cbegin();
    std::advance(it, best.front());
    v.Travel(depot, *it);
    best.erase(best.begin());
    for (auto e : best) {
        from = it;
        it = custs.cbegin();
        std::advance(it, e);
        v.Travel(*from, *it);
    }
    v.CloseTravel(*it);
    this->routes.push_back(v);
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
int VRP::init(int start) {
    this->routes.clear();
    Customer depot, last;
    // the iterator for the sorted customer
    Map::const_iterator it;
    /* distances of customers sorted from depot */
    Map dist = this->graph.sortV0();
    /* get the depot and remove it from the map */
    depot = dist.cbegin()->second;
    dist.erase(dist.cbegin());
    it = dist.cbegin();
    std::advance(it, start);
    // for each customer try to insert it in a route, otherwise create a new route
    while(!dist.empty()) {
        bool stop = false;
        // create an empty route
        Route v(this->capacity, this->workTime, this->graph, this->costTravel, this->alphaParam);
        Customer from, to;
        // start the route with a depot
		if (!v.Travel(depot, it->second))
            throw std::runtime_error("Customer unreachable!");
		// if one customer is left add it to route
        if (dist.size() == 1) {
            v.CloseTravel(it->second);
            stop = true;
            dist.clear();
        }
        // while the route is not empty
        while (!stop) {
            from = it->second;
            Map::const_iterator fallback = it;
            ++it;
            if (it == dist.cend())
                it = dist.cbegin();
            to = it->second;
            // add path from 'from' to 'to'
            if(from != to && !v.Travel(from, to)) {
                stop = true;
                // if cannot add the customer try to close the route
                if (!v.CloseTravel(from, depot))
                    v.CloseTravel(from);
            // if the customer is added and remain one customer add it to route
            }else if (dist.size() == 1) {
                if (!v.CloseTravel(to, depot))
                    v.CloseTravel(from);
                stop = true;
            }
            dist.erase(fallback);
        }
        this->routes.push_back(v);
        it = dist.cbegin();
    }
    OptimalMove opt;
    opt.RouteBalancer(this->routes);
    int j = 0;
    if (this->routes.size() < (unsigned)this->vehicles)
        j = -1;
    else if (this->routes.size() == (unsigned)this->vehicles)
        j = 0;
    else
        j = 1;
    return j;
}

/** @brief ###This function creates the initial solution.
 *
 *  This function creates the initial solution of the algorithm.
 *  Starting from the depot it creates the routes running an iterated local search
 *  for each customer. Whenever the insertion of a customer would lead to a violation
 *  of the capacity of the working time a new route is created.
 *  @return Error or Warning code.
 */
int VRP::InitSolutionsNeigh() {
    // distances of customers sorted from depot
    Map dist = this->graph.sortV0();
    // get the depot and remove it from the map
    Customer depot = dist.cbegin()->second;
    Customer from, to;
    dist.erase(dist.cbegin());
    std::set<Customer> done;
    done.emplace(depot);
    // the customer closer to the depot
    auto it = dist.cbegin();
    while((int)done.size() < this->numVertices) {
        bool stop = false;
        Route v(this->capacity, this->workTime, this->graph, this->costTravel, this->alphaParam);
        while(done.find(it->second) != done.end()) {
            ++it;
        }
        to = it->second;
        if (!v.Travel(depot, to))
            throw std::runtime_error("Customer unreachable!!");
        while (!stop) {
            from = to;
            // find the closer customer to it->second
            auto neigh = this->graph.GetNeighborhood(from);
            auto neighit = neigh.begin();
            if ((int)done.size() == this->numVertices - 1) {
                break;
            }
            // take the next customer, the nearest
            while(done.find(neighit->second) != done.end()) {
                ++neighit;
            }
            to = neighit->second;
            // add path from 'from' to 'to'
            if(!v.Travel(from, to)) {
                stop = true;
            }else {
                done.emplace(from);
            }
        }
        // if cannot add the customer try to close the route
        if (!v.CloseTravel(from, depot)) {
            v.CloseTravel(from);
        }else
            done.emplace(from);
        this->routes.emplace_back(v);
    }
    Utils::Instance().logger("Routes created", Utils::VERBOSE);
    int j = 0;
    if (this->routes.size() < (unsigned)this->vehicles)
        j = -1;
    else if (this->routes.size() == (unsigned)this->vehicles)
        j = 0;
    else
        j = 1;
    return j;
}

/* @brief ###Run the tabu search function.
 *
 * Run the basic function of this algorithm.
 * @param[in] times Iteration condition.
 */
void VRP::RunTabuSearch(int times) {
    TabuSearch s(this->graph, this->numVertices);
    s.Tabu(this->routes, times);
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
    while (i < times && duration <= 25) {
        Utils::Instance().logger("Round " + std::to_string(i+1) + " of " + std::to_string(times), Utils::VERBOSE);
        if (optxx) {
            result = opt.Opt10(this->routes, flag);
            if (flag) flag = false;
            //if (result < 0) result = opt.Opt01(this->routes, flag);
            if (result <= 0) result = opt.Opt11(this->routes, flag);
            if (result <= 0) result = opt.Opt12(this->routes, flag);
            //if (result <= 0 || flag) result = opt.Opt21(this->routes, flag);
            if (result <= 0) result = opt.Opt22(this->routes, flag);
        }
        // if no more improvements run only 2-opt and 3-opt
        if (result <= 0)
            optxx = false;
        if (opt.Opt2(this->routes))
            optxx = true;
        if (opt.Opt3(this->routes))
            optxx = true;
        if (result <= 0 && !optxx)
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
bool VRP::UpdateBest() {
    int tCost = 0;
    for (auto e : this->bestRoutes)
        tCost += e.GetTotalCost();
    this->GetTotalCost();
    if (this->totalCost <= tCost || tCost == 0) {
        this->bestRoutes.clear();
        std::copy(this->routes.cbegin(), this->routes.cend(), std::back_inserter(this->bestRoutes));
        return true;
    }
    return false;
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
