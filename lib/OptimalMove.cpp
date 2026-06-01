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
#include <bit>
#include <iterator>
#include <limits>
#include <optional>
#include <set>
#include <utility>
#include <vector>

auto comp = [](const BestResult& l, const BestResult& r) -> bool {
    const int leftImprovement = l.Improvement();
    const int rightImprovement = r.Improvement();
    if (leftImprovement != rightImprovement) {
        return leftImprovement > rightImprovement;
    }
    const int leftCost = l.NewCost();
    const int rightCost = r.NewCost();
    if (leftCost != rightCost) {
        return leftCost < rightCost;
    }
    if (l.sourceIndex != r.sourceIndex) {
        return l.sourceIndex < r.sourceIndex;
    }
    // Tie-break by route indexes so parallel candidate insertion stays reproducible.
    return l.destIndex < r.destIndex;
};

int ApplyBestResult(Routes& routes, const std::set<BestResult, decltype(comp)>& bestResults) {
    Routes::iterator itFinal = routes.begin();
    const BestResult& best = *bestResults.begin();
    const int indexFrom = best.sourceIndex;
    const int indexTo = best.destIndex;
    std::advance(itFinal, indexFrom);
    *itFinal = best.source;
    itFinal = routes.begin();
    std::advance(itFinal, indexTo);
    *itFinal = best.dest;
    return best.Improvement();
}

/** @brief Best pair of replacement routes produced by a 2-opt* tail exchange. */
struct TwoOptStarRoutes {
    Route source;
    Route dest;
    int improvement;
};

/** @brief 2-opt* result annotated with source and destination route indexes. */
struct IndexedTwoOptStarRoutes {
    Route source;
    Route dest;
    int sourceIndex;
    int destIndex;
    int improvement;
};

/** @brief Best relocation of a contiguous segment between two indexed routes. */
struct SegmentRelocation {
    Route source;
    Route dest;
    int sourceIndex;
    int destIndex;
    int improvement;
};

/** @brief Best exchange of two contiguous segments between indexed routes. */
struct SegmentExchange {
    Route source;
    Route dest;
    int sourceIndex;
    int destIndex;
    int improvement;
};

/** @brief Best exact repartition of two route memberships. */
struct PairSplit {
    Route source;
    Route dest;
    int sourceIndex;
    int destIndex;
    int improvement;
};

/** @brief Route-pair candidate metadata used to scope exact pair splitting. */
struct PairSplitCandidatePair {
    std::size_t sourceIndex;
    std::size_t destIndex;
    float distance;
    int combinedCost;
};

/** @brief Customer removal candidate scored by its route-cost contribution. */
struct RuinCustomer {
    Customer customer;
    int contribution;
    std::size_t sequence;
};

/** @brief Recreated route set after removing and reinserting customers. */
struct RuinRecreateResult {
    Routes routes;
    int improvement;
    std::size_t sequence;
};

/** @brief Snapshot of a route with its original index. */
struct RouteSnapshot {
    Route route;
    int index;
};

/** @brief Copy every customer step from a route into a vector. */
std::vector<Customer> CopyCustomers(const Route& route) {
    std::vector<Customer> customers;
    customers.reserve(route.GetRoute()->size());
    for (const StepType& step : *route.GetRoute()) {
        customers.push_back(step.first);
    }
    return customers;
}

/** @brief Build one 2-opt* side from a prefix of one route and suffix of another. */
std::list<Customer> BuildTwoOptStarRoute(const std::vector<Customer>& prefixRoute, std::size_t prefixCut,
                                         const std::vector<Customer>& suffixRoute, std::size_t suffixCut) {
    std::list<Customer> candidate;
    // The cut is after prefixCut, so the prefix endpoint is kept in the new route.
    for (std::size_t i = 0; i <= prefixCut; ++i) {
        candidate.push_back(prefixRoute[i]);
    }
    // The other route contributes everything after its cut edge.
    for (std::size_t i = suffixCut + 1; i < suffixRoute.size(); ++i) {
        candidate.push_back(suffixRoute[i]);
    }
    return candidate;
}

/** @brief Find the best feasible 2-opt* exchange between two routes. */
std::optional<TwoOptStarRoutes> FindBestTwoOptStarRoutes(const Route& source, const Route& dest) {
    const std::vector<Customer> sourceCustomers = CopyCustomers(source);
    const std::vector<Customer> destCustomers = CopyCustomers(dest);
    const int originalCost = source.GetTotalCost() + dest.GetTotalCost();
    int bestCost = originalCost;
    std::optional<TwoOptStarRoutes> best;
    for (std::size_t sourceCut = 0; sourceCut + 1 < sourceCustomers.size(); ++sourceCut) {
        for (std::size_t destCut = 0; destCut + 1 < destCustomers.size(); ++destCut) {
            // Try swapping the two tails produced by cutting one edge in each route.
            Route candidateSource = source;
            Route candidateDest = dest;
            const std::list<Customer> newSourceRoute =
                BuildTwoOptStarRoute(sourceCustomers, sourceCut, destCustomers, destCut);
            const std::list<Customer> newDestRoute =
                BuildTwoOptStarRoute(destCustomers, destCut, sourceCustomers, sourceCut);
            if (candidateSource.RebuildRoute(newSourceRoute) && candidateDest.RebuildRoute(newDestRoute)) {
                const int candidateCost = candidateSource.GetTotalCost() + candidateDest.GetTotalCost();
                if (candidateCost < bestCost) {
                    bestCost = candidateCost;
                    best = TwoOptStarRoutes{
                        .source = candidateSource,
                        .dest = candidateDest,
                        .improvement = originalCost - candidateCost,
                    };
                }
            }
        }
    }
    return best;
}

/** @brief Return all non-depot customers from a route in route order. */
std::list<Customer> RouteCustomersWithoutDepot(const Route& route) {
    std::list<Customer> customers;
    const Customer& depot = route.GetRoute()->front().first;
    for (const StepType& step : *route.GetRoute()) {
        if (step.first != depot) {
            customers.push_back(step.first);
        }
    }
    return customers;
}

/** @brief Return non-depot route customers in route order. */
std::vector<Customer> RouteCustomerVectorWithoutDepot(const Route& route) {
    std::vector<Customer> customers;
    customers.reserve(static_cast<std::size_t>(std::max(0, route.size() - 2)));
    const Customer& depot = route.GetRoute()->front().first;
    for (const StepType& step : *route.GetRoute()) {
        if (step.first != depot) {
            customers.push_back(step.first);
        }
    }
    return customers;
}

/** @brief Recursively enumerate fixed-size customer subsets in route order. */
void BuildCustomerCombinations(const std::vector<Customer>& customers, int count, std::size_t start,
                               std::list<Customer>& current, std::vector<std::list<Customer>>& combinations) {
    if (std::cmp_equal(current.size(), count)) {
        combinations.push_back(current);
        return;
    }
    for (std::size_t i = start; i < customers.size(); ++i) {
        current.push_back(customers[i]);
        BuildCustomerCombinations(customers, count, i + 1, current, combinations);
        current.pop_back();
    }
}

/** @brief Build all route-order-preserving subsets of a fixed size. */
std::vector<std::list<Customer>> BuildCustomerCombinations(const Route& route, int count) {
    std::vector<std::list<Customer>> combinations;
    if (count <= 0 || route.size() < count + 2) {
        return combinations;
    }
    std::list<Customer> current;
    BuildCustomerCombinations(RouteCustomerVectorWithoutDepot(route), count, 0, current, combinations);
    return combinations;
}

/** @brief Read a fixed-size customer segment from a route iterator range. */
std::list<Customer> ReadSegment(RouteList::const_iterator start, RouteList::const_iterator end, int segmentSize) {
    std::list<Customer> segment;
    for (int count = 0; count < segmentSize && start != end; ++count, ++start) {
        segment.push_back(start->first);
    }
    return segment;
}

/** @brief Return zero for depot-only routes, otherwise the stored route cost. */
int ActiveRouteCost(const Route& route) { return route.size() <= 2 ? 0 : route.GetTotalCost(); }

/** @brief Sum active route costs while ignoring empty depot-only routes. */
int TotalRouteCost(const Routes& routes) {
    int total = 0;
    for (const Route& route : routes) {
        total += ActiveRouteCost(route);
    }
    return total;
}

/** @brief Remove all customers in a segment from a route copy.
 *
 * Segment-based neighborhoods work on route copies. This helper keeps the
 * removal loop consistent and returns false if any expected customer is missing.
 */
bool RemoveSegment(Route& route, const std::list<Customer>& segment) {
    bool removed = true;
    for (const Customer& customer : segment) {
        removed = route.RemoveCustomer(customer) && removed;
    }
    return removed;
}

