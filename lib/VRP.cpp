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
#include <cstdint>
#include <cmath>
#include <limits>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using Map = std::multimap<int, Customer>;

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

/** @brief Candidate route used by route-pool recombination. */
struct RoutePoolCandidate {
    Route route;
    std::vector<std::uint64_t> mask;
    std::vector<int> customers;
    int demand = 0;
    int cost = 0;
};

/** @brief Mutable state for exact search over route-pool candidates. */
struct RoutePoolSearch {
    Routes bestRoutes;
    int minimumRoutes = 0;
    int capacity = 0;
    int customerCount = 0;
    int totalDemand = 0;
    std::size_t nodesVisited = 0;
    std::size_t nodeLimit = 0;
    bool nodeLimitReached = false;
};

constexpr std::size_t kArchiveRoutesPerCustomer = 2;
constexpr std::size_t kArchiveRoutesPerRequiredRoute = 8;
constexpr std::size_t kMinRoutePoolCustomerChoices = 8;
constexpr std::size_t kRoutePoolChoiceSlack = 2;
constexpr std::size_t kRoutePoolNodeBudgetMultiplier = 4;
constexpr int kMaxFreshTabuRestarts = 1;

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
        cost += graph.GetCost(route[i], route[i + 1]);
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

/** @brief Count routes that serve at least one customer. */
std::size_t ActiveRouteCount(const Routes& routes) {
    return static_cast<std::size_t>(std::ranges::count_if(routes, [](const Route& route) { return route.size() > 2; }));
}

/** @brief Return how many active routes exceed the capacity lower bound. */
std::size_t ExcessRouteCount(const Routes& routes, int minimumRoutes) {
    if (minimumRoutes <= 0) {
        return 0;
    }
    const std::size_t activeRoutes = ActiveRouteCount(routes);
    const std::size_t target = static_cast<std::size_t>(minimumRoutes);
    return activeRoutes > target ? activeRoutes - target : 0;
}

/** @brief Compare solutions by route-count feasibility first, then cost. */
bool IsBetterSolution(const Routes& candidate, const Routes& incumbent, int minimumRoutes) {
    if (incumbent.empty()) {
        return true;
    }
    const std::size_t candidateExcess = ExcessRouteCount(candidate, minimumRoutes);
    const std::size_t incumbentExcess = ExcessRouteCount(incumbent, minimumRoutes);
    if (candidateExcess != incumbentExcess) {
        return candidateExcess < incumbentExcess;
    }
    return RoutesCost(candidate) < RoutesCost(incumbent);
}

/** @brief Bounded local-search parameters derived from the current route set. */
struct SearchProfile {
    int relatedRemoval = 0;
    int deepRelatedRemoval = 0;
    int relatedSeedLimit = 0;
    int ruinRemovalMax = 0;
    int ruinSeedLimit = 0;
    int exchangeGroupMax = 0;
    int segmentRelocateMax = 0;
    int boundaryPoolLimit = 0;
    int boundaryPairLimit = 0;
    int pairSplitLimit = 0;
};

// Related-LNS breadth. The seed bounds keep the neighborhood representative
// without scanning every customer, while removal bounds keep regret repair small
// enough to run repeatedly inside VND.
constexpr int kMinRelatedSeeds = 12;
constexpr int kMaxRelatedSeeds = 24;
constexpr int kMinRelatedRemoval = 4;
constexpr int kMaxRelatedRemoval = 7;

// Deeper related-LNS is only used after cheaper moves fail. These bounds allow a
// larger basin change without turning the repair step into full ALNS.
constexpr int kMinDeepRelatedRemoval = 6;
constexpr int kMaxDeepRelatedRemoval = 8;

// Ruin/recreate targets high-contribution customers instead of spatially related
// clusters. The candidate and removal caps bound combination growth.
constexpr int kMinRuinRemoval = 3;
constexpr int kMaxRuinRemoval = 6;
constexpr int kMinRuinSeeds = 12;
constexpr int kMaxRuinSeeds = 18;

// Cross-exchange/string-relocation sizes. Groups above these values grow
// combinatorially and are handled by split/recombination neighborhoods instead.
constexpr int kMinSegmentRelocate = 3;
constexpr int kMaxSegmentRelocate = 4;
constexpr int kMaxArbitraryExchangeGroup = 3;

// Boundary and pair-split candidate limits. These moves repair route membership,
// but exact pair split is exponential in combined customer count, so it needs a
// strict generic cap rather than an instance-specific one.
constexpr int kMinBoundaryPool = 5;
constexpr int kMaxBoundaryPool = 8;
constexpr int kMinBoundaryPairs = 6;
constexpr int kMaxBoundaryPairs = 18;
constexpr int kMinPairSplit = 14;
constexpr int kMaxPairSplit = 18;

