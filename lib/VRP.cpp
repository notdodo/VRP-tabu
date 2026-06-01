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
#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {
/** @brief Active route fragment used by the Clarke-Wright savings initializer. */
struct SavingsRoute {
    std::vector<Customer> customers;
    bool active;
};

/** @brief Potential customer-pair merge ordered by distance saving. */
struct Saving {
    Customer first;
    Customer second;
    double value;
};

/** @brief Customer annotated with its polar angle around the depot. */
struct SweepCustomer {
    Customer customer;
    double angle;
};

/** @brief Feasible contiguous sweep segment and its standalone route cost. */
struct SweepSegment {
    std::vector<Customer> route;
    int cost = 0;
    bool feasible = false;
};

/** @brief Complete sweep partition candidate with total cost. */
struct SweepPlan {
    std::vector<std::vector<Customer>> routes;
    int cost = 0;
};

/** @brief Check whether a customer is at either end of a savings route. */
bool IsRouteEnd(const SavingsRoute& route, const Customer& customer) {
    return !route.customers.empty() && (route.customers.front() == customer || route.customers.back() == customer);
}

/** @brief Find the active savings route that currently contains a customer. */
int FindSavingsRouteIndex(const std::vector<SavingsRoute>& routes, const Customer& customer) {
    for (std::size_t i = 0; i < routes.size(); ++i) {
        if (routes[i].active && std::ranges::find(routes[i].customers, customer) != routes[i].customers.end()) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

/** @brief Build a depot-to-depot customer list from an internal customer order. */
std::list<Customer> BuildRouteCustomers(const Customer& depot, const std::vector<Customer>& customers) {
    std::list<Customer> route;
    route.push_back(depot);
    std::ranges::copy(customers, std::back_inserter(route));
    route.push_back(depot);
    return route;
}

/** @brief Merge two savings route fragments so the selected customers become adjacent. */
std::vector<Customer> MergeSavingsRoutes(const SavingsRoute& firstRoute, const Customer& firstCustomer,
                                         const SavingsRoute& secondRoute, const Customer& secondCustomer) {
    std::vector<Customer> first = firstRoute.customers;
    std::vector<Customer> second = secondRoute.customers;
    if (first.front() == firstCustomer) {
        std::ranges::reverse(first);
    }
    if (second.back() == secondCustomer) {
        std::ranges::reverse(second);
    }
    first.insert(first.end(), second.cbegin(), second.cend());
    return first;
}

/** @brief Compare route count against the target vehicle count. */
int CompareRouteCount(std::size_t routeCount, int vehicles) {
    if (std::cmp_less(routeCount, vehicles)) {
        return -1;
    }
    if (std::cmp_equal(routeCount, vehicles)) {
        return 0;
    }
    return 1;
}

/** @brief Compute the travel cost of a complete customer sequence. */
int RouteOrderCost(const Graph& graph, const std::vector<Customer>& route) {
    int cost = 0;
    for (std::size_t i = 0; i + 1 < route.size(); ++i) {
        cost += graph.GetCosts(route[i], route[i + 1]).second;
    }
    return cost;
}

/** @brief Improve one fixed-customer route with deterministic 2-opt. */
std::vector<Customer> ImproveRouteOrder2Opt(const Graph& graph, std::vector<Customer> route) {
    if (route.size() <= 4) {
        return route;
    }
    int bestCost = RouteOrderCost(graph, route);
    bool improved = true;
    while (improved) {
        improved = false;
        for (std::size_t i = 1; i + 2 < route.size(); ++i) {
            for (std::size_t k = i + 1; k + 1 < route.size(); ++k) {
                std::vector<Customer> candidate = route;
                std::reverse(candidate.begin() + static_cast<std::ptrdiff_t>(i),
                             candidate.begin() + static_cast<std::ptrdiff_t>(k + 1));
                const int candidateCost = RouteOrderCost(graph, candidate);
                if (candidateCost < bestCost) {
                    route = std::move(candidate);
                    bestCost = candidateCost;
                    improved = true;
                }
            }
        }
    }
    return route;
}

/** @brief Sum the active costs of all routes. */
int RoutesCost(const Routes& routes) {
    int cost = 0;
    for (const Route& route : routes) {
        cost += route.GetTotalCost();
    }
    return cost;
}

/** @brief Build one Clarke-Wright savings candidate for a savings weight.
 *
 * The classic savings score is c(depot,i) + c(depot,j) - lambda*c(i,j).
 * Trying several lambda values gives a cheap deterministic multi-start
 * initializer: low values favor radial merges, high values favor compact
 * customer-to-customer links.
 */
Routes BuildSavingsRoutes(Graph& graph, const std::vector<Customer>& customers, const Customer& depot, int capacity,
                          float workTime, float costTravel, float alphaParam, double lambda) {
    std::vector<SavingsRoute> savingsRoutes;
    savingsRoutes.reserve(customers.size());
    for (const Customer& customer : customers) {
        savingsRoutes.push_back(SavingsRoute{
            .customers = {customer},
            .active = true,
        });
    }

    std::vector<Saving> savings;
    savings.reserve(customers.size() * customers.size());
    for (std::size_t i = 0; i < customers.size(); ++i) {
        for (std::size_t j = i + 1; j < customers.size(); ++j) {
            savings.push_back(Saving{
                .first = customers[i],
                .second = customers[j],
                .value = static_cast<double>(graph.GetCost(depot, customers[i]) + graph.GetCost(depot, customers[j])) -
                         (lambda * static_cast<double>(graph.GetCost(customers[i], customers[j]))),
            });
        }
    }
    std::ranges::sort(savings, [](const Saving& lhs, const Saving& rhs) {
        if (lhs.value != rhs.value) {
            return lhs.value > rhs.value;
        }
        if (lhs.first.name != rhs.first.name) {
            return lhs.first.name < rhs.first.name;
        }
        return lhs.second.name < rhs.second.name;
    });
    for (const Saving& saving : savings) {
        const int firstIndex = FindSavingsRouteIndex(savingsRoutes, saving.first);
        const int secondIndex = FindSavingsRouteIndex(savingsRoutes, saving.second);
        if (firstIndex < 0 || secondIndex < 0 || firstIndex == secondIndex) {
            continue;
        }
        SavingsRoute& firstRoute = savingsRoutes[static_cast<std::size_t>(firstIndex)];
        SavingsRoute& secondRoute = savingsRoutes[static_cast<std::size_t>(secondIndex)];
        if (!IsRouteEnd(firstRoute, saving.first) || !IsRouteEnd(secondRoute, saving.second)) {
            continue;
        }
        std::vector<Customer> mergedCustomers =
            MergeSavingsRoutes(firstRoute, saving.first, secondRoute, saving.second);
        Route candidate(capacity, workTime, graph, costTravel, alphaParam);
        if (candidate.RebuildRoute(BuildRouteCustomers(depot, mergedCustomers))) {
            firstRoute.customers = mergedCustomers;
            secondRoute.active = false;
        }
    }

    Routes routes;
    for (const SavingsRoute& route : savingsRoutes) {
        if (route.active) {
            Route candidate(capacity, workTime, graph, costTravel, alphaParam);
            if (!candidate.RebuildRoute(BuildRouteCustomers(depot, route.customers))) {
                throw std::runtime_error("Savings route is infeasible");
            }
            routes.push_back(candidate);
        }
    }
    return routes;
}

/** @brief Build a sweep-partition initial solution.
 *
 * Customers are sorted by polar angle around the depot. For every rotation and
 * direction, dynamic programming chooses a contiguous partition into the
 * minimum capacity-feasible route count. The selected route set is then
 * internally improved with 2-opt before competing with the current best plan.
 */
std::optional<Routes> BuildSweepRoutes(Graph& graph, int capacity, float workTime, float costTravel, float alphaParam) {
    if (capacity <= 0) {
        return std::nullopt;
    }
    Map dist = graph.sortV0();
    Customer depot = dist.cbegin()->second;
    dist.erase(dist.cbegin());
    std::vector<SweepCustomer> customers;
    customers.reserve(dist.size());
    int totalDemand = 0;
    for (const auto& entry : dist) {
        const Customer& customer = entry.second;
        totalDemand += customer.request;
        customers.push_back(SweepCustomer{
            .customer = customer,
            .angle = std::atan2(static_cast<double>(customer.y - depot.y), static_cast<double>(customer.x - depot.x)),
        });
    }
    if (customers.empty()) {
        return std::nullopt;
    }
    // Capacity gives a hard lower bound on the number of routes.
    const int targetRouteCount = std::max(1, (totalDemand + capacity - 1) / capacity);
    if (std::cmp_greater(targetRouteCount, customers.size())) {
        return std::nullopt;
    }
    std::ranges::sort(customers, [](const SweepCustomer& left, const SweepCustomer& right) {
        if (left.angle != right.angle) {
            return left.angle < right.angle;
        }
        return left.customer.name < right.customer.name;
    });
    std::optional<SweepPlan> bestPlan;
    const std::size_t customerCount = customers.size();
    // Try every circular sweep start in both directions; the first customer in
    // an angular ordering can materially change the partition.
    for (const int direction : {1, -1}) {
        for (std::size_t start = 0; start < customerCount; ++start) {
            std::vector<Customer> ordered;
            ordered.reserve(customerCount);
            for (std::size_t offset = 0; offset < customerCount; ++offset) {
                // Walk the circular order without physically rotating the vector.
                const std::size_t index =
                    direction > 0 ? (start + offset) % customerCount : (start + customerCount - offset) % customerCount;
                ordered.push_back(customers[index].customer);
            }
            std::vector<int> demandPrefix(customerCount + 1, 0);
            for (std::size_t i = 0; i < customerCount; ++i) {
                demandPrefix[i + 1] = demandPrefix[i] + ordered[i].request;
            }
            std::vector<std::vector<SweepSegment>> segments(customerCount,
                                                            std::vector<SweepSegment>(customerCount + 1));
            // segments[i][j] is the direct route serving ordered[i, j).
            for (std::size_t i = 0; i < customerCount; ++i) {
                for (std::size_t j = i + 1; j <= customerCount; ++j) {
                    if (demandPrefix[j] - demandPrefix[i] > capacity) {
                        break;
                    }
                    std::vector<Customer> route;
                    route.reserve(j - i + 2);
                    route.push_back(depot);
                    route.insert(route.end(), ordered.cbegin() + static_cast<std::ptrdiff_t>(i),
                                 ordered.cbegin() + static_cast<std::ptrdiff_t>(j));
                    route.push_back(depot);
                    route = ImproveRouteOrder2Opt(graph, std::move(route));
                    const int routeCost = RouteOrderCost(graph, route);
                    segments[i][j] = SweepSegment{
                        .route = std::move(route),
                        .cost = routeCost,
                        .feasible = true,
                    };
                }
            }
            constexpr int infinity = std::numeric_limits<int>::max() / 4;
            std::vector<std::vector<int>> dp(static_cast<std::size_t>(targetRouteCount + 1),
                                             std::vector<int>(customerCount + 1, infinity));
            std::vector<std::vector<int>> previous(static_cast<std::size_t>(targetRouteCount + 1),
                                                   std::vector<int>(customerCount + 1, -1));
            // dp[r][end] is the cheapest way to cover ordered[0, end) with r routes.
            dp[0][0] = 0;
            for (int routeCount = 1; routeCount <= targetRouteCount; ++routeCount) {
                for (std::size_t end = 1; end <= customerCount; ++end) {
                    for (std::size_t begin = 0; begin < end; ++begin) {
                        if (!segments[begin][end].feasible ||
                            dp[static_cast<std::size_t>(routeCount - 1)][begin] == infinity) {
                            continue;
                        }
                        const int candidateCost =
                            dp[static_cast<std::size_t>(routeCount - 1)][begin] + segments[begin][end].cost;
                        if (candidateCost < dp[static_cast<std::size_t>(routeCount)][end]) {
                            dp[static_cast<std::size_t>(routeCount)][end] = candidateCost;
                            // Store the split point so the route partition can be reconstructed later.
                            previous[static_cast<std::size_t>(routeCount)][end] = static_cast<int>(begin);
                        }
                    }
                }
            }
            const int planCost = dp[static_cast<std::size_t>(targetRouteCount)][customerCount];
            if (planCost == infinity) {
                continue;
            }
            SweepPlan plan;
            std::size_t end = customerCount;
            // Follow split points backwards from the full prefix to recover each chosen segment.
            for (int routeCount = targetRouteCount; routeCount >= 1; --routeCount) {
                const int begin = previous[static_cast<std::size_t>(routeCount)][end];
                if (begin < 0) {
                    plan.routes.clear();
                    break;
                }
                plan.routes.push_back(segments[static_cast<std::size_t>(begin)][end].route);
                end = static_cast<std::size_t>(begin);
            }
            if (std::cmp_equal(plan.routes.size(), targetRouteCount)) {
                // Reconstruction walks backwards, so restore the original sweep order.
                std::ranges::reverse(plan.routes);
                int improvedCost = 0;
                for (std::vector<Customer>& route : plan.routes) {
                    route = ImproveRouteOrder2Opt(graph, std::move(route));
                    improvedCost += RouteOrderCost(graph, route);
                }
                plan.cost = improvedCost;
                if (!bestPlan.has_value() || plan.cost < bestPlan->cost) {
                    bestPlan = std::move(plan);
                }
            }
        }
    }
    if (!bestPlan.has_value()) {
        return std::nullopt;
    }
    Routes routes;
    for (const std::vector<Customer>& routeCustomers : bestPlan->routes) {
        Route route(capacity, workTime, graph, costTravel, alphaParam);
        std::list<Customer> routeList(routeCustomers.cbegin(), routeCustomers.cend());
        if (!route.RebuildRoute(routeList)) {
            return std::nullopt;
        }
        routes.push_back(route);
    }
    return routes;
}
} // namespace

/** @brief Construct a VRP model from graph data and solver parameters.
 *
 * @param[in] g The graph generate from the input.
 * @param[in] n The number of Customers.
 * @param[in] v The number of vehicles.
 * @param[in] c The capacity of each vehicle.
 * @param[in] t The work time of each driver.
 * @param[in] flagTime If the service time is a constraint.
 * @param[in] costTravel Cost parameter for each travel.
 * @param[in] alphaParam Alpha parameter for route evaluation.
 */
VRP::VRP(Graph&& g, const int n, const int v, const int c, const float t, const bool flagTime, const float costTravel,
         const float alphaParam)
    : graph(std::move(g)) {
    this->numVertices = n;
    this->vehicles = v;
    this->capacity = c;
    // if no service time is needed
    if (flagTime)
        this->workTime = t;
    else
        this->workTime = std::numeric_limits<float>::max();
    this->costTravel = costTravel;
    this->totalCost = 0;
    this->alphaParam = alphaParam;
    this->averageDistance = 0;
}

/** @brief Create the initial route set and locally improve it. */
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
    Routes b;
    for (int i = 0; i < this->numVertices - 1; i++) {
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

/** @brief Create a route from a fixed customer-index sequence.
 *
 * From a list of indexes of customers creates the route.
 * Created for testing a visualizing the best routes for a known problem.
 * @param[in] custs Set of all customers
 * @param[in] best  Ordered customer indexes forming the route
 * @param[in] depot The depot
 */
void VRP::CreateBest(const std::set<Customer>& custs, std::list<int> best, const Customer& depot) {
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
    if (!v.CloseTravel(*it, depot)) {
        throw std::runtime_error("Cannot close route without violating work-time constraint");
    }
    this->routes.push_back(v);
}

/** @brief Create one nearest-neighbor initial solution.
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
    while (!dist.empty()) {
        bool stop = false;
        // create an empty route
        Route v(this->capacity, this->workTime, this->graph, this->costTravel, this->alphaParam);
        Customer from, to;
        // start the route with a depot
        if (!v.Travel(depot, it->second))
            throw std::runtime_error("Customer unreachable!");
        // if one customer is left add it to route
        if (dist.size() == 1) {
            if (!v.CloseTravel(it->second, depot)) {
                throw std::runtime_error("Cannot close route without violating work-time constraint");
            }
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
            if (from != to && !v.Travel(from, to)) {
                stop = true;
                // if cannot add the customer try to close the route
                if (!v.CloseTravel(from, depot))
                    throw std::runtime_error("Cannot close route without violating work-time constraint");
                // if the customer is added and remain one customer add it to route
            } else if (dist.size() == 1) {
                if (!v.CloseTravel(to, depot))
                    throw std::runtime_error("Cannot close route without violating work-time constraint");
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
    if (this->routes.size() < static_cast<unsigned>(this->vehicles))
        j = -1;
    else if (this->routes.size() == static_cast<unsigned>(this->vehicles))
        j = 0;
    else
        j = 1;
    return j;
}

/** @brief Create an initial solution using nearest-neighbor expansion.
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
    while (std::cmp_less(done.size(), this->numVertices)) {
        bool stop = false;
        Route v(this->capacity, this->workTime, this->graph, this->costTravel, this->alphaParam);
        while (done.find(it->second) != done.end()) {
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
            if (std::cmp_equal(done.size(), this->numVertices - 1)) {
                break;
            }
            // take the next customer, the nearest
            while (done.find(neighit->second) != done.end()) {
                ++neighit;
            }
            to = neighit->second;
            // add path from 'from' to 'to'
            if (!v.Travel(from, to)) {
                stop = true;
            } else {
                done.emplace(from);
            }
        }
        // if cannot add the customer try to close the route
        if (!v.CloseTravel(from, depot)) {
            throw std::runtime_error("Cannot close route without violating work-time constraint");
        } else
            done.emplace(from);
        this->routes.emplace_back(v);
    }
    Utils::Instance().logger("Routes created", Utils::VERBOSE);
    int j = 0;
    if (this->routes.size() < static_cast<unsigned>(this->vehicles))
        j = -1;
    else if (this->routes.size() == static_cast<unsigned>(this->vehicles))
        j = 0;
    else
        j = 1;
    return j;
}

/** @brief Create an initial solution with Clarke-Wright savings.
 *
 * Starts with one route per customer, then greedily merges compatible route
 * endpoints by descending depot-distance savings while preserving constraints.
 * @return Error or Warning code.
 */
int VRP::InitSolutionsSavings() {
    Map dist = this->graph.sortV0();
    Customer depot = dist.cbegin()->second;
    dist.erase(dist.cbegin());
    std::vector<Customer> customers;
    customers.reserve(dist.size());
    for (const auto& entry : dist) {
        customers.push_back(entry.second);
    }

    OptimalMove opt;
    std::optional<Routes> bestRoutes;
    for (double lambda : {0.4, 0.6, 0.8, 1.0, 1.2, 1.4, 1.6, 1.8, 2.0}) {
        Routes candidate = BuildSavingsRoutes(this->graph, customers, depot, this->capacity, this->workTime,
                                              this->costTravel, this->alphaParam, lambda);
        opt.ReduceRoutes(candidate);
        opt.OptRouteTsp(candidate, 14);
        if (!bestRoutes.has_value() || RoutesCost(candidate) < RoutesCost(*bestRoutes)) {
            bestRoutes = std::move(candidate);
        }
    }
    if (!bestRoutes.has_value()) {
        throw std::runtime_error("Savings initialization failed");
    }
    this->routes = std::move(*bestRoutes);
    const int savingsCost = RoutesCost(this->routes);
    std::optional<Routes> sweepRoutes =
        BuildSweepRoutes(this->graph, this->capacity, this->workTime, this->costTravel, this->alphaParam);
    if (sweepRoutes.has_value() && RoutesCost(*sweepRoutes) < savingsCost) {
        this->routes = std::move(*sweepRoutes);
        Utils::Instance().logger("Sweep routes selected", Utils::VERBOSE);
    }
    opt.OptRouteTsp(this->routes, 14);
    Utils::Instance().logger("Initial routes created", Utils::VERBOSE);
    return CompareRouteCount(this->routes.size(), this->vehicles);
}

/** @brief Run the tabu search function.
 *
 * Run the basic function of this algorithm.
 * @param[in] times Iteration condition.
 */
void VRP::RunTabuSearch(int times) {
    TabuSearch s(this->graph, this->numVertices);
    s.Tabu(this->routes, times);
}

/** @brief Run the configured local-search optimization functions.
 *
 * Runs all the optimal functions to achieve a better optimization of the routes.
 * When the routine do not improve the routes, stops and try to balance the routes.
 * @param[in] times Number of iteration
 * @param[in] flag  Force or not the moves
 * @return          If the routine made some improvements.
 */
bool VRP::RunOpts(int times, bool flag) {
    OptimalMove opt;
    int i = 0;
    std::chrono::minutes::rep duration = 0;
    bool improved = false;
    if (flag) {
        Utils::Instance().logger("Forced opt diversification", Utils::VERBOSE);
        opt.Opt10(this->routes, true);
    }
    auto runVndStep = [this, &opt]() {
        if (opt.ReduceRoutes(this->routes) > 0)
            return true;
        if (opt.Opt10(this->routes, false) > 0)
            return true;
        if (opt.Opt11(this->routes, false) > 0)
            return true;
        if (opt.Opt12(this->routes, false) > 0)
            return true;
        if (opt.Opt21(this->routes, false) > 0)
            return true;
        if (opt.Opt22(this->routes, false) > 0)
            return true;
        if (opt.OptSwapSegments(this->routes, 3) > 0)
            return true;
        if (opt.OptRuinRecreate(this->routes, 3, 12) > 0)
            return true;
        if (opt.OptRuinRecreate(this->routes, 4, 12) > 0)
            return true;
        if (this->numVertices > 80 && opt.OptRuinRecreate(this->routes, 5, 14) > 0)
            return true;
        if (this->numVertices > 80 && opt.OptRuinRecreate(this->routes, 6, 14) > 0)
            return true;
        if (opt.OptRelocateSegment(this->routes, 2) > 0)
            return true;
        if (opt.OptRelocateSegment(this->routes, 3) > 0)
            return true;
        if (this->numVertices > 80 && opt.OptRelocateSegment(this->routes, 4) > 0)
            return true;
        if (opt.Opt2Star(this->routes) > 0)
            return true;
        if (this->numVertices > 80 && opt.OptPairSplit(this->routes, 17) > 0)
            return true;
        if (this->numVertices <= 80 && opt.OptPairSplit(this->routes, 14) > 0)
            return true;
        if (opt.OptRouteTsp(this->routes, 14) > 0)
            return true;
        if (opt.Opt2(this->routes))
            return true;
        if (opt.Opt3(this->routes))
            return true;
        return false;
    };
    // start time
    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
    while (i < times && duration <= 25) {
        Utils::Instance().logger("Round " + std::to_string(i + 1) + " of " + std::to_string(times), Utils::VERBOSE);
        if (!runVndStep())
            break;
        improved = true;
        i++;
        // partial time
        std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::minutes>(t2 - t1).count();
    }
    return improved;
}

/** @brief Save the best configuration.
 *
 * Finds out if the actual configuration is the best and save it.
 */
bool VRP::UpdateBest() {
    int tCost = 0;
    for (const auto& e : this->bestRoutes)
        tCost += e.GetTotalCost();
    const int currentCost = this->GetTotalCost();
    if (this->bestRoutes.empty() || currentCost < tCost) {
        this->bestRoutes.clear();
        std::ranges::copy(this->routes, std::back_inserter(this->bestRoutes));
        return true;
    }
    return false;
}

/** @brief Sort the list of routes by cost.
 *
 * This function sort the routes by costs in ascending order.
 */
void VRP::OrderByCosts() {
    std::ranges::sort(this->routes,
                      [](Route const& lhs, Route const& rhs) { return lhs.GetTotalCost() < rhs.GetTotalCost(); });
}

/** @brief Check the integrity of the system.
 *
 * Each customer have to be visited only once, this function checks if
 * this constraint is valid in the system. Used for debugging.
 * @return The status of the constraint
 */
bool VRP::CheckIntegrity() {
    std::list<Customer> checked;
    int custs = 1;
    for (auto& route : this->routes) {
        RouteList::const_iterator ic = route.GetRoute()->cbegin();
        Customer depot = ic->first;
        ++ic;
        for (; ic->first != route.GetRoute()->back().first; ++ic) {
            custs++;
            std::list<Customer>::const_iterator findIter = std::ranges::find(checked, ic->first);
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

/** @brief Compute the total cost of routes.
 *
 * Compute the amount of costs for each route.
 * The cost is used to check improvement on paths.
 * @return The total cost
 */
int VRP::GetTotalCost() {
    this->totalCost = 0;
    for (const auto& e : this->routes) {
        this->totalCost += e.GetTotalCost();
    }
    return this->totalCost;
}

/** @brief Return the number of customers. */
int VRP::GetNumberOfCustomers() const { return numVertices; }

/** @brief Return the routes.
 *
 * @return The pointer to the routes list
 */
Routes* VRP::GetRoutes() { return &this->routes; }

/** @brief Return the best configuration of routes.
 *
 * @return The pointer to the routes list
 */
Routes* VRP::GetBestRoutes() {
    this->UpdateBest();
    return &this->bestRoutes;
}