/** @brief Build a depot-to-depot route list from a Held-Karp parent table. */
std::list<Customer> BuildExactRoute(const Customer& depot, const std::vector<Customer>& customers,
                                    const std::vector<int>& parent, const std::vector<int>& routeLast,
                                    std::size_t mask) {
    std::vector<Customer> ordered;
    ordered.reserve(std::popcount(mask));
    const std::size_t customerCount = customers.size();
    int current = routeLast[mask];
    while (mask != 0 && current >= 0) {
        const std::size_t currentIndex = static_cast<std::size_t>(current);
        ordered.push_back(customers[currentIndex]);
        const int previous = parent[(mask * customerCount) + currentIndex];
        mask ^= std::size_t{1} << currentIndex;
        current = previous;
    }
    std::ranges::reverse(ordered);
    std::list<Customer> route;
    route.push_back(depot);
    std::ranges::copy(ordered, std::back_inserter(route));
    route.push_back(depot);
    return route;
}

/** @brief Find the best exact two-route repartition within a customer-count bound.
 *
 * The two route memberships are pooled, all feasible bipartitions are evaluated,
 * and each side uses the exact minimum-cost customer order from one shared
 * Held-Karp dynamic program. The lowest customer bit is fixed to one side to
 * avoid evaluating symmetric duplicate partitions.
 */
std::optional<PairSplit> FindBestPairSplit(const Route& source, const Route& dest, int sourceIndex, int destIndex,
                                           int maxCombinedCustomers) {
    std::vector<Customer> customers = RouteCustomerVectorWithoutDepot(source);
    const std::vector<Customer> destCustomers = RouteCustomerVectorWithoutDepot(dest);
    customers.insert(customers.end(), destCustomers.cbegin(), destCustomers.cend());
    if (customers.size() < 3 || std::cmp_greater(customers.size(), maxCombinedCustomers)) {
        return std::nullopt;
    }

    const Customer depot = source.GetRoute()->front().first;
    const std::size_t customerCount = customers.size();
    const std::size_t stateCount = std::size_t{1} << customerCount;
    const std::size_t allCustomers = stateCount - 1;
    const int capacity = source.GetInitialCapacity();
    constexpr int infinity = std::numeric_limits<int>::max() / 4;

    std::vector<int> demand(stateCount, 0);
    for (std::size_t mask = 1; mask < stateCount; ++mask) {
        const std::size_t bit = mask & (~mask + 1);
        const std::size_t customer = std::countr_zero(bit);
        demand[mask] = demand[mask ^ bit] + customers[customer].request;
    }

    std::vector<int> dp(stateCount * customerCount, infinity);
    std::vector<int> parent(stateCount * customerCount, -1);
    const auto state = [customerCount](std::size_t mask, std::size_t last) { return (mask * customerCount) + last; };
    for (std::size_t customer = 0; customer < customerCount; ++customer) {
        dp[state(std::size_t{1} << customer, customer)] = source.GetTravelCost(depot, customers[customer]);
    }

    for (std::size_t mask = 1; mask < stateCount; ++mask) {
        for (std::size_t last = 0; last < customerCount; ++last) {
            const int currentCost = dp[state(mask, last)];
            if (currentCost == infinity || (mask & (std::size_t{1} << last)) == 0) {
                continue;
            }
            const std::size_t remaining = allCustomers ^ mask;
            for (std::size_t next = 0; next < customerCount; ++next) {
                const std::size_t nextBit = std::size_t{1} << next;
                if ((remaining & nextBit) == 0) {
                    continue;
                }
                const std::size_t nextMask = mask | nextBit;
                const int candidateCost = currentCost + source.GetTravelCost(customers[last], customers[next]);
                int& bestCost = dp[state(nextMask, next)];
                if (candidateCost < bestCost) {
                    bestCost = candidateCost;
                    parent[state(nextMask, next)] = static_cast<int>(last);
                }
            }
        }
    }

    std::vector<int> routeCost(stateCount, infinity);
    std::vector<int> routeLast(stateCount, -1);
    routeCost[0] = 0;
    for (std::size_t mask = 1; mask < stateCount; ++mask) {
        for (std::size_t last = 0; last < customerCount; ++last) {
            const int pathCost = dp[state(mask, last)];
            if (pathCost == infinity) {
                continue;
            }
            const int candidateCost = pathCost + source.GetTravelCost(customers[last], depot);
            if (candidateCost < routeCost[mask]) {
                routeCost[mask] = candidateCost;
                routeLast[mask] = static_cast<int>(last);
            }
        }
    }

    const int originalCost = source.GetTotalCost() + dest.GetTotalCost();
    int bestImprovement = 0;
    std::size_t bestMask = 0;
    for (std::size_t sourceMask = 1; sourceMask < allCustomers; ++sourceMask) {
        if ((sourceMask & 1U) == 0) {
            continue;
        }
        const std::size_t destMask = allCustomers ^ sourceMask;
        if (demand[sourceMask] > capacity || demand[destMask] > capacity || routeCost[sourceMask] == infinity ||
            routeCost[destMask] == infinity) {
            continue;
        }
        const int candidateCost = routeCost[sourceMask] + routeCost[destMask];
        const int improvement = originalCost - candidateCost;
        if (improvement > bestImprovement) {
            bestImprovement = improvement;
            bestMask = sourceMask;
        }
    }
    if (bestImprovement <= 0) {
        return std::nullopt;
    }

    Route candidateSource = source;
    Route candidateDest = dest;
    if (!candidateSource.RebuildRoute(BuildExactRoute(depot, customers, parent, routeLast, bestMask)) ||
        !candidateDest.RebuildRoute(BuildExactRoute(depot, customers, parent, routeLast, allCustomers ^ bestMask))) {
        return std::nullopt;
    }
    return PairSplit{
        .source = candidateSource,
        .dest = candidateDest,
        .sourceIndex = sourceIndex,
        .destIndex = destIndex,
        .improvement = bestImprovement,
    };
}

/** @brief Copy route list entries into index-addressable snapshots.
 *
 * The local-search neighborhoods keep the original route container immutable
 * while workers are running. A vector snapshot avoids sharing list iterators
 * across threads and preserves the original route index for applying a result.
 */
std::vector<RouteSnapshot> SnapshotRoutes(const Routes& routes) {
    std::vector<RouteSnapshot> snapshots;
    snapshots.reserve(routes.size());
    int index = 0;
    for (const Route& route : routes) {
        snapshots.emplace_back(RouteSnapshot{
            .route = route,
            .index = index,
        });
        ++index;
    }
    return snapshots;
}

/** @brief Collect high-contribution customers for ruin-and-recreate.
 *
 * Contribution is the route-cost saving produced by bypassing that customer.
 * It is used only to choose promising removal candidates, not to score the
 * final solution.
 */
std::vector<RuinCustomer> CollectRuinCustomers(const Routes& routes, int candidateLimit) {
    std::vector<RuinCustomer> customers;
    std::size_t sequence = 0;
    for (const Route& route : routes) {
        if (route.size() <= 2) {
            continue;
        }
        RouteList::const_iterator previous = route.GetRoute()->cbegin();
        RouteList::const_iterator current = previous;
        ++current;
        for (; current != route.GetRoute()->cend() && current->first != route.GetRoute()->front().first;
             ++previous, ++current) {
            const RouteList::const_iterator next = std::next(current);
            const int bypassCost =
                next == route.GetRoute()->cend() ? 0 : route.GetTravelCost(previous->first, next->first);
            customers.emplace_back(RuinCustomer{
                .customer = current->first,
                .contribution = previous->second + current->second - bypassCost,
                .sequence = sequence,
            });
            ++sequence;
        }
    }
    std::ranges::sort(customers, [](const RuinCustomer& left, const RuinCustomer& right) {
        if (left.contribution != right.contribution) {
            return left.contribution > right.contribution;
        }
        return left.sequence < right.sequence;
    });
    if (std::cmp_less(candidateLimit, customers.size())) {
        customers.resize(static_cast<std::size_t>(candidateLimit));
    }
    return customers;
}

/** @brief Recursively enumerate fixed-size ruin-removal combinations. */
void BuildRuinCombinations(const std::vector<RuinCustomer>& customers, int removalCount, std::size_t start,
                           std::vector<RuinCustomer>& current, std::vector<std::vector<RuinCustomer>>& combinations) {
    if (std::cmp_equal(current.size(), removalCount)) {
        combinations.push_back(current);
        return;
    }
    for (std::size_t i = start; i < customers.size(); ++i) {
        current.push_back(customers[i]);
        BuildRuinCombinations(customers, removalCount, i + 1, current, combinations);
        current.pop_back();
    }
}