// Beam width for related repair states; wider beams improve diversity but are
// expensive because each state carries a full route set.
constexpr int kRelatedBeamWidth = 4;

/** @brief Return how many customers a route serves, excluding depot endpoints. */
std::size_t RouteCustomerCount(const Route& route) {
    return route.size() > 2 ? static_cast<std::size_t>(route.size() - 2) : 0;
}

/** @brief Return the number of served customers in a full route set. */
std::size_t ServedCustomerCount(const Routes& routes) {
    std::size_t count = 0;
    for (const Route& route : routes) {
        count += RouteCustomerCount(route);
    }
    return count;
}

/** @brief Return the largest customer count among active routes. */
std::size_t MaxRouteCustomerCount(const Routes& routes) {
    std::size_t maxCount = 0;
    for (const Route& route : routes) {
        maxCount = std::max(maxCount, RouteCustomerCount(route));
    }
    return maxCount;
}

/** @brief Integer ceil division for search-profile sizing. */
int CeilDiv(std::size_t numerator, std::size_t denominator) {
    return static_cast<int>((numerator + denominator - 1) / denominator);
}

/** @brief Size expensive neighborhoods from route shape instead of instance names or sample sizes. */
SearchProfile BuildSearchProfile(const Routes& routes) {
    const std::size_t activeRoutes = std::max<std::size_t>(1, ActiveRouteCount(routes));
    const std::size_t servedCustomers = ServedCustomerCount(routes);
    const int averageRouteCustomers = CeilDiv(servedCustomers, activeRoutes);
    const int maxRouteCustomers = static_cast<int>(MaxRouteCustomerCount(routes));
    const int seedLimit =
        std::clamp(static_cast<int>(activeRoutes * 2) + averageRouteCustomers, kMinRelatedSeeds, kMaxRelatedSeeds);
    const int relatedRemoval = std::min(
        maxRouteCustomers, std::clamp((averageRouteCustomers / 2) + 1, kMinRelatedRemoval, kMaxRelatedRemoval));
    return SearchProfile{
        .relatedRemoval = relatedRemoval,
        .deepRelatedRemoval =
            std::min(maxRouteCustomers, std::clamp(relatedRemoval + 2, kMinDeepRelatedRemoval, kMaxDeepRelatedRemoval)),
        .relatedSeedLimit = seedLimit,
        .ruinRemovalMax =
            std::min(maxRouteCustomers, std::clamp(averageRouteCustomers / 2, kMinRuinRemoval, kMaxRuinRemoval)),
        .ruinSeedLimit =
            std::clamp(static_cast<int>(activeRoutes) + averageRouteCustomers / 2, kMinRuinSeeds, kMaxRuinSeeds),
        .exchangeGroupMax = std::min(maxRouteCustomers, kMaxArbitraryExchangeGroup),
        .segmentRelocateMax = std::min(
            maxRouteCustomers, std::clamp((averageRouteCustomers + 5) / 4, kMinSegmentRelocate, kMaxSegmentRelocate)),
        .boundaryPoolLimit = std::clamp((averageRouteCustomers + 1) / 2, kMinBoundaryPool, kMaxBoundaryPool),
        .boundaryPairLimit = std::clamp(static_cast<int>(activeRoutes), kMinBoundaryPairs, kMaxBoundaryPairs),
        .pairSplitLimit = std::clamp(averageRouteCustomers + 6, kMinPairSplit, kMaxPairSplit),
    };
}

/** @brief Return true when a bit is present in a dynamic route mask. */
bool MaskContains(const std::vector<std::uint64_t>& mask, int bit) {
    const std::size_t word = static_cast<std::size_t>(bit) / 64;
    const std::size_t offset = static_cast<std::size_t>(bit) % 64;
    return (mask[word] & (std::uint64_t{1} << offset)) != 0;
}

/** @brief Set one customer bit in a dynamic route mask. */
void MaskSet(std::vector<std::uint64_t>& mask, int bit) {
    const std::size_t word = static_cast<std::size_t>(bit) / 64;
    const std::size_t offset = static_cast<std::size_t>(bit) % 64;
    mask[word] |= std::uint64_t{1} << offset;
}

/** @brief Return true when two route masks overlap. */
bool MasksOverlap(const std::vector<std::uint64_t>& left, const std::vector<std::uint64_t>& right) {
    for (std::size_t word = 0; word < left.size(); ++word) {
        if ((left[word] & right[word]) != 0) {
            return true;
        }
    }
    return false;
}

/** @brief Add all bits from a candidate route mask into the covered mask. */
void MaskAdd(std::vector<std::uint64_t>& covered, const std::vector<std::uint64_t>& candidate) {
    for (std::size_t word = 0; word < covered.size(); ++word) {
        covered[word] |= candidate[word];
    }
}

/** @brief Return the first customer not covered by the current route-pool search mask. */
int FirstUncoveredCustomer(const std::vector<std::uint64_t>& covered, int customerCount) {
    for (int customer = 0; customer < customerCount; ++customer) {
        if (!MaskContains(covered, customer)) {
            return customer;
        }
    }
    return -1;
}

/** @brief Extract non-depot customers from a route as an indexable vector. */
std::vector<Customer> RouteCustomers(const Route& route) {
    std::vector<Customer> customers;
    const Customer& depot = route.GetRoute()->front().first;
    for (const auto& step : *route.GetRoute()) {
        if (step.first != depot) {
            customers.push_back(step.first);
        }
    }
    return customers;
}

/** @brief Return a route identity independent of customer visit order. */
std::vector<std::string> RouteSignature(const Route& route) {
    std::vector<std::string> names;
    for (const Customer& customer : RouteCustomers(route)) {
        names.push_back(customer.name);
    }
    std::ranges::sort(names);
    return names;
}

/** @brief Return a normalized archive priority; lower values are kept first. */
double RouteArchiveScore(const Route& route) {
    const int customerCount = std::max(1, route.size() - 2);
    return static_cast<double>(route.GetTotalCost()) / static_cast<double>(customerCount);
}

/** @brief Return how many route fragments may be retained for recombination. */
std::size_t RouteArchiveLimit(int customerCount, int minimumRoutes) {
    return std::max(static_cast<std::size_t>(std::max(1, customerCount)) * kArchiveRoutesPerCustomer,
                    static_cast<std::size_t>(std::max(1, minimumRoutes)) * kArchiveRoutesPerRequiredRoute);
}

/** @brief Add one feasible route to the recombination pool, replacing duplicate sets by lower cost. */
void AddRoutePoolCandidate(std::vector<RoutePoolCandidate>& pool,
                           std::map<std::vector<int>, std::size_t>& byCustomerSet,
                           const std::map<std::string, int>& customerIndexByName, const Route& route) {
    const std::vector<Customer> routeCustomers = RouteCustomers(route);
    if (routeCustomers.empty()) {
        return;
    }
    std::vector<int> customerIndexes;
    customerIndexes.reserve(routeCustomers.size());
    int demand = 0;
    const std::size_t maskWords = (customerIndexByName.size() + 63) / 64;
    std::vector<std::uint64_t> mask(maskWords, 0);
    for (const Customer& customer : routeCustomers) {
        const auto found = customerIndexByName.find(customer.name);
        if (found == customerIndexByName.end()) {
            return;
        }
        customerIndexes.push_back(found->second);
        demand += customer.request;
        MaskSet(mask, found->second);
    }
    std::ranges::sort(customerIndexes);
    const auto duplicate = byCustomerSet.find(customerIndexes);
    if (duplicate != byCustomerSet.end()) {
        RoutePoolCandidate& existing = pool[duplicate->second];
        if (route.GetTotalCost() < existing.cost) {
            existing = RoutePoolCandidate{
                .route = route,
                .mask = std::move(mask),
                .customers = std::move(customerIndexes),
                .demand = demand,
                .cost = route.GetTotalCost(),
            };
        }
        return;
    }
    byCustomerSet[customerIndexes] = pool.size();
    pool.push_back(RoutePoolCandidate{
        .route = route,
        .mask = std::move(mask),
        .customers = std::move(customerIndexes),
        .demand = demand,
        .cost = route.GetTotalCost(),
    });
}

/** @brief Add every active route from a solution to the recombination pool. */
void AddRoutePoolCandidates(std::vector<RoutePoolCandidate>& pool,
                            std::map<std::vector<int>, std::size_t>& byCustomerSet,
                            const std::map<std::string, int>& customerIndexByName, const Routes& routes) {
    for (const Route& route : routes) {
        AddRoutePoolCandidate(pool, byCustomerSet, customerIndexByName, route);
    }
}

/** @brief Materialize selected route-pool candidate indexes as a route set. */
Routes BuildRoutesFromPool(const std::vector<RoutePoolCandidate>& pool, const std::vector<std::size_t>& selected) {
    Routes routes;
    routes.reserve(selected.size());
    for (const std::size_t index : selected) {
        routes.push_back(pool[index].route);
    }
    return routes;
}