/** @brief Build fixed-size removal sets from the highest-contribution customers. */
std::vector<std::vector<RuinCustomer>> BuildRuinCombinations(const std::vector<RuinCustomer>& customers,
                                                             int removalCount) {
    std::vector<std::vector<RuinCustomer>> combinations;
    std::vector<RuinCustomer> current;
    current.reserve(static_cast<std::size_t>(removalCount));
    BuildRuinCombinations(customers, removalCount, 0, current, combinations);
    return combinations;
}

/** @brief Insert one customer into the feasible route with the lowest cost increase. */
bool InsertCustomerBestRoute(Routes& routes, const Customer& customer) {
    Routes::iterator bestRoute = routes.end();
    Route bestCandidate = routes.front();
    int bestCostIncrease = 0;
    bool found = false;
    for (Routes::iterator route = routes.begin(); route != routes.end(); ++route) {
        Route candidate = *route;
        if (!candidate.AddElem(customer)) {
            continue;
        }
        const int costIncrease = ActiveRouteCost(candidate) - ActiveRouteCost(*route);
        if (!found || costIncrease < bestCostIncrease) {
            bestRoute = route;
            bestCandidate = candidate;
            bestCostIncrease = costIncrease;
            found = true;
        }
    }
    if (!found) {
        return false;
    }
    *bestRoute = bestCandidate;
    return true;
}

/** @brief Optimize one small route exactly as a fixed-membership TSP. */
int OptimizeSingleRouteTsp(Route& route, int maxCustomers) {
    if (route.size() <= 2) {
        return 0;
    }
    const int customerCount = route.size() - 2;
    if (customerCount <= 1 || customerCount > maxCustomers) {
        return 0;
    }
    std::vector<Customer> customers;
    customers.reserve(static_cast<std::size_t>(customerCount));
    RouteList::const_iterator it = route.GetRoute()->cbegin();
    const Customer depot = it->first;
    for (++it; it != route.GetRoute()->cend() && it->first != depot; ++it) {
        customers.push_back(it->first);
    }
    const std::size_t count = customers.size();
    const std::size_t stateCount = std::size_t{1} << count;
    constexpr int infinity = std::numeric_limits<int>::max() / 4;
    std::vector<std::vector<int>> dp(stateCount, std::vector<int>(count, infinity));
    std::vector<std::vector<int>> parent(stateCount, std::vector<int>(count, -1));
    // dp[mask][last] is the cheapest depot -> ... -> customers[last] path
    // that visits exactly the customer bits contained in mask.
    for (std::size_t customer = 0; customer < count; ++customer) {
        dp[std::size_t{1} << customer][customer] = route.GetTravelCost(depot, customers[customer]);
    }
    for (std::size_t mask = 1; mask < stateCount; ++mask) {
        for (std::size_t last = 0; last < count; ++last) {
            if (dp[mask][last] == infinity || (mask & (std::size_t{1} << last)) == 0) {
                continue;
            }
            for (std::size_t next = 0; next < count; ++next) {
                const std::size_t nextBit = std::size_t{1} << next;
                if ((mask & nextBit) != 0) {
                    continue;
                }
                const std::size_t nextMask = mask | nextBit;
                const int candidateCost = dp[mask][last] + route.GetTravelCost(customers[last], customers[next]);
                if (candidateCost < dp[nextMask][next]) {
                    dp[nextMask][next] = candidateCost;
                    // Remember the predecessor so the optimal route order can be rebuilt.
                    parent[nextMask][next] = static_cast<int>(last);
                }
            }
        }
    }
    const std::size_t allVisited = stateCount - 1;
    int bestCost = infinity;
    int bestLast = -1;
    for (std::size_t last = 0; last < count; ++last) {
        const int candidateCost = dp[allVisited][last] + route.GetTravelCost(customers[last], depot);
        if (candidateCost < bestCost) {
            bestCost = candidateCost;
            bestLast = static_cast<int>(last);
        }
    }
    if (bestLast < 0 || bestCost >= route.GetTotalCost()) {
        return 0;
    }
    std::vector<Customer> ordered;
    ordered.reserve(count);
    std::size_t mask = allVisited;
    int current = bestLast;
    // Walk parent pointers backward from the cheapest final customer.
    while (current >= 0) {
        ordered.push_back(customers[static_cast<std::size_t>(current)]);
        const int previous = parent[mask][static_cast<std::size_t>(current)];
        // Clear the bit for the customer we just consumed.
        mask ^= std::size_t{1} << static_cast<std::size_t>(current);
        current = previous;
    }
    std::ranges::reverse(ordered);
    std::list<Customer> rebuilt;
    rebuilt.push_back(depot);
    std::ranges::copy(ordered, std::back_inserter(rebuilt));
    rebuilt.push_back(depot);
    Route candidate = route;
    if (!candidate.RebuildRoute(rebuilt)) {
        return 0;
    }
    const int improvement = route.GetTotalCost() - candidate.GetTotalCost();
    route = std::move(candidate);
    return improvement;
}

/** @brief Evaluate one ruin-and-recreate candidate on a route copy.
 *
 * The selected customers are removed together, empty depot-only routes are kept
 * available, and the removed customers are then reinserted globally in best
 * feasible positions.
 * Only strict total-cost improvements are returned to the caller.
 */
std::optional<RuinRecreateResult> EvaluateRuinRecreate(const Routes& routes, const std::vector<RuinCustomer>& customers,
                                                       std::size_t sequence) {
    Routes candidate = routes;
    const int originalCost = TotalRouteCost(routes);
    for (const RuinCustomer& customer : customers) {
        for (Route& route : candidate) {
            if (route.RemoveCustomer(customer.customer)) {
                break;
            }
        }
    }
    if (candidate.empty()) {
        return std::nullopt;
    }
    std::vector<RuinCustomer> insertionOrder = customers;
    std::ranges::sort(insertionOrder, [](const RuinCustomer& left, const RuinCustomer& right) {
        if (left.contribution != right.contribution) {
            return left.contribution > right.contribution;
        }
        return left.sequence < right.sequence;
    });
    for (const RuinCustomer& customer : insertionOrder) {
        if (!InsertCustomerBestRoute(candidate, customer.customer)) {
            return std::nullopt;
        }
    }
    const int candidateCost = TotalRouteCost(candidate);
    if (candidateCost >= originalCost) {
        return std::nullopt;
    }
    return RuinRecreateResult{
        .routes = candidate,
        .improvement = originalCost - candidateCost,
        .sequence = sequence,
    };
}

/** @brief Find the best feasible segment relocation between two route snapshots.
 *
 * The function evaluates all consecutive segments of the requested size in the
 * source route and inserts each segment into the destination route through the
 * normal Route::AddElem feasibility checks. It returns only strict combined-cost
 * improvements, including the source and destination route indexes needed by the
 * caller to apply the move.
 */
std::optional<SegmentRelocation> FindBestSegmentRelocation(const Route& source, const Route& dest, int sourceIndex,
                                                           int destIndex, int segmentSize) {
    if (source.size() < segmentSize + 2) {
        return std::nullopt;
    }
    std::optional<SegmentRelocation> best;
    const int originalCost = source.GetTotalCost() + dest.GetTotalCost();
    RouteList::const_iterator segmentStart = source.GetRoute()->cbegin();
    // Skip the leading depot; customer segments cannot include either depot.
    std::advance(segmentStart, 1);
    for (; segmentStart != source.GetRoute()->cend(); ++segmentStart) {
        std::list<Customer> segment = ReadSegment(segmentStart, source.GetRoute()->cend(), segmentSize);
        if (std::cmp_not_equal(segment.size(), segmentSize) || segment.back() == source.GetRoute()->back().first) {
            // Once the fixed-size segment reaches the trailing depot, later starts are invalid too.
            break;
        }
        Route candidateSource = source;
        Route candidateDest = dest;
        if (!RemoveSegment(candidateSource, segment) || !candidateDest.AddElem(segment)) {
            continue;
        }
        const int candidateCost = ActiveRouteCost(candidateSource) + candidateDest.GetTotalCost();
        const int improvement = originalCost - candidateCost;
        if (improvement > 0 && (!best.has_value() || improvement > best->improvement)) {
            best = SegmentRelocation{
                .source = candidateSource,
                .dest = candidateDest,
                .sourceIndex = sourceIndex,
                .destIndex = destIndex,
                .improvement = improvement,
            };
        }
    }
    return best;
}

/** @brief Find the best feasible exchange of two route segments.
 *
 * Evaluates every source/destination segment pair up to maxSegmentSize and
 * reinserts each exchanged segment in the best feasible position of the other
 * route. This generalizes the fixed 1-1, 1-2, 2-1, and 2-2 exchanges and gives
 * the search a larger route-composition move without mutating shared state.
 */