/** @brief Depth-first exact cover search over the collected route-pool candidates. */
void SearchRoutePool(const std::vector<RoutePoolCandidate>& pool,
                     const std::vector<std::vector<std::size_t>>& byCustomer, RoutePoolSearch& search,
                     std::vector<std::uint64_t>& covered, int coveredCount, int coveredDemand, int currentCost,
                     std::vector<std::size_t>& selected) {
    if (search.nodeLimit > 0 && search.nodesVisited++ >= search.nodeLimit) {
        search.nodeLimitReached = true;
        return;
    }
    if (coveredCount == search.customerCount) {
        Routes candidate = BuildRoutesFromPool(pool, selected);
        if (IsBetterSolution(candidate, search.bestRoutes, search.minimumRoutes)) {
            search.bestRoutes = std::move(candidate);
        }
        return;
    }
    const std::size_t incumbentRouteCount = ActiveRouteCount(search.bestRoutes);
    const int incumbentCost = RoutesCost(search.bestRoutes);
    if (std::cmp_less_equal(incumbentRouteCount, search.minimumRoutes) && currentCost >= incumbentCost) {
        return;
    }

    const int firstUncovered = FirstUncoveredCustomer(covered, search.customerCount);
    if (firstUncovered < 0) {
        return;
    }
    const int remainingDemand = search.totalDemand - coveredDemand;
    const int remainingRouteLowerBound = (remainingDemand + search.capacity - 1) / search.capacity;
    if (selected.size() >= incumbentRouteCount) {
        return;
    }
    if (selected.size() + static_cast<std::size_t>(remainingRouteLowerBound) > incumbentRouteCount) {
        return;
    }
    if (selected.size() + static_cast<std::size_t>(remainingRouteLowerBound) == incumbentRouteCount &&
        currentCost >= incumbentCost) {
        return;
    }

    for (const std::size_t candidateIndex : byCustomer[static_cast<std::size_t>(firstUncovered)]) {
        if (search.nodeLimitReached) {
            return;
        }
        const RoutePoolCandidate& candidate = pool[candidateIndex];
        if (MasksOverlap(covered, candidate.mask)) {
            continue;
        }
        std::vector<std::uint64_t> nextCovered = covered;
        MaskAdd(nextCovered, candidate.mask);
        selected.push_back(candidateIndex);
        SearchRoutePool(pool, byCustomer, search, nextCovered,
                        coveredCount + static_cast<int>(candidate.customers.size()), coveredDemand + candidate.demand,
                        currentCost + candidate.cost, selected);
        selected.pop_back();
    }
}

/** @brief Recombine routes from multiple initial solutions with exact set partitioning over the route pool. */
std::optional<Routes> RecombineRoutePool(const std::vector<RoutePoolCandidate>& pool,
                                         const std::vector<Customer>& customers, int capacity, int minimumRoutes,
                                         const Routes& incumbent) {
    if (pool.empty() || incumbent.empty()) {
        return std::nullopt;
    }
    std::vector<std::vector<std::size_t>> candidateIndexesByCustomer(customers.size());
    for (std::size_t routeIndex = 0; routeIndex < pool.size(); ++routeIndex) {
        for (const int customerIndex : pool[routeIndex].customers) {
            candidateIndexesByCustomer[static_cast<std::size_t>(customerIndex)].push_back(routeIndex);
        }
    }
    const std::size_t incumbentRouteCount = ActiveRouteCount(incumbent);
    const std::size_t perCustomerChoiceLimit =
        std::max(kMinRoutePoolCustomerChoices, incumbentRouteCount + kRoutePoolChoiceSlack);
    std::vector<bool> allowedRoute(pool.size(), false);
    for (std::vector<std::size_t>& routeIndexes : candidateIndexesByCustomer) {
        if (routeIndexes.empty()) {
            return std::nullopt;
        }
        std::ranges::sort(routeIndexes, [&pool](std::size_t left, std::size_t right) {
            if (pool[left].cost != pool[right].cost) {
                return pool[left].cost < pool[right].cost;
            }
            return left < right;
        });
        if (routeIndexes.size() > perCustomerChoiceLimit) {
            routeIndexes.resize(perCustomerChoiceLimit);
        }
        for (const std::size_t routeIndex : routeIndexes) {
            allowedRoute[routeIndex] = true;
        }
    }
    std::vector<std::vector<std::size_t>> byCustomer(customers.size());
    for (std::size_t routeIndex = 0; routeIndex < pool.size(); ++routeIndex) {
        if (!allowedRoute[routeIndex]) {
            continue;
        }
        for (const int customerIndex : pool[routeIndex].customers) {
            byCustomer[static_cast<std::size_t>(customerIndex)].push_back(routeIndex);
        }
    }
    for (std::vector<std::size_t>& routeIndexes : byCustomer) {
        if (routeIndexes.empty()) {
            return std::nullopt;
        }
        std::ranges::sort(routeIndexes, [&pool](std::size_t left, std::size_t right) {
            if (pool[left].cost != pool[right].cost) {
                return pool[left].cost < pool[right].cost;
            }
            return left < right;
        });
    }
    int totalDemand = 0;
    for (const Customer& customer : customers) {
        totalDemand += customer.request;
    }
    RoutePoolSearch search{
        .bestRoutes = incumbent,
        .minimumRoutes = minimumRoutes,
        .capacity = capacity,
        .customerCount = static_cast<int>(customers.size()),
        .totalDemand = totalDemand,
        .nodeLimit = customers.size() * perCustomerChoiceLimit * incumbentRouteCount * kRoutePoolNodeBudgetMultiplier,
    };
    std::vector<std::uint64_t> covered((customers.size() + 63) / 64, 0);
    std::vector<std::size_t> selected;
    SearchRoutePool(pool, byCustomer, search, covered, 0, 0, 0, selected);
    if (IsBetterSolution(search.bestRoutes, incumbent, minimumRoutes)) {
        return search.bestRoutes;
    }
    return std::nullopt;
}