std::optional<SegmentExchange> FindBestSegmentExchange(const Route& source, const Route& dest, int sourceIndex,
                                                       int destIndex, int maxSegmentSize) {
    std::optional<SegmentExchange> best;
    const int originalCost = source.GetTotalCost() + dest.GetTotalCost();
    for (int sourceSegmentSize = 1; sourceSegmentSize <= maxSegmentSize; ++sourceSegmentSize) {
        if (source.size() < sourceSegmentSize + 2) {
            continue;
        }
        RouteList::const_iterator sourceStart = source.GetRoute()->cbegin();
        std::advance(sourceStart, 1);
        for (; sourceStart != source.GetRoute()->cend(); ++sourceStart) {
            std::list<Customer> sourceSegment = ReadSegment(sourceStart, source.GetRoute()->cend(), sourceSegmentSize);
            if (std::cmp_not_equal(sourceSegment.size(), sourceSegmentSize) ||
                sourceSegment.back() == source.GetRoute()->back().first) {
                // Route segments are contiguous customer-only blocks; never exchange a depot.
                break;
            }
            for (int destSegmentSize = 1; destSegmentSize <= maxSegmentSize; ++destSegmentSize) {
                if (dest.size() < destSegmentSize + 2) {
                    continue;
                }
                RouteList::const_iterator destStart = dest.GetRoute()->cbegin();
                std::advance(destStart, 1);
                for (; destStart != dest.GetRoute()->cend(); ++destStart) {
                    std::list<Customer> destSegment = ReadSegment(destStart, dest.GetRoute()->cend(), destSegmentSize);
                    if (std::cmp_not_equal(destSegment.size(), destSegmentSize) ||
                        destSegment.back() == dest.GetRoute()->back().first) {
                        break;
                    }
                    Route candidateSource = source;
                    Route candidateDest = dest;
                    if (!RemoveSegment(candidateSource, sourceSegment) || !RemoveSegment(candidateDest, destSegment) ||
                        !candidateSource.AddElem(destSegment) || !candidateDest.AddElem(sourceSegment)) {
                        continue;
                    }
                    const int candidateCost = ActiveRouteCost(candidateSource) + ActiveRouteCost(candidateDest);
                    const int improvement = originalCost - candidateCost;
                    if (improvement > 0 && (!best.has_value() || improvement > best->improvement)) {
                        best = SegmentExchange{
                            .source = candidateSource,
                            .dest = candidateDest,
                            .sourceIndex = sourceIndex,
                            .destIndex = destIndex,
                            .improvement = improvement,
                        };
                    }
                }
            }
        }
    }
    return best;
}

/** @brief Remove all void routes. */
void OptimalMove::CleanVoid(Routes& routes) {
    std::erase_if(routes, [](const Route& r) { return r.size() <= 2; });
}

/** @brief Move one customer from each source route to a destination route.
 *
 * This opt function try to move, for every route, a customer
 * from a route to another and removes empty route.
 * @return True if the routes are improved
 */
int OptimalMove::Opt10(Routes& routes, bool force) {
    int diffCost = -1;
    Routes::iterator it = routes.begin();
    std::set<BestResult, decltype(comp)> b(comp);
    bool flag = false;
    // pool of threads
    ThreadPool pool(this->cores);
    for (int i = 0; it != routes.end(); std::advance(it, 1), ++i) {
        Routes::const_iterator jt = routes.cbegin();
        for (int j = 0; jt != routes.cend(); std::advance(jt, 1), ++j) {
            if (jt != it) {
                // create a thread to run Move1FromTo function and save the result in l list
                pool.AddTask([force, it, jt, i, j, &b, &flag, this]() {
                    const int originalCost = it->GetTotalCost() + jt->GetTotalCost();
                    Route tFrom = *it;
                    Route tTo = *jt;
                    // if the move is done and the cost of routes is less than before
                    if (Move1FromTo(tFrom, tTo, force)) {
                        // wait until the lock is unlocked from an other thread, which is terminated
                        std::scoped_lock lock(this->mtx);
                        b.insert(BestResult{.sourceIndex = i,
                                            .destIndex = j,
                                            .originalCost = originalCost,
                                            .source = tFrom,
                                            .dest = tTo});
                        flag = true;
                    }
                });
            }
        }
    }
    // wait to finish all threads
    pool.JoinAll();
    // if some improvements are made update the routes
    if (flag) {
        diffCost = ApplyBestResult(routes, b);
        this->CleanVoid(routes);
        Utils::Instance().logger("opt10 improved: " + std::to_string(diffCost), Utils::VERBOSE);
    } else
        Utils::Instance().logger("opt10 no improvement", Utils::VERBOSE);
    return diffCost;
}

/** @brief Move one customer in the opposite route-pair direction.
 *
 * This opt function try to move, for every route, a customer
 * from a route to another and remove empty route.
 * @param[in] routes The routes to edit
 * @param[in] force  If needs to find the worst combination
 * @return True if the routes are improves
 */
int OptimalMove::Opt01(Routes& routes, bool force) {
    int diffCost = -1;
    Routes::iterator it = routes.begin();
    std::set<BestResult, decltype(comp)> b(comp);
    bool flag = false;
    ThreadPool pool(this->cores);
    for (int i = 0; it != routes.end(); std::advance(it, 1), ++i) {
        Routes::const_iterator jt = routes.cbegin();
        for (int j = 0; jt != routes.cend(); std::advance(jt, 1), ++j) {
            if (jt != it) {
                pool.AddTask([force, it, jt, i, j, &b, &flag, this]() {
                    const int originalCost = it->GetTotalCost() + jt->GetTotalCost();
                    Route tFrom = *it;
                    Route tTo = *jt;
                    if (Move1FromTo(tTo, tFrom, force)) {
                        // wait until the lock is unlocked from an other thread, which is terminated
                        std::scoped_lock lock(this->mtx);
                        b.insert(BestResult{.sourceIndex = i,
                                            .destIndex = j,
                                            .originalCost = originalCost,
                                            .source = tFrom,
                                            .dest = tTo});
                        flag = true;
                    }
                });
            }
        }
    }
    pool.JoinAll();
    if (flag) {
        diffCost = ApplyBestResult(routes, b);
        this->CleanVoid(routes);
        Utils::Instance().logger("opt01 improved: " + std::to_string(diffCost), Utils::VERBOSE);
    } else
        Utils::Instance().logger("opt01 no improvement", Utils::VERBOSE);
    return diffCost;
}

/** @brief Move one customer from source to destination.
 *
 * This function tries to move a customer from a route to another trying
 * in every possible position if and only if the movement results in
 * an improvement of the total cost in both routes.
 * @param[in] source Route where to choose a random customer
 * @param[in] dest Route destination
 * @param[in] force Force the movement
 * @return True if the customer is moved.
 */
bool OptimalMove::Move1FromTo(Route& source, Route& dest, bool force) {
    bool ret = false;
    int bestCost = source.GetTotalCost() + dest.GetTotalCost();
    Route bestDestRoute = dest, bestSourceRoute = source;
    RouteList::iterator itSource = source.GetRoute()->begin();
    // cannot move the depot
    std::advance(itSource, 1);
    // for each position in source route
    for (unsigned i = 1; i < (source.GetRoute()->size() - 1); ++itSource, ++i) {
        // copy the destination route to try some path configuration
        Route tempDest = dest;
        Route tempSource = source;
        if (tempSource.RemoveCustomer(itSource->first) && tempDest.AddElem(itSource->first)) {
            const int candidateCost = tempSource.GetTotalCost() + tempDest.GetTotalCost();
            if ((!force && candidateCost < bestCost) || (force && candidateCost > bestCost)) {
                bestCost = candidateCost;
                bestSourceRoute = tempSource;
                bestDestRoute = tempDest;
                ret = true;
            }
        }
    }
    // if the route is better than before, update
    if (ret) {
        source = bestSourceRoute;
        dest = bestDestRoute;
    }
    return ret;
}

/** @brief Swap one customer between route pairs.
 *
 * This opt function try to swap, for every route, a random customer
 * from a route with another random customer from the next.
 * @param[in] routes The routes to edit
 * @param[in] force  If needs to find the worst combination
 * @return True if the routes are improves
 */
int OptimalMove::Opt11(Routes& routes, bool force) {
    int diffCost = -1;
    Routes::iterator it = routes.begin();
    std::set<BestResult, decltype(comp)> b(comp);
    bool flag = false;
    ThreadPool pool(this->cores);
    for (int i = 0; it != routes.end(); std::advance(it, 1), ++i) {
        Routes::const_iterator jt = routes.cbegin();
        for (int j = 0; jt != routes.cend(); std::advance(jt, 1), ++j) {
            if (jt != it) {
                pool.AddTask([force, it, jt, i, j, &b, &flag, this]() {
                    const int originalCost = it->GetTotalCost() + jt->GetTotalCost();
                    Route tFrom = *it;
                    Route tTo = *jt;
                    if (SwapFromTo(tFrom, tTo, force)) {
                        // wait until the lock is unlocked from an other thread, which is terminated
                        std::scoped_lock lock(this->mtx);
                        b.insert(BestResult{.sourceIndex = i,
                                            .destIndex = j,
                                            .originalCost = originalCost,
                                            .source = tFrom,
                                            .dest = tTo});
                        flag = true;
                    }
                });
            }
        }
    }
    pool.JoinAll();
    if (flag) {
        diffCost = ApplyBestResult(routes, b);
        this->CleanVoid(routes);
        Utils::Instance().logger("opt11 improved: " + std::to_string(diffCost), Utils::VERBOSE);
    } else
        Utils::Instance().logger("opt11 no improvement", Utils::VERBOSE);
    return diffCost;
}

/** @brief Swap the best feasible customer pair between two routes.
 *
 * This function swap two customers from two routes.
 * @param[in] source First route
 * @param[in] dest   Second route
 * @param[in] force  If needs to find the worst combination
 * @return True is the swap is successful
 */
bool OptimalMove::SwapFromTo(Route& source, Route& dest, bool force) {
    bool ret = false;
    int bestCost = source.GetTotalCost() + dest.GetTotalCost();
    Route bestRouteSource = source, bestRouteDest = dest;
    RouteList::iterator itSource = source.GetRoute()->begin();
    // cannot move the depot (start)
    std::advance(itSource, 1);
    // for each customer in the source route try to move it to the next route
    for (unsigned i = 1; i < (source.GetRoute()->size() - 1); std::advance(itSource, 1), i++) {
        RouteList::iterator itDest = dest.GetRoute()->begin();
        std::advance(itDest, 1);
        // cannot move the depot (end)
        for (unsigned j = 1; j < (dest.GetRoute()->size() - 1); std::advance(itDest, 1), j++) {
            // copy the destination route to try some path configuration
            Route tempDest = dest;
            // copy the source route to check out if this configuration is valid and better
            Route tempSource = source;
            if (tempSource.RemoveCustomer(itSource->first) && tempDest.RemoveCustomer(itDest->first) &&
                tempSource.AddElem(itDest->first) && tempDest.AddElem(itSource->first)) {
                const int candidateCost = tempSource.GetTotalCost() + tempDest.GetTotalCost();
                if ((!force && candidateCost < bestCost) || (force && candidateCost > bestCost)) {
                    bestCost = candidateCost;
                    bestRouteDest = tempDest;
                    bestRouteSource = tempSource;
                    ret = true;
                }
            }
        }
    }
    // if the routes are better than before, update
    if (ret) {
        // update the routes
        dest = bestRouteDest;
        source = bestRouteSource;
    }
    return ret;
}

/** @brief Move one customer from source while removing two from destination.
 *
 * This function swap two customers from the routes and moves one
 * customer from the second to the first route.
 * @param[in] routes The routes to edit
 * @param[in] force  If needs to find the worst combination
 * @return True if the routes are improves
 */
int OptimalMove::Opt12(Routes& routes, bool force) {
    int diffCost = -1;
    Routes::iterator it = routes.begin();
    std::set<BestResult, decltype(comp)> b(comp);
    bool flag = false;
    ThreadPool pool(this->cores);
    for (int i = 0; it != routes.end(); std::advance(it, 1), ++i) {
        Routes::const_iterator jt = routes.cbegin();
        for (int j = 0; jt != routes.cend(); std::advance(jt, 1), ++j) {
            if (jt != it) {
                pool.AddTask([force, it, jt, i, j, &b, &flag, this]() {
                    const int originalCost = it->GetTotalCost() + jt->GetTotalCost();
                    Route tFrom = *it;
                    Route tTo = *jt;
                    if (AddRemoveFromTo(tFrom, tTo, 1, 2, force)) {
                        // wait until the lock is unlocked from an other thread, which is terminated
                        std::scoped_lock lock(this->mtx);
                        b.insert(BestResult{.sourceIndex = i,
                                            .destIndex = j,
                                            .originalCost = originalCost,
                                            .source = tFrom,
                                            .dest = tTo});
                        flag = true;
                    }
                });
            }
        }
    }
    pool.JoinAll();
    if (flag) {
        diffCost = ApplyBestResult(routes, b);
        this->CleanVoid(routes);
        Utils::Instance().logger("opt12 improved: " + std::to_string(diffCost), Utils::VERBOSE);
    } else
        Utils::Instance().logger("opt12 no improvement", Utils::VERBOSE);
    return diffCost;
}

/** @brief Exchange different-sized customer groups between two routes.
 *
 * This function moves a number of customers from two routes and find the best
 * move checking all possible route configurations.
 * @param[in] source Route where to choose a random customer
 * @param[in] dest Route destination
 * @param[in] nInsert Number of customer to move from 'source' to 'dest'
 * @param[in] nRemove Number of customer to move from 'dest' to 'source'
 * @param[in] force  If needs to find the worst combination
 * @return True if the customer is moved.
 */
bool OptimalMove::AddRemoveFromTo(Route& source, Route& dest, int nInsert, int nRemove, bool force) {
    bool bestFound = false;
    int bestCost = source.GetTotalCost() + dest.GetTotalCost();
    std::pair<Route, Route> bests = std::make_pair(source, dest);

    const std::vector<std::list<Customer>> sourceCombinations = BuildCustomerCombinations(source, nInsert);
    const std::vector<std::list<Customer>> destCombinations = BuildCustomerCombinations(dest, nRemove);
    for (const std::list<Customer>& custsInsert : sourceCombinations) {
        for (const std::list<Customer>& custsRemove : destCombinations) {
            Route tempFrom = source;
            Route tempTo = dest;
            if (!RemoveSegment(tempFrom, custsInsert) || !RemoveSegment(tempTo, custsRemove) ||
                !tempFrom.AddElem(custsRemove) || !tempTo.AddElem(custsInsert)) {
                continue;
            }
            const int candidateCost = tempFrom.GetTotalCost() + tempTo.GetTotalCost();
            if ((!force && candidateCost < bestCost) || (force && candidateCost > bestCost)) {
                bestFound = true;
                bestCost = candidateCost;
                std::get<0>(bests) = tempFrom;
                std::get<1>(bests) = tempTo;
            }
        }
    }
    if (bestFound) {
        source = std::get<0>(bests);
        dest = std::get<1>(bests);
    }
    return bestFound;
}

/** @brief Move two customers from source while removing one from destination.
 *
 * This function swap one customers from each routes and moves one
 * customer from the first to the second.
 * @param[in] routes The routes to edit
 * @param[in] force  If needs to find the worst combination
 * @return True if the routes are improves
 */
int OptimalMove::Opt21(Routes& routes, bool force) {
    int diffCost = -1;
    Routes::iterator it = routes.begin();
    std::set<BestResult, decltype(comp)> b(comp);
    bool flag = false;
    ThreadPool pool(this->cores);
    for (int i = 0; it != routes.end(); std::advance(it, 1), ++i) {
        Routes::const_iterator jt = routes.cbegin();
        for (int j = 0; jt != routes.cend(); std::advance(jt, 1), ++j) {
            if (jt != it) {
                pool.AddTask([force, it, jt, i, j, &b, &flag, this]() {
                    const int originalCost = it->GetTotalCost() + jt->GetTotalCost();
                    Route tFrom = *it;
                    Route tTo = *jt;
                    if (AddRemoveFromTo(tFrom, tTo, 2, 1, force)) {
                        // wait until the lock is unlocked from an other thread, which is terminated
                        std::scoped_lock lock(this->mtx);
                        b.insert(BestResult{.sourceIndex = i,
                                            .destIndex = j,
                                            .originalCost = originalCost,
                                            .source = tFrom,
                                            .dest = tTo});
                        flag = true;
                    }
                });
            }
        }
    }
    pool.JoinAll();
    if (flag) {
        diffCost = ApplyBestResult(routes, b);
        this->CleanVoid(routes);
        Utils::Instance().logger("opt21 improved: " + std::to_string(diffCost), Utils::VERBOSE);
    } else
        Utils::Instance().logger("opt21 no improvement", Utils::VERBOSE);
    return diffCost;
}

/** @brief Swap two customers from each route in every route pair.
 *
 * This function swaps two customers from the first route
 * with two customers from the second.
 * @param[in] routes The routes to edit
 * @param[in] force  If needs to find the worst combination
 * @return True if the routes are improved
 */