/** @brief Return a stable customer list from a complete solution. */
std::vector<Customer> UniqueCustomersFromRoutes(const Routes& routes) {
    std::map<std::string, Customer> byName;
    for (const Route& route : routes) {
        for (const Customer& customer : RouteCustomers(route)) {
            byName.emplace(customer.name, customer);
        }
    }
    std::vector<Customer> customers;
    customers.reserve(byName.size());
    for (const auto& [_, customer] : byName) {
        customers.push_back(customer);
    }
    return customers;
}

/** @brief Build route-pool customer indexes for a stable customer list. */
std::map<std::string, int> BuildCustomerIndexByName(const std::vector<Customer>& customers) {
    std::map<std::string, int> customerIndexByName;
    for (std::size_t index = 0; index < customers.size(); ++index) {
        customerIndexByName[customers[index].name] = static_cast<int>(index);
    }
    return customerIndexByName;
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
VRP::VRP(Graph&& g, const int n, const int v, const int c, const int minRoutes, const float t, const bool flagTime,
         const float costTravel, const float alphaParam)
    : graph(std::move(g)) {
    this->numVertices = n;
    this->vehicles = v;
    this->capacity = c;
    this->minimumRoutes = minRoutes;
    // if no service time is needed
    if (flagTime)
        this->workTime = t;
    else
        this->workTime = std::numeric_limits<float>::max();
    this->costTravel = costTravel;
    this->totalCost = 0;
    this->alphaParam = alphaParam;
    this->tabuSearch.emplace(this->graph, this->numVertices);
}

/** @brief Create an initial solution with Clarke-Wright savings.
 *
 * Starts with one route per customer, then greedily merges compatible route
 * endpoints by descending depot-distance savings while preserving constraints.
 * @return Error or Warning code.
 */
int VRP::InitSolutionsSavings() {
    this->freshTabuRestartsUsed = 0;
    Map dist = this->graph.sortV0();
    Customer depot = dist.cbegin()->second;
    dist.erase(dist.cbegin());
    std::vector<Customer> customers;
    customers.reserve(dist.size());
    for (const auto& entry : dist) {
        customers.push_back(entry.second);
    }
    // Route-pool recombination stores candidates by compact customer indexes, so
    // customer names are translated once before the construction multi-start.
    std::map<std::string, int> customerIndexByName;
    for (std::size_t index = 0; index < customers.size(); ++index) {
        customerIndexByName[customers[index].name] = static_cast<int>(index);
    }
    std::vector<RoutePoolCandidate> routePool;
    std::map<std::vector<int>, std::size_t> routePoolByCustomerSet;

    OptimalMove opt;
    std::optional<Routes> bestRoutes;
    // Sweep the Clarke-Wright lambda parameter to create different route
    // memberships without using instance-specific starts or hardcoded tours.
    for (double lambda : {0.4, 0.6, 0.8, 1.0, 1.2, 1.4, 1.6, 1.8, 2.0}) {
        Routes candidate = BuildSavingsRoutes(this->graph, customers, depot, this->capacity, this->workTime,
                                              this->costTravel, this->alphaParam, lambda);
        opt.OptRouteTsp(candidate, 14);
        AddRoutePoolCandidates(routePool, routePoolByCustomerSet, customerIndexByName, candidate);
        this->ArchiveRoutes(candidate);
        if (!bestRoutes.has_value() || IsBetterSolution(candidate, *bestRoutes, this->minimumRoutes)) {
            bestRoutes = std::move(candidate);
        }
    }
    if (!bestRoutes.has_value()) {
        throw std::runtime_error("Savings initialization failed");
    }
    this->routes = std::move(*bestRoutes);
    std::optional<Routes> sweepRoutes =
        BuildSweepRoutes(this->graph, this->capacity, this->workTime, this->costTravel, this->alphaParam);
    if (sweepRoutes.has_value()) {
        // The sweep construction is kept even when it is not the best complete
        // solution because individual routes can still improve later pool picks.
        AddRoutePoolCandidates(routePool, routePoolByCustomerSet, customerIndexByName, *sweepRoutes);
        this->ArchiveRoutes(*sweepRoutes);
    }
    if (sweepRoutes.has_value() && IsBetterSolution(*sweepRoutes, this->routes, this->minimumRoutes)) {
        this->routes = std::move(*sweepRoutes);
        Utils::Instance().logger("Sweep routes selected", Utils::VERBOSE);
    }
    std::optional<Routes> recombinedRoutes =
        RecombineRoutePool(routePool, customers, this->capacity, this->minimumRoutes, this->routes);
    if (recombinedRoutes.has_value() && IsBetterSolution(*recombinedRoutes, this->routes, this->minimumRoutes)) {
        // Exact set partitioning can combine good routes from different starts
        // that no single construction pass produced together.
        this->routes = std::move(*recombinedRoutes);
        Utils::Instance().logger("Route pool recombination selected", Utils::VERBOSE);
    }
    opt.OptRouteTsp(this->routes, 14);
    this->ArchiveRoutes(this->routes);
    Utils::Instance().logger("Initial routes created", Utils::VERBOSE);
    return CompareRouteCount(this->routes.size(), this->vehicles);
}

/** @brief Run the tabu search function.
 *
 * Run the basic function of this algorithm.
 * @param[in] times Iteration condition.
 */
void VRP::RunTabuSearch(int times) {
    if (!this->tabuSearch.has_value()) {
        this->tabuSearch.emplace(this->graph, this->numVertices);
    }
    this->tabuSearch->Tabu(this->routes, times);
}

/** @brief Run the configured local-search optimization functions.
 *
 * Runs all the optimal functions to achieve a better optimization of the routes.
 * When the routine do not improve the routes, stops and try to balance the routes.
 * @param[in] times Number of iteration
 * @param[in] flag  Force or not the moves
 * @return          If the routine made some improvements.
 */
bool VRP::RunOpts(int times, bool flag, int diversificationRank) {
    OptimalMove opt;
    int i = 0;
    std::chrono::minutes::rep duration = 0;
    bool improved = false;
    auto runVndStep = [this, &opt](Routes& workingRoutes, bool allowDeepSearch) {
        const SearchProfile profile = BuildSearchProfile(workingRoutes);
        // VND accepts the first improving neighborhood, then restarts from the
        // cheapest moves so expensive repairs run only after simpler moves stall.
        if (opt.ReduceRoutes(workingRoutes, static_cast<std::size_t>(this->minimumRoutes)) > 0)
            return true;
        if (opt.Opt10(workingRoutes, false) > 0)
            return true;
        if (opt.Opt11(workingRoutes, false) > 0)
            return true;
        if (opt.Opt12(workingRoutes, false) > 0)
            return true;
        if (opt.Opt21(workingRoutes, false) > 0)
            return true;
        if (opt.Opt22(workingRoutes, false) > 0)
            return true;
        if (opt.OptCyclicExchange(workingRoutes, 1) > 0)
            return true;
        if (opt.OptSwapSegments(workingRoutes, profile.segmentRelocateMax) > 0)
            return true;
        const int shallowRelatedSeedLimit = std::min(18, profile.relatedSeedLimit);
        if (opt.OptRelatedRuinRecreate(workingRoutes, profile.relatedRemoval, shallowRelatedSeedLimit) > 0)
            return true;
        if (opt.Opt2Star(workingRoutes) > 0)
            return true;
        if (opt.OptRouteTsp(workingRoutes, 14) > 0)
            return true;
        if (opt.Opt2(workingRoutes))
            return true;
        if (opt.Opt3(workingRoutes))
            return true;
        if (!allowDeepSearch)
            return false;
        // Deep neighborhoods move several customers or route memberships at once;
        // they are bounded by SearchProfile to keep large instances tractable.
        if (profile.deepRelatedRemoval > profile.relatedRemoval &&
            opt.OptRelatedRuinRecreate(workingRoutes, profile.deepRelatedRemoval, profile.relatedSeedLimit) > 0)
            return true;
        if (profile.relatedSeedLimit > shallowRelatedSeedLimit &&
            opt.OptRelatedRuinRecreate(workingRoutes, profile.relatedRemoval, profile.relatedSeedLimit) > 0)
            return true;
        if (opt.OptRelatedBeamRuinRecreate(workingRoutes, profile.deepRelatedRemoval, profile.relatedSeedLimit,
                                           kRelatedBeamWidth) > 0)
            return true;
        if (profile.ruinRemovalMax >= 4 && opt.OptExchange(workingRoutes, 3, 1, false) > 0)
            return true;
        if (profile.ruinRemovalMax >= 4 && opt.OptExchange(workingRoutes, 1, 3, false) > 0)
            return true;
        for (int sourceGroupSize = 1; sourceGroupSize <= profile.exchangeGroupMax; ++sourceGroupSize) {
            for (int destGroupSize = 1; destGroupSize <= profile.exchangeGroupMax; ++destGroupSize) {
                if (sourceGroupSize + destGroupSize <= 4)
                    continue;
                if (opt.OptExchange(workingRoutes, sourceGroupSize, destGroupSize, false) > 0)
                    return true;
            }
        }
        for (int removalCount = 3; removalCount <= profile.ruinRemovalMax; ++removalCount) {
            if (opt.OptRuinRecreate(workingRoutes, removalCount, profile.ruinSeedLimit) > 0)
                return true;
        }
        for (int segmentSize = 2; segmentSize <= profile.segmentRelocateMax; ++segmentSize) {
            if (opt.OptRelocateSegment(workingRoutes, segmentSize) > 0)
                return true;
        }
        if (opt.OptBoundaryPairSplit(workingRoutes, profile.boundaryPoolLimit, profile.boundaryPairLimit) > 0)
            return true;
        if (opt.OptPairSweepSplit(workingRoutes) > 0)
            return true;
        if (opt.OptRouteClusterSplit(workingRoutes, profile.boundaryPoolLimit) > 0)
            return true;
        if (opt.OptPairSplit(workingRoutes, profile.pairSplitLimit) > 0)
            return true;
        return false;
    };
    if (flag) {
        Utils::Instance().logger("Forced opt diversification", Utils::VERBOSE);
        const SearchProfile profile = BuildSearchProfile(this->routes);
        const int perturbationRemoval =
            std::min(profile.relatedSeedLimit, profile.deepRelatedRemoval + std::max(0, diversificationRank));
        // Rotate perturbation families by rank so repeated stagnant rounds enter
        // different basins before the strict VND pass tries to polish them.
        switch (diversificationRank % 4) {
        case 0:
            opt.PerturbRelatedRuinRecreate(this->routes, perturbationRemoval, profile.relatedSeedLimit,
                                           diversificationRank);
            break;
        case 1:
            opt.PerturbAngularRuinRecreate(this->routes, perturbationRemoval, diversificationRank);
            break;
        case 2:
            opt.OptSwapSegments(this->routes, profile.segmentRelocateMax, true);
            break;
        default:
            opt.OptExchange(this->routes, 2, 2, true);
            break;
        }
    }
    // start time
    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
    while (i < times && duration <= 25) {
        Utils::Instance().logger("Round " + std::to_string(i + 1) + " of " + std::to_string(times), Utils::VERBOSE);
        if (!runVndStep(this->routes, true))
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
    // Archive before comparing incumbents so recombination can use useful routes
    // from a non-winning solution without accepting that full solution.
    this->ArchiveRoutes(this->routes);
    bool updated = false;
    if (IsBetterSolution(this->routes, this->bestRoutes, this->minimumRoutes)) {
        this->bestRoutes.clear();
        std::ranges::copy(this->routes, std::back_inserter(this->bestRoutes));
        updated = true;
    }
    return this->RecombineArchivedRoutes() || updated;
}

/** @brief Restore the incumbent best solution as the active search state. */
void VRP::RestoreBest() {
    if (!this->bestRoutes.empty()) {
        this->routes = this->bestRoutes;
    }
}

/** @brief Store non-empty routes from a complete solution for route-pool recombination. */
void VRP::ArchiveRoutes(const Routes& solution) {
    // A sorted customer-name signature identifies the membership of a route
    // independently of its traversal order.
    std::map<std::vector<std::string>, std::size_t> archivedRouteIndex;
    for (std::size_t index = 0; index < this->routeArchive.size(); ++index) {
        archivedRouteIndex.emplace(RouteSignature(this->routeArchive[index]), index);
    }

    for (const Route& route : solution) {
        if (route.size() <= 2) {
            continue;
        }
        const std::vector<std::string> signature = RouteSignature(route);
        const auto existing = archivedRouteIndex.find(signature);
        if (existing != archivedRouteIndex.end()) {
            Route& archivedRoute = this->routeArchive[existing->second];
            if (route.GetTotalCost() < archivedRoute.GetTotalCost()) {
                archivedRoute = route;
            }
            continue;
        }
        archivedRouteIndex.emplace(signature, this->routeArchive.size());
        this->routeArchive.push_back(route);
    }

    const std::size_t archiveLimit = RouteArchiveLimit(this->numVertices - 1, this->minimumRoutes);
    while (this->routeArchive.size() > archiveLimit) {
        // Drop the worst archived route by score so the set-partitioning pool
        // stays bounded while preserving the strongest membership candidates.
        const auto worst = std::ranges::max_element(this->routeArchive, [](const Route& left, const Route& right) {
            const double leftScore = RouteArchiveScore(left);
            const double rightScore = RouteArchiveScore(right);
            if (leftScore != rightScore) {
                return leftScore < rightScore;
            }
            return left.GetTotalCost() < right.GetTotalCost();
        });
        if (worst == this->routeArchive.end()) {
            return;
        }
        this->routeArchive.erase(worst);
    }
}

/** @brief Try exact set partitioning over all archived route candidates. */
bool VRP::RecombineArchivedRoutes() {
    const Routes& incumbent = this->bestRoutes.empty() ? this->routes : this->bestRoutes;
    if (incumbent.empty() || this->routeArchive.empty()) {
        return false;
    }
    const std::vector<Customer> customers = UniqueCustomersFromRoutes(incumbent);
    if (std::cmp_not_equal(customers.size(), this->numVertices - 1)) {
        return false;
    }
    const std::map<std::string, int> customerIndexByName = BuildCustomerIndexByName(customers);
    std::vector<RoutePoolCandidate> routePool;
    std::map<std::vector<int>, std::size_t> routePoolByCustomerSet;
    // Seed the pool with the incumbent so recombination is never worse than the
    // current best unless a strictly better exact cover is found.
    AddRoutePoolCandidates(routePool, routePoolByCustomerSet, customerIndexByName, incumbent);
    AddRoutePoolCandidates(routePool, routePoolByCustomerSet, customerIndexByName, this->routeArchive);
    std::optional<Routes> recombinedRoutes =
        RecombineRoutePool(routePool, customers, this->capacity, this->minimumRoutes, incumbent);
    if (!recombinedRoutes.has_value() || !IsBetterSolution(*recombinedRoutes, incumbent, this->minimumRoutes)) {
        return false;
    }
    this->bestRoutes = std::move(*recombinedRoutes);
    this->routes = this->bestRoutes;
    this->ArchiveRoutes(this->bestRoutes);
    Utils::Instance().logger("Route archive recombination improved", Utils::VERBOSE);
    return true;
}

/** @brief Explore bounded perturbation branches from the incumbent only.
 *
 * The main controller restores the incumbent after a failed pass, which keeps
 * quality stable but can also prevent wider perturbations from surviving long
 * enough to expose a better local optimum. This method runs those perturbations
 * on copies of the best route set and commits only strict improvements, so the
 * final incumbent cannot regress.
 */
bool VRP::IntensifyBestWithDiversifiedBranches(int branchCount, int optIterations) {
    if (branchCount <= 0 || optIterations <= 0 || this->bestRoutes.empty()) {
        return false;
    }
    bool improved = false;
    for (int branch = 0; branch < branchCount; ++branch) {
        this->routes = this->bestRoutes;
        OptimalMove opt;
        const SearchProfile profile = BuildSearchProfile(this->routes);
        const int perturbationRemoval =
            std::min(profile.relatedSeedLimit, profile.deepRelatedRemoval + std::max(0, branch));
        // Each branch starts from the incumbent and uses one controlled
        // perturbation; only a later strict improvement is allowed to survive.
        switch (branch % 4) {
        case 0:
            opt.PerturbRelatedRuinRecreate(this->routes, perturbationRemoval, profile.relatedSeedLimit, branch);
            break;
        case 1:
            opt.PerturbAngularRuinRecreate(this->routes, perturbationRemoval, branch);
            break;
        case 2:
            opt.OptSwapSegments(this->routes, profile.segmentRelocateMax, true);
            break;
        default:
            opt.OptExchange(this->routes, 2, 2, true);
            break;
        }
        this->RunOpts(optIterations, false, branch);
        if (IsBetterSolution(this->routes, this->bestRoutes, this->minimumRoutes)) {
            this->bestRoutes = this->routes;
            this->ArchiveRoutes(this->bestRoutes);
            this->RecombineArchivedRoutes();
            improved = true;
            Utils::Instance().logger("Diversified incumbent branch improved", Utils::VERBOSE);
        }
    }
    this->routes = this->bestRoutes;
    return improved;
}

/** @brief Restart once from the incumbent with fresh tabu memory. */
bool VRP::RestartFromBestWithFreshTabu() {
    if (this->freshTabuRestartsUsed >= kMaxFreshTabuRestarts || this->bestRoutes.empty()) {
        return false;
    }
    this->routes = this->bestRoutes;
    // Rebuild tabu memory while keeping the incumbent route set. This gives the
    // same solution one fresh neighborhood trajectory without repeated restarts.
    this->tabuSearch.emplace(this->graph, this->numVertices);
    ++this->freshTabuRestartsUsed;
    Utils::Instance().logger("Fresh incumbent tabu restart selected", Utils::VERBOSE);
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