int OptimalMove::Opt22(Routes& routes, bool force) {
    int diffCost = -1;
    Routes::iterator it = routes.begin();
    std::set<BestResult, decltype(comp)> b(comp);
    bool flag = false;
    ThreadPool pool(this->cores);
    for (int i = 0; it != routes.end(); std::advance(it, 1), ++i) {
        Routes::const_iterator jt = routes.cbegin();
        for (int j = 0; jt != routes.cend(); std::advance(jt, 1), ++j) {
            if (jt != it) {
                pool.AddTask([force, it, jt, i, j, &b, &flag, this]() {
                    const int originalCost = it->GetTotalCost() + jt->GetTotalCost();
                    Route tFrom = *it;
                    Route tTo = *jt;
                    if (AddRemoveFromTo(tFrom, tTo, 2, 2, force)) {
                        // wait until the lock is unlocked from an other thread, which is terminated
                        std::scoped_lock lock(this->mtx);
                        b.insert(BestResult{.sourceIndex = i,
                                            .destIndex = j,
                                            .originalCost = originalCost,
                                            .source = tFrom,
                                            .dest = tTo});
                        flag = true;
                    }
                });
            }
        }
    }
    pool.JoinAll();
    if (flag) {
        diffCost = ApplyBestResult(routes, b);
        this->CleanVoid(routes);
        Utils::Instance().logger("opt22 improved: " + std::to_string(diffCost), Utils::VERBOSE);
    } else
        Utils::Instance().logger("opt22 no improvement", Utils::VERBOSE);
    return diffCost;
}

/** @brief Exchange variable-length segments between routes.
 *
 * Searches all route pairs for the best strict improvement produced by
 * exchanging consecutive customer segments up to maxSegmentSize. The expensive
 * per-pair scans run in parallel and the best result is applied once all
 * workers finish.
 * @param[in] routes The routes to edit
 * @param[in] maxSegmentSize Maximum consecutive customers per exchanged segment
 * @return Positive cost reduction if routes are improved, otherwise -1
 */
int OptimalMove::OptSwapSegments(Routes& routes, int maxSegmentSize) {
    if (maxSegmentSize <= 0) {
        Utils::Instance().logger("segment exchange no improvement", Utils::VERBOSE);
        return -1;
    }
    const std::vector<RouteSnapshot> snapshots = SnapshotRoutes(routes);
    std::vector<SegmentExchange> candidates;
    std::mutex candidatesMutex;
    ThreadPool pool(this->cores);
    for (std::size_t sourceIndex = 0; sourceIndex < snapshots.size(); ++sourceIndex) {
        for (std::size_t destIndex = sourceIndex + 1; destIndex < snapshots.size(); ++destIndex) {
            pool.AddTask([&snapshots, sourceIndex, destIndex, maxSegmentSize, &candidates, &candidatesMutex]() {
                std::optional<SegmentExchange> candidate =
                    FindBestSegmentExchange(snapshots[sourceIndex].route, snapshots[destIndex].route,
                                            snapshots[sourceIndex].index, snapshots[destIndex].index, maxSegmentSize);
                if (!candidate.has_value()) {
                    return;
                }
                std::scoped_lock lock(candidatesMutex);
                candidates.push_back(std::move(*candidate));
            });
        }
    }
    pool.JoinAll();
    const auto best =
        std::ranges::max_element(candidates, [](const SegmentExchange& left, const SegmentExchange& right) {
            if (left.improvement != right.improvement) {
                return left.improvement < right.improvement;
            }
            if (left.sourceIndex != right.sourceIndex) {
                return left.sourceIndex > right.sourceIndex;
            }
            return left.destIndex > right.destIndex;
        });
    if (best == candidates.end()) {
        Utils::Instance().logger("segment exchange no improvement", Utils::VERBOSE);
        return -1;
    }
    Routes::iterator source = routes.begin();
    std::advance(source, best->sourceIndex);
    *source = best->source;
    Routes::iterator dest = routes.begin();
    std::advance(dest, best->destIndex);
    *dest = best->dest;
    this->CleanVoid(routes);
    Utils::Instance().logger("segment exchange improved: " + std::to_string(best->improvement), Utils::VERBOSE);
    return best->improvement;
}

/** @brief Remove and greedily reinsert high-impact customers.
 *
 * Builds combinations from the highest adjacent-edge-contribution customers,
 * evaluates each combination on a route copy in parallel, and applies the best
 * strict total-cost improvement. This is a small deterministic large-neighborhood
 * move that can cross local minima which one-customer moves cannot.
 * @param[in] routes The routes to edit
 * @param[in] removalCount Number of customers removed together
 * @param[in] candidateLimit Number of high-contribution customers considered
 * @return Positive cost reduction if routes are improved, otherwise -1
 */
int OptimalMove::OptRuinRecreate(Routes& routes, int removalCount, int candidateLimit) {
    if (removalCount <= 0 || candidateLimit <= 0) {
        Utils::Instance().logger("ruin-recreate no improvement", Utils::VERBOSE);
        return -1;
    }
    const std::vector<RuinCustomer> ruinCustomers = CollectRuinCustomers(routes, candidateLimit);
    if (std::cmp_less(ruinCustomers.size(), removalCount)) {
        Utils::Instance().logger("ruin-recreate no improvement", Utils::VERBOSE);
        return -1;
    }
    const std::vector<std::vector<RuinCustomer>> combinations = BuildRuinCombinations(ruinCustomers, removalCount);
    std::vector<RuinRecreateResult> candidates;
    std::mutex candidatesMutex;
    ThreadPool pool(this->cores);
    for (std::size_t i = 0; i < combinations.size(); ++i) {
        pool.AddTask([&routes, &combinations, i, &candidates, &candidatesMutex]() {
            std::optional<RuinRecreateResult> candidate = EvaluateRuinRecreate(routes, combinations[i], i);
            if (!candidate.has_value()) {
                return;
            }
            std::scoped_lock lock(candidatesMutex);
            candidates.push_back(std::move(*candidate));
        });
    }
    pool.JoinAll();
    const auto best =
        std::ranges::max_element(candidates, [](const RuinRecreateResult& left, const RuinRecreateResult& right) {
            if (left.improvement != right.improvement) {
                return left.improvement < right.improvement;
            }
            return left.sequence > right.sequence;
        });
    if (best == candidates.end()) {
        Utils::Instance().logger("ruin-recreate no improvement", Utils::VERBOSE);
        return -1;
    }
    routes = best->routes;
    this->CleanVoid(routes);
    Utils::Instance().logger("ruin-recreate improved: " + std::to_string(best->improvement), Utils::VERBOSE);
    return best->improvement;
}

/** @brief Exchange route tails between two routes.
 *
 * This opt function cuts two routes and swaps their suffixes, keeping the best
 * feasible combined-cost improvement.
 * @param[in] routes The routes to edit
 * @return Positive cost reduction if routes are improved, otherwise -1
 */
int OptimalMove::Opt2Star(Routes& routes) {
    const std::vector<RouteSnapshot> snapshots = SnapshotRoutes(routes);
    std::vector<IndexedTwoOptStarRoutes> candidates;
    std::mutex candidatesMutex;
    ThreadPool pool(this->cores);
    for (std::size_t sourceIndex = 0; sourceIndex < snapshots.size(); ++sourceIndex) {
        for (std::size_t destIndex = sourceIndex + 1; destIndex < snapshots.size(); ++destIndex) {
            pool.AddTask([&snapshots, sourceIndex, destIndex, &candidates, &candidatesMutex]() {
                const std::optional<TwoOptStarRoutes> candidate =
                    FindBestTwoOptStarRoutes(snapshots[sourceIndex].route, snapshots[destIndex].route);
                if (!candidate.has_value()) {
                    return;
                }
                std::scoped_lock lock(candidatesMutex);
                candidates.emplace_back(IndexedTwoOptStarRoutes{
                    .source = candidate->source,
                    .dest = candidate->dest,
                    .sourceIndex = snapshots[sourceIndex].index,
                    .destIndex = snapshots[destIndex].index,
                    .improvement = candidate->improvement,
                });
            });
        }
    }
    pool.JoinAll();
    const auto best = std::ranges::max_element(
        candidates, [](const IndexedTwoOptStarRoutes& left, const IndexedTwoOptStarRoutes& right) {
            if (left.improvement != right.improvement) {
                return left.improvement < right.improvement;
            }
            // Lower route indexes win ties, matching deterministic serial scan order.
            if (left.sourceIndex != right.sourceIndex) {
                return left.sourceIndex > right.sourceIndex;
            }
            return left.destIndex > right.destIndex;
        });
    if (best == candidates.end()) {
        Utils::Instance().logger("2-Opt* no improvement", Utils::VERBOSE);
        return -1;
    }
    Routes::iterator source = routes.begin();
    std::advance(source, best->sourceIndex);
    *source = best->source;
    Routes::iterator dest = routes.begin();
    std::advance(dest, best->destIndex);
    *dest = best->dest;
    this->CleanVoid(routes);
    Utils::Instance().logger("2-Opt* improved: " + std::to_string(best->improvement), Utils::VERBOSE);
    return best->improvement;
}

/** @brief Repartition route pairs with exact route ordering.
 *
 * For each bounded route pair, this pools both customer sets, evaluates every
 * capacity-feasible bipartition, and rebuilds the pair with exact per-route TSP
 * ordering. It is intentionally separate from the small exchange moves so the
 * expensive dynamic program is paid once per route pair instead of once per
 * tiny candidate exchange.
 * @param[in] routes The routes to edit
 * @param[in] maxCombinedCustomers Maximum customers allowed across the two routes
 * @return Positive cost reduction if routes are improved, otherwise -1
 */
int OptimalMove::OptPairSplit(Routes& routes, int maxCombinedCustomers) {
    if (maxCombinedCustomers <= 2) {
        Utils::Instance().logger("pair split no improvement", Utils::VERBOSE);
        return -1;
    }
    const std::vector<RouteSnapshot> snapshots = SnapshotRoutes(routes);
    std::vector<PairSplitCandidatePair> pairCandidates;
    pairCandidates.reserve((snapshots.size() * snapshots.size()) / 2);
    for (std::size_t sourceIndex = 0; sourceIndex < snapshots.size(); ++sourceIndex) {
        for (std::size_t destIndex = sourceIndex + 1; destIndex < snapshots.size(); ++destIndex) {
            const int combinedCustomers = snapshots[sourceIndex].route.size() + snapshots[destIndex].route.size() - 4;
            if (combinedCustomers > maxCombinedCustomers) {
                continue;
            }
            pairCandidates.push_back(PairSplitCandidatePair{
                .sourceIndex = sourceIndex,
                .destIndex = destIndex,
                .distance = snapshots[sourceIndex].route.GetDistanceFrom(snapshots[destIndex].route),
                .combinedCost = snapshots[sourceIndex].route.GetTotalCost() + snapshots[destIndex].route.GetTotalCost(),
            });
        }
    }
    std::ranges::sort(pairCandidates, [](const PairSplitCandidatePair& left, const PairSplitCandidatePair& right) {
        if (left.distance != right.distance) {
            return left.distance < right.distance;
        }
        if (left.combinedCost != right.combinedCost) {
            return left.combinedCost > right.combinedCost;
        }
        if (left.sourceIndex != right.sourceIndex) {
            return left.sourceIndex < right.sourceIndex;
        }
        return left.destIndex < right.destIndex;
    });
    const std::size_t pairLimit = std::min(pairCandidates.size(), std::max<std::size_t>(16, snapshots.size() * 2));
    pairCandidates.resize(pairLimit);
    std::vector<PairSplit> candidates;
    std::mutex candidatesMutex;
    ThreadPool pool(this->cores);
    for (const PairSplitCandidatePair& pair : pairCandidates) {
        pool.AddTask([&snapshots, pair, maxCombinedCustomers, &candidates, &candidatesMutex]() {
            std::optional<PairSplit> candidate = FindBestPairSplit(
                snapshots[pair.sourceIndex].route, snapshots[pair.destIndex].route, snapshots[pair.sourceIndex].index,
                snapshots[pair.destIndex].index, maxCombinedCustomers);
            if (!candidate.has_value()) {
                return;
            }
            std::scoped_lock lock(candidatesMutex);
            candidates.push_back(std::move(*candidate));
        });
    }
    pool.JoinAll();
    const auto best = std::ranges::max_element(candidates, [](const PairSplit& left, const PairSplit& right) {
        if (left.improvement != right.improvement) {
            return left.improvement < right.improvement;
        }
        if (left.sourceIndex != right.sourceIndex) {
            return left.sourceIndex > right.sourceIndex;
        }
        return left.destIndex > right.destIndex;
    });
    if (best == candidates.end()) {
        Utils::Instance().logger("pair split no improvement", Utils::VERBOSE);
        return -1;
    }
    Routes::iterator source = routes.begin();
    std::advance(source, best->sourceIndex);
    *source = best->source;
    Routes::iterator dest = routes.begin();
    std::advance(dest, best->destIndex);
    *dest = best->dest;
    this->CleanVoid(routes);
    Utils::Instance().logger("pair split improved: " + std::to_string(best->improvement), Utils::VERBOSE);
    return best->improvement;
}

/** @brief Move a consecutive segment from one route to another.
 *
 * Tries every segment of the requested size in every source route and inserts
 * it into the best feasible destination route position.
 * @param[in] routes The routes to edit
 * @param[in] segmentSize Number of consecutive customers to move
 * @return Positive cost reduction if routes are improved, otherwise -1
 */
int OptimalMove::OptRelocateSegment(Routes& routes, int segmentSize) {
    if (segmentSize <= 0) {
        Utils::Instance().logger("segment relocate no improvement", Utils::VERBOSE);
        return -1;
    }
    const std::vector<RouteSnapshot> snapshots = SnapshotRoutes(routes);
    std::vector<SegmentRelocation> candidates;
    std::mutex candidatesMutex;
    ThreadPool pool(this->cores);
    for (std::size_t sourceIndex = 0; sourceIndex < snapshots.size(); ++sourceIndex) {
        for (std::size_t destIndex = 0; destIndex < snapshots.size(); ++destIndex) {
            if (sourceIndex == destIndex) {
                continue;
            }
            pool.AddTask([&snapshots, sourceIndex, destIndex, segmentSize, &candidates, &candidatesMutex]() {
                std::optional<SegmentRelocation> candidate =
                    FindBestSegmentRelocation(snapshots[sourceIndex].route, snapshots[destIndex].route,
                                              snapshots[sourceIndex].index, snapshots[destIndex].index, segmentSize);
                if (!candidate.has_value()) {
                    return;
                }
                std::scoped_lock lock(candidatesMutex);
                candidates.push_back(std::move(*candidate));
            });
        }
    }
    pool.JoinAll();
    const auto best =
        std::ranges::max_element(candidates, [](const SegmentRelocation& left, const SegmentRelocation& right) {
            if (left.improvement != right.improvement) {
                return left.improvement < right.improvement;
            }
            if (left.sourceIndex != right.sourceIndex) {
                return left.sourceIndex > right.sourceIndex;
            }
            return left.destIndex > right.destIndex;
        });
    if (best == candidates.end()) {
        Utils::Instance().logger("segment relocate no improvement", Utils::VERBOSE);
        return -1;
    }
    Routes::iterator source = routes.begin();
    std::advance(source, best->sourceIndex);
    *source = best->source;
    Routes::iterator dest = routes.begin();
    std::advance(dest, best->destIndex);
    *dest = best->dest;
    this->CleanVoid(routes);
    Utils::Instance().logger("segment relocate improved: " + std::to_string(best->improvement), Utils::VERBOSE);
    return best->improvement;
}

/** @brief Solve small route-ordering subproblems exactly.
 *
 * For each route with at most maxCustomers non-depot customers, runs a
 * Held-Karp dynamic program to find the minimum-cost customer order for that
 * fixed route membership. The route is rebuilt only when the exact order is a
 * strict improvement.
 * @param[in] routes The routes to edit
 * @param[in] maxCustomers Maximum non-depot route size to optimize exactly
 * @return Positive total cost reduction if any route is improved, otherwise -1
 */
int OptimalMove::OptRouteTsp(Routes& routes, int maxCustomers) {
    int totalImprovement = 0;
    for (Route& route : routes) {
        totalImprovement += OptimizeSingleRouteTsp(route, maxCustomers);
    }
    if (totalImprovement <= 0) {
        Utils::Instance().logger("route TSP no improvement", Utils::VERBOSE);
        return -1;
    }
    Utils::Instance().logger("route TSP improved: " + std::to_string(totalImprovement), Utils::VERBOSE);
    return totalImprovement;
}

/** @brief Try to balance the routes.
 *
 * Find route with one or two customers and move it/them to another route:
 * execute an Opt10 to balance the route and get more occupancy.
 * @param[in] routes The routes to edit
 */
void OptimalMove::RouteBalancer(Routes& routes) {
    Routes temp = routes;
    bool stop = false;
    while (!stop) {
        bool reset = false;
        Routes::iterator it = temp.begin();
        for (int i = 0; it != temp.end() && !reset; ++it, i++) {
            if (it->size() >= 3 && it->size() <= 4) {
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
}

/** @brief Remove routes that can be fully inserted into other routes.
 *
 * Targets the smallest route first and deletes it only when all of its
 * customers can be inserted into another feasible route.
 * @param[in] routes The routes to edit
 * @return Number of routes removed
 */
int OptimalMove::ReduceRoutes(Routes& routes) {
    int removedRoutes = 0;
    bool changed = true;
    while (changed) {
        changed = false;
        Routes::iterator source = routes.end();
        for (Routes::iterator it = routes.begin(); it != routes.end(); ++it) {
            if (it->size() > 2 && (source == routes.end() || it->size() < source->size())) {
                source = it;
            }
        }
        if (source == routes.end()) {
            break;
        }
        const int originalCost = source->GetTotalCost();
        const std::list<Customer> sourceCustomers = RouteCustomersWithoutDepot(*source);
        Routes::iterator bestDest = routes.end();
        Route bestDestRoute = *source;
        int bestCostIncrease = 0;
        bool found = false;
        for (Routes::iterator dest = routes.begin(); dest != routes.end(); ++dest) {
            if (dest == source) {
                continue;
            }
            Route candidateDest = *dest;
            bool feasible = true;
            for (const Customer& customer : sourceCustomers) {
                if (!candidateDest.AddElem(customer)) {
                    feasible = false;
                    break;
                }
            }
            if (feasible) {
                const int costIncrease = candidateDest.GetTotalCost() - dest->GetTotalCost();
                if (!found || costIncrease < bestCostIncrease) {
                    bestDest = dest;
                    bestDestRoute = candidateDest;
                    bestCostIncrease = costIncrease;
                    found = true;
                }
            }
        }
        if (found && bestDest != routes.end() && bestCostIncrease <= originalCost) {
            *bestDest = bestDestRoute;
            routes.erase(source);
            ++removedRoutes;
            changed = true;
        }
    }
    if (removedRoutes > 0) {
        Utils::Instance().logger("Route reduction removed: " + std::to_string(removedRoutes), Utils::VERBOSE);
    } else {
        Utils::Instance().logger("Route reduction no improvement", Utils::VERBOSE);
    }
    return removedRoutes;
}

/** @brief Reorder route customers with intra-route 2-opt.
 *
 * For every route swap two customer with distance lower than the average distance
 * of the route and save the best solution.
 * @param[in] routes The routes to edit
 * @return True if routes are improved
 */
bool OptimalMove::Opt2(Routes& routes) {
    bool ret = false;
    Routes::iterator it = routes.begin();
    int diffCost = 0;
    // for each route
    for (; it != routes.end(); ++it) {
        bool routeImproved = false;
        Route bestRoute = *it;
        ThreadPool pool(this->cores);
        int bestCost = it->GetTotalCost();
        RouteList::iterator i = it->GetRoute()->begin();
        std::advance(i, 1);
        for (; i->first != it->GetRoute()->back().first; ++i) {
            RouteList::iterator k = i;
            for (++k; k->first != it->GetRoute()->back().first; ++k) {
                pool.AddTask([i, k, it, &bestCost, &bestRoute, &routeImproved, this]() {
                    // swap customers
                    Route tempRoute = this->Opt2Swap(*it, i->first, k->first);
                    std::scoped_lock lock(this->mtx);
                    if (tempRoute.GetTotalCost() < bestCost) {
                        bestCost = tempRoute.GetTotalCost();
                        bestRoute = tempRoute;
                        routeImproved = true;
                    }
                });
            }
        }
        pool.JoinAll();
        if (routeImproved) {
            diffCost += it->GetTotalCost();
            *it = bestRoute;
            diffCost -= bestRoute.GetTotalCost();
            ret = true;
        }
    }
    if (diffCost != 0) {
        Utils::Instance().logger("2-Opt improved: " + std::to_string(diffCost), Utils::VERBOSE);
    } else {
        ret = false;
        Utils::Instance().logger("2-Opt no improvement", Utils::VERBOSE);
    }
    return ret;
}

/** @brief Reverse the route segment between two customers.
 *
 * This function swap two customers in a route.
 * @param[in] route The route to work with
 * @param[in] i The first customer to swap
 * @param[in] k The second customer to swap
 * @return The new route with customers swapped
 */
Route OptimalMove::Opt2Swap(Route route, const Customer& i, const Customer& k) {
    std::list<Customer> cust;
    Route tempRoute = route;
    RouteList::const_iterator it = tempRoute.GetRoute()->cbegin();
    // from start to i-1
    while (it->first != i) {
        cust.push_back(it->first);
        ++it;
    }
    it = tempRoute.GetRoute()->cend();
    while (it == tempRoute.GetRoute()->cend() || it->first != k) {
        --it;
    }
    // from i to k in reverse order
    while (it->first != i) {
        cust.push_back(it->first);
        --it;
    }
    // push i
    cust.push_back(i);
    // from k+1 to end
    it = route.GetRoute()->begin();
    while (it->first != k) {
        ++it;
    }
    ++it;
    while (it != route.GetRoute()->cend()) {
        cust.push_back(it->first);
        ++it;
    }
    // rebuild route
    Route ret = route;
    if (static_cast<unsigned>(tempRoute.size()) == cust.size() && tempRoute.RebuildRoute(cust))
        ret = tempRoute;
    return ret;
}

/** @brief Reorder route customers with intra-route 3-opt.
 *
 * For every route swap two customer with distance lower than the average distance
 * (the route crosses over itself) of the route and save the best solution.
 * @param[in] routes The routes to edit
 * @return True if routes are improved
 */
bool OptimalMove::Opt3(Routes& routes) {
    bool ret = false;
    Routes::iterator it = routes.begin();
    // Customers with short path (less than average)
    int diffCost = 0;
    // for each route
    for (; it != routes.end(); ++it) {
        bool routeImproved = false;
        Route bestRoute = *it;
        ThreadPool pool(this->cores);
        int bestCost = it->GetTotalCost();
        RouteList::iterator i = it->GetRoute()->begin();
        Customer depot = i->first;
        std::advance(i, it->GetRoute()->size() - 2);
        Customer lastK = i->first;
        std::advance(i, -1);
        Customer lastI = i->first;
        i = it->GetRoute()->begin();
        for (; i->first != lastI; ++i) {
            if (i->first != depot) {
                RouteList::iterator k = i;
                for (++k; k->first != lastK; ++k) {
                    if (k->first != depot) {
                        RouteList::iterator l = k;
                        for (++l; l != it->GetRoute()->end(); ++l) {
                            if (l->first != depot) {
                                RouteList::iterator m = l;
                                for (++m; m != it->GetRoute()->end(); ++m) {
                                    pool.AddTask([i, k, l, m, it, &bestCost, &bestRoute, &routeImproved, this]() {
                                        // swap customers
                                        Route tempRoute = this->Opt3Swap(*it, i->first, k->first, l->first, m->first);
                                        std::scoped_lock lock(this->mtx);
                                        if (tempRoute.GetTotalCost() < bestCost) {
                                            bestCost = tempRoute.GetTotalCost();
                                            bestRoute = tempRoute;
                                            routeImproved = true;
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
        if (routeImproved) {
            diffCost += it->GetTotalCost();
            *it = bestRoute;
            diffCost -= bestRoute.GetTotalCost();
            ret = true;
        }
    }
    if (ret && diffCost != 0) {
        Utils::Instance().logger("3-Opt improved: " + std::to_string(diffCost), Utils::VERBOSE);
    } else {
        ret = false;
        Utils::Instance().logger("3-Opt no improvement", Utils::VERBOSE);
    }
    return ret;
}

/** @brief Apply one 3-opt reconnection pattern around four customers.
 *
 * This function swap two customers in a route.
 * @param[in] route The route to work with
 * @param[in] i The first customer to swap
 * @param[in] k The second customer to swap
 * @param[in] l The third customer to swap
 * @param[in] m The fourth customer to swap
 * @return The new route with customers swapped
 */
Route OptimalMove::Opt3Swap(Route route, const Customer& i, const Customer& k, const Customer& l, const Customer& m) {
    std::list<Customer> cust;
    Route tempRoute = route;
    RouteList::const_iterator it = tempRoute.GetRoute()->cbegin();
    // from start to i-1
    while (it->first != i) {
        cust.push_back(it->first);
        ++it;
    }
    // from i to k in reverse order
    it = tempRoute.GetRoute()->cend();
    while (it == tempRoute.GetRoute()->cend() || it->first != k) {
        --it;
    }
    while (it->first != i) {
        cust.push_back(it->first);
        --it;
    }
    // push i
    cust.push_back(i);
    // from k+1 to l-1
    while (it->first != k)
        ++it;
    ++it;
    while (it->first != l) {
        if (it != tempRoute.GetRoute()->cend())
            cust.push_back(it->first);
        ++it;
    }
    // from l to m in reverse order
    it = tempRoute.GetRoute()->cend();
    while (it == tempRoute.GetRoute()->cend() || it->first != m) {
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
    while (it->first != m && it != tempRoute.GetRoute()->cend()) {
        ++it;
    }
    if (it != tempRoute.GetRoute()->cend())
        ++it;
    while (it != route.GetRoute()->cend()) {
        cust.push_back(it->first);
        ++it;
    }
    // rebuild route
    Route ret = route;
    if (static_cast<unsigned>(tempRoute.size()) == cust.size() && tempRoute.RebuildRoute(cust))
        ret = tempRoute;
    return ret;
}
