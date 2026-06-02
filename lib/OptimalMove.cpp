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
#include <array>
#include <bit>
#include <cmath>
#include <iterator>
#include <limits>
#include <map>
#include <optional>
#include <ranges>
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

/** @brief Circular sweep split candidate before expensive route reordering. */
struct PairSweepOrderCandidate {
    std::vector<Customer> first;
    std::vector<Customer> second;
    int estimatedCost;
};

/** @brief Boundary customer considered movable in a large route-pair split. */
struct BoundaryCustomer {
    Customer customer;
    bool fromSource;
    int crossDistance;
    int contribution;
};

/** @brief Best bounded boundary-customer reassignment between two routes. */
struct BoundaryPairSplit {
    Route source;
    Route dest;
    int sourceIndex;
    int destIndex;
    int improvement;
};

/** @brief Boundary customer considered movable in a three-route cluster split. */
struct ClusterBoundaryCustomer {
    Customer customer;
    std::size_t routeSlot;
    int crossDistance;
    int contribution;
};

/** @brief Route-pair candidate metadata used to scope exact pair splitting. */
struct PairSplitCandidatePair {
    std::size_t sourceIndex;
    std::size_t destIndex;
    float distance;
    int combinedCost;
};

/** @brief Route-triple candidate metadata used by clustered repartitioning. */
struct RouteClusterCandidate {
    std::array<std::size_t, 3> routeIndices;
    float distance;
    int combinedCost;
};

/** @brief Best replacement produced by a three-route cluster repartition. */
struct RouteClusterSplit {
    std::array<Route, 3> routes;
    std::array<int, 3> indexes;
    int improvement;
};

/** @brief Circular three-route sweep split before expensive route reordering. */
struct RouteClusterSweepOrderCandidate {
    std::array<std::vector<Customer>, 3> parts;
    int estimatedCost;
};

/** @brief Three-route boundary assignment retained for final route polishing. */
struct RouteClusterBoundaryCandidate {
    std::array<Route, 3> routes;
    int estimatedCost;
};

/** @brief Customer removal candidate scored by its route-cost contribution. */
struct RuinCustomer {
    Customer customer;
    int contribution;
    std::size_t sequence;
};

/** @brief Customer near another route boundary and useful as a related-removal seed. */
struct BoundaryRuinSeed {
    Customer customer;
    int crossDistance;
    int contribution;
    std::size_t sequence;
};

/** @brief Recreated route set after removing and reinserting customers. */
struct RuinRecreateResult {
    Routes routes;
    int improvement;
    std::size_t sequence;
};

/** @brief Best known route insertion for one removed customer. */
struct RegretInsertion {
    std::size_t customerIndex;
    std::size_t routeIndex;
    Route route;
    int bestIncrease;
    int regret;
};

/** @brief Partial route assignment retained by beam ruin-and-recreate. */
struct BeamInsertionState {
    Routes routes;
    std::vector<Customer> remaining;
    int cost;
    std::size_t sequence;
};

/** @brief Candidate produced by deleting one route and reinserting its customers globally. */
struct RouteReduction {
    Routes routes;
    int removedRouteIndex;
    int costDelta;
};

/** @brief Candidate extra customer to temporarily remove while deleting a route. */
struct RouteReductionBufferCustomer {
    Customer customer;
    int relationCost;
    int contribution;
    std::size_t sequence;
};

/** @brief One direct or ejection-chain insertion step while deleting a route. */
struct RouteEliminationMove {
    std::size_t poolIndex;
    std::size_t routeIndex;
    Route route;
    std::optional<Customer> ejectedCustomer;
    int costDelta;
};

/** @brief Snapshot of a route with its original index. */
struct RouteSnapshot {
    Route route;
    int index;
};

/** @brief Add a canonical route-triple candidate if it has not been queued yet. */
void AddRouteClusterCandidate(const std::vector<RouteSnapshot>& snapshots, std::array<std::size_t, 3> routeIndices,
                              std::set<std::array<std::size_t, 3>>& seen,
                              std::vector<RouteClusterCandidate>& candidates) {
    std::ranges::sort(routeIndices);
    if (!seen.insert(routeIndices).second) {
        return;
    }
    const Route& first = snapshots[routeIndices[0]].route;
    const Route& second = snapshots[routeIndices[1]].route;
    const Route& third = snapshots[routeIndices[2]].route;
    const float distance = first.GetDistanceFrom(second) + first.GetDistanceFrom(third) + second.GetDistanceFrom(third);
    candidates.push_back(RouteClusterCandidate{
        .routeIndices = routeIndices,
        .distance = distance,
        .combinedCost = first.GetTotalCost() + second.GetTotalCost() + third.GetTotalCost(),
    });
}

/** @brief Build geographically close route triples for the cluster repartitioning move. */
std::vector<RouteClusterCandidate> BuildRouteClusterCandidates(const std::vector<RouteSnapshot>& snapshots) {
    std::vector<RouteClusterCandidate> candidates;
    if (snapshots.size() < 3) {
        return candidates;
    }
    std::set<std::array<std::size_t, 3>> seen;
    const std::size_t neighborCount =
        std::min(snapshots.size() - 1, std::max<std::size_t>(2, std::bit_width(snapshots.size())));
    for (std::size_t anchor = 0; anchor < snapshots.size(); ++anchor) {
        std::vector<std::pair<float, std::size_t>> neighbors;
        neighbors.reserve(snapshots.size() - 1);
        for (std::size_t other = 0; other < snapshots.size(); ++other) {
            if (anchor == other) {
                continue;
            }
            neighbors.emplace_back(snapshots[anchor].route.GetDistanceFrom(snapshots[other].route), other);
        }
        std::ranges::sort(neighbors, [&snapshots](const auto& left, const auto& right) {
            if (left.first != right.first) {
                return left.first < right.first;
            }
            if (snapshots[left.second].route.GetTotalCost() != snapshots[right.second].route.GetTotalCost()) {
                return snapshots[left.second].route.GetTotalCost() > snapshots[right.second].route.GetTotalCost();
            }
            return left.second < right.second;
        });
        if (neighbors.size() > neighborCount) {
            neighbors.resize(neighborCount);
        }
        for (std::size_t left = 0; left + 1 < neighbors.size(); ++left) {
            for (std::size_t right = left + 1; right < neighbors.size(); ++right) {
                AddRouteClusterCandidate(snapshots, {anchor, neighbors[left].second, neighbors[right].second}, seen,
                                         candidates);
            }
        }
    }
    std::ranges::sort(candidates, [](const RouteClusterCandidate& left, const RouteClusterCandidate& right) {
        if (left.distance != right.distance) {
            return left.distance < right.distance;
        }
        if (left.combinedCost != right.combinedCost) {
            return left.combinedCost > right.combinedCost;
        }
        return left.routeIndices < right.routeIndices;
    });
    return candidates;
}

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
                                         const std::vector<Customer>& suffixRoute, std::size_t suffixCut,
                                         bool reverseTail) {
    std::list<Customer> candidate;
    // The cut is after prefixCut, so the prefix endpoint is kept in the new route.
    for (std::size_t i = 0; i <= prefixCut; ++i) {
        candidate.push_back(prefixRoute[i]);
    }
    // The other route contributes the customer tail after its cut edge, then the depot.
    if (reverseTail) {
        for (std::size_t i = suffixRoute.size() - 1; i-- > suffixCut + 1;) {
            candidate.push_back(suffixRoute[i]);
        }
    } else {
        for (std::size_t i = suffixCut + 1; i + 1 < suffixRoute.size(); ++i) {
            candidate.push_back(suffixRoute[i]);
        }
    }
    candidate.push_back(suffixRoute.back());
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
            for (const bool reverseDestTail : {false, true}) {
                for (const bool reverseSourceTail : {false, true}) {
                    Route candidateSource = source;
                    Route candidateDest = dest;
                    const std::list<Customer> newSourceRoute =
                        BuildTwoOptStarRoute(sourceCustomers, sourceCut, destCustomers, destCut, reverseDestTail);
                    const std::list<Customer> newDestRoute =
                        BuildTwoOptStarRoute(destCustomers, destCut, sourceCustomers, sourceCut, reverseSourceTail);
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

int CustomerRemovalContribution(const Route&, const Customer&);

/** @brief Build a boundary-customer ranking from one route toward another route. */
std::vector<BoundaryCustomer> BuildBoundaryRanking(const Route& source, const Route& other, bool fromSource) {
    std::vector<BoundaryCustomer> ranking;
    const std::vector<Customer> sourceCustomers = RouteCustomerVectorWithoutDepot(source);
    const std::vector<Customer> otherCustomers = RouteCustomerVectorWithoutDepot(other);
    ranking.reserve(sourceCustomers.size());
    if (sourceCustomers.empty() || otherCustomers.empty()) {
        return ranking;
    }
    for (const Customer& customer : sourceCustomers) {
        int crossDistance = std::numeric_limits<int>::max() / 4;
        for (const Customer& otherCustomer : otherCustomers) {
            crossDistance = std::min(crossDistance, source.GetTravelCost(customer, otherCustomer));
        }
        ranking.push_back(BoundaryCustomer{
            .customer = customer,
            .fromSource = fromSource,
            .crossDistance = crossDistance,
            .contribution = CustomerRemovalContribution(source, customer),
        });
    }
    std::ranges::sort(ranking, [](const BoundaryCustomer& left, const BoundaryCustomer& right) {
        if (left.crossDistance != right.crossDistance) {
            return left.crossDistance < right.crossDistance;
        }
        if (left.contribution != right.contribution) {
            return left.contribution > right.contribution;
        }
        return left.customer.name < right.customer.name;
    });
    return ranking;
}

/** @brief Select a balanced movable customer pool for a large route pair. */
std::vector<BoundaryCustomer> BuildBoundaryPool(const Route& source, const Route& dest, int maxBoundaryCustomers) {
    if (maxBoundaryCustomers <= 0) {
        return {};
    }
    std::vector<BoundaryCustomer> sourceRanking = BuildBoundaryRanking(source, dest, true);
    std::vector<BoundaryCustomer> destRanking = BuildBoundaryRanking(dest, source, false);
    std::vector<BoundaryCustomer> pool;
    pool.reserve(static_cast<std::size_t>(maxBoundaryCustomers));
    const std::size_t perSide = static_cast<std::size_t>(std::max(1, maxBoundaryCustomers / 2));
    for (std::size_t index = 0; index < perSide && index < sourceRanking.size(); ++index) {
        pool.push_back(sourceRanking[index]);
    }
    for (std::size_t index = 0; index < perSide && index < destRanking.size(); ++index) {
        pool.push_back(destRanking[index]);
    }
    sourceRanking.insert(sourceRanking.end(), destRanking.cbegin(), destRanking.cend());
    std::ranges::sort(sourceRanking, [](const BoundaryCustomer& left, const BoundaryCustomer& right) {
        if (left.crossDistance != right.crossDistance) {
            return left.crossDistance < right.crossDistance;
        }
        if (left.contribution != right.contribution) {
            return left.contribution > right.contribution;
        }
        return left.customer.name < right.customer.name;
    });
    for (const BoundaryCustomer& customer : sourceRanking) {
        if (std::cmp_greater_equal(pool.size(), maxBoundaryCustomers)) {
            break;
        }
        const auto exists = std::ranges::find_if(
            pool, [&customer](const BoundaryCustomer& selected) { return selected.customer == customer.customer; });
        if (exists == pool.end()) {
            pool.push_back(customer);
        }
    }
    return pool;
}

/** @brief Insert customers one at a time using the lowest feasible cost increase. */
bool AddCustomersGreedy(Route& route, std::vector<Customer> customers) {
    while (!customers.empty()) {
        std::optional<std::pair<std::size_t, Route>> best;
        int bestIncrease = 0;
        for (std::size_t index = 0; index < customers.size(); ++index) {
            Route candidate = route;
            if (!candidate.AddElem(customers[index])) {
                continue;
            }
            const int increase = candidate.GetTotalCost() - route.GetTotalCost();
            if (!best.has_value() || increase < bestIncrease ||
                (increase == bestIncrease && customers[index].name < customers[best->first].name)) {
                best = std::make_pair(index, std::move(candidate));
                bestIncrease = increase;
            }
        }
        if (!best.has_value()) {
            return false;
        }
        route = std::move(best->second);
        customers.erase(customers.begin() + static_cast<std::ptrdiff_t>(best->first));
    }
    return true;
}

/** @brief Read a fixed-size customer segment from a route iterator range. */
std::list<Customer> ReadSegment(RouteList::const_iterator start, RouteList::const_iterator end, int segmentSize) {
    std::list<Customer> segment;
    for (int count = 0; count < segmentSize && start != end; ++count, ++start) {
        segment.push_back(start->first);
    }
    return segment;
}

/** @brief Return original and reversed segment orientations for route-string insertion. */
std::vector<std::list<Customer>> SegmentOrientations(const std::list<Customer>& segment) {
    std::vector<std::list<Customer>> orientations;
    orientations.push_back(segment);
    if (segment.size() > 1) {
        std::list<Customer> reversed = segment;
        reversed.reverse();
        orientations.push_back(std::move(reversed));
    }
    return orientations;
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

/** @brief Build a depot-to-depot route list from an internal customer order. */
std::list<Customer> BuildRouteList(const Customer& depot, const std::vector<Customer>& customers) {
    std::list<Customer> route;
    route.push_back(depot);
    std::ranges::copy(customers, std::back_inserter(route));
    route.push_back(depot);
    return route;
}

/** @brief Return the cost of a depot-to-depot customer order. */
int CustomerOrderCost(const Route& reference, const Customer& depot, const std::vector<Customer>& customers) {
    if (customers.empty()) {
        return 0;
    }
    int cost = reference.GetTravelCost(depot, customers.front());
    for (std::size_t index = 0; index + 1 < customers.size(); ++index) {
        cost += reference.GetTravelCost(customers[index], customers[index + 1]);
    }
    cost += reference.GetTravelCost(customers.back(), depot);
    return cost;
}

/** @brief Improve a fixed customer set with deterministic 2-opt ordering. */
std::vector<Customer> ImproveCustomerOrder2Opt(const Route& reference, const Customer& depot,
                                               std::vector<Customer> customers) {
    if (customers.size() <= 2) {
        return customers;
    }
    int bestCost = CustomerOrderCost(reference, depot, customers);
    bool improved = true;
    while (improved) {
        improved = false;
        for (std::size_t begin = 0; begin + 1 < customers.size(); ++begin) {
            for (std::size_t end = begin + 1; end < customers.size(); ++end) {
                std::vector<Customer> candidate = customers;
                std::reverse(candidate.begin() + static_cast<std::ptrdiff_t>(begin),
                             candidate.begin() + static_cast<std::ptrdiff_t>(end + 1));
                const int candidateCost = CustomerOrderCost(reference, depot, candidate);
                if (candidateCost < bestCost) {
                    customers = std::move(candidate);
                    bestCost = candidateCost;
                    improved = true;
                }
            }
        }
    }
    return customers;
}

/** @brief Return the route-cost saving from bypassing a customer. */
int CustomerRemovalContribution(const Route& route, const Customer& customer) {
    if (route.size() <= 2) {
        return 0;
    }
    RouteList::const_iterator previous = route.GetRoute()->cbegin();
    RouteList::const_iterator current = previous;
    ++current;
    for (; current != route.GetRoute()->cend() && current->first != route.GetRoute()->front().first;
         ++previous, ++current) {
        if (current->first != customer) {
            continue;
        }
        const RouteList::const_iterator next = std::next(current);
        const int bypassCost = next == route.GetRoute()->cend() ? 0 : route.GetTravelCost(previous->first, next->first);
        return previous->second + current->second - bypassCost;
    }
    return 0;
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
    constexpr std::size_t outputRouteCount = 2;
    if (customers.size() < outputRouteCount || std::cmp_greater(customers.size(), maxCombinedCustomers)) {
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
    if (bestImprovement <= 0 || bestMask == 0) {
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

/** @brief Find the best exact three-route repartition inside a bounded cluster.
 *
 * The clustered split pools all customers from three geographically close routes,
 * computes the exact route cost for every feasible subset once, then searches
 * all three-way capacity-feasible partitions. This is the three-route companion
 * to FindBestPairSplit and catches cyclic membership mistakes that strict pair
 * moves cannot improve one pair at a time.
 */
std::optional<RouteClusterSplit> FindBestExactRouteClusterSplit(const std::array<RouteSnapshot, 3>& cluster,
                                                                std::size_t maxCombinedCustomers) {
    std::vector<Customer> customers;
    for (const RouteSnapshot& snapshot : cluster) {
        const std::vector<Customer> routeCustomers = RouteCustomerVectorWithoutDepot(snapshot.route);
        customers.insert(customers.end(), routeCustomers.cbegin(), routeCustomers.cend());
    }
    if (customers.size() < cluster.size() || customers.size() > maxCombinedCustomers) {
        return std::nullopt;
    }

    const Route& reference = cluster.front().route;
    const Customer depot = reference.GetRoute()->front().first;
    const int capacity = reference.GetInitialCapacity();
    const std::size_t customerCount = customers.size();
    const std::size_t stateCount = std::size_t{1} << customerCount;
    const std::size_t allCustomers = stateCount - 1;
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
        dp[state(std::size_t{1} << customer, customer)] = reference.GetTravelCost(depot, customers[customer]);
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
                const int candidateCost = currentCost + reference.GetTravelCost(customers[last], customers[next]);
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
        if (demand[mask] > capacity) {
            continue;
        }
        for (std::size_t last = 0; last < customerCount; ++last) {
            const int pathCost = dp[state(mask, last)];
            if (pathCost == infinity) {
                continue;
            }
            const int candidateCost = pathCost + reference.GetTravelCost(customers[last], depot);
            if (candidateCost < routeCost[mask]) {
                routeCost[mask] = candidateCost;
                routeLast[mask] = static_cast<int>(last);
            }
        }
    }

    const int originalCost =
        cluster[0].route.GetTotalCost() + cluster[1].route.GetTotalCost() + cluster[2].route.GetTotalCost();
    int bestImprovement = 0;
    std::array<std::size_t, 3> bestMasks = {0, 0, 0};
    // Put the lowest-numbered customer in the first subset to remove symmetric
    // duplicates where the same three routes are merely permuted.
    for (std::size_t firstMask = 1; firstMask < allCustomers; ++firstMask) {
        if ((firstMask & 1U) == 0 || routeCost[firstMask] == infinity) {
            continue;
        }
        const std::size_t remainingAfterFirst = allCustomers ^ firstMask;
        if (remainingAfterFirst == 0) {
            continue;
        }
        const std::size_t requiredSecondBit = remainingAfterFirst & (~remainingAfterFirst + 1);
        // Force the lowest remaining bit into the second subset. The third subset
        // is then implied, avoiding duplicate second/third route orderings.
        for (std::size_t secondMask = remainingAfterFirst; secondMask != 0;
             secondMask = (secondMask - 1) & remainingAfterFirst) {
            if ((secondMask & requiredSecondBit) == 0 || secondMask == remainingAfterFirst ||
                routeCost[secondMask] == infinity) {
                continue;
            }
            const std::size_t thirdMask = remainingAfterFirst ^ secondMask;
            if (thirdMask == 0 || routeCost[thirdMask] == infinity) {
                continue;
            }
            const int candidateCost = routeCost[firstMask] + routeCost[secondMask] + routeCost[thirdMask];
            const int improvement = originalCost - candidateCost;
            if (improvement > bestImprovement) {
                bestImprovement = improvement;
                bestMasks = {firstMask, secondMask, thirdMask};
            }
        }
    }
    if (bestImprovement <= 0) {
        return std::nullopt;
    }

    std::array<Route, 3> candidateRoutes = {cluster[0].route, cluster[1].route, cluster[2].route};
    for (std::size_t routeSlot = 0; routeSlot < candidateRoutes.size(); ++routeSlot) {
        if (!candidateRoutes[routeSlot].RebuildRoute(
                BuildExactRoute(depot, customers, parent, routeLast, bestMasks[routeSlot]))) {
            return std::nullopt;
        }
    }
    return RouteClusterSplit{
        .routes = std::move(candidateRoutes),
        .indexes = {cluster[0].index, cluster[1].index, cluster[2].index},
        .improvement = bestImprovement,
    };
}

/** @brief Reassign a small boundary pool between two larger routes.
 *
 * Full exact pair splitting becomes exponential in the combined route size.
 * This bounded variant keeps the stable cores in their current routes and only
 * enumerates side assignments for customers that look close to the opposite
 * route boundary. It is a generic λ-interchange-style intensification move,
 * not tied to a specific instance size or benchmark name.
 */
std::optional<BoundaryPairSplit> FindBestBoundaryPairSplit(const Route& source, const Route& dest, int sourceIndex,
                                                           int destIndex, int maxBoundaryCustomers) {
    const std::vector<BoundaryCustomer> boundaryPool = BuildBoundaryPool(source, dest, maxBoundaryCustomers);
    if (boundaryPool.size() < 2) {
        return std::nullopt;
    }
    const int originalCost = source.GetTotalCost() + dest.GetTotalCost();
    int bestImprovement = 0;
    std::optional<BoundaryPairSplit> best;
    const std::size_t assignmentCount = std::size_t{1} << boundaryPool.size();
    for (std::size_t assignment = 0; assignment < assignmentCount; ++assignment) {
        Route candidateSource = source;
        Route candidateDest = dest;
        std::vector<Customer> addToSource;
        std::vector<Customer> addToDest;
        bool movedCustomer = false;
        bool feasible = true;
        for (std::size_t index = 0; index < boundaryPool.size(); ++index) {
            const BoundaryCustomer& boundary = boundaryPool[index];
            const bool assignedSource = (assignment & (std::size_t{1} << index)) != 0;
            if (boundary.fromSource == assignedSource) {
                continue;
            }
            movedCustomer = true;
            if (boundary.fromSource) {
                feasible = candidateSource.RemoveCustomer(boundary.customer) && feasible;
                addToDest.push_back(boundary.customer);
            } else {
                feasible = candidateDest.RemoveCustomer(boundary.customer) && feasible;
                addToSource.push_back(boundary.customer);
            }
        }
        if (!movedCustomer || !feasible || (candidateSource.size() <= 2 && addToSource.empty()) ||
            (candidateDest.size() <= 2 && addToDest.empty())) {
            continue;
        }
        if (!AddCustomersGreedy(candidateSource, std::move(addToSource)) ||
            !AddCustomersGreedy(candidateDest, std::move(addToDest))) {
            continue;
        }
        const int candidateCost = candidateSource.GetTotalCost() + candidateDest.GetTotalCost();
        const int improvement = originalCost - candidateCost;
        if (improvement > bestImprovement) {
            bestImprovement = improvement;
            best = BoundaryPairSplit{
                .source = std::move(candidateSource),
                .dest = std::move(candidateDest),
                .sourceIndex = sourceIndex,
                .destIndex = destIndex,
                .improvement = improvement,
            };
        }
    }
    return best;
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

/** @brief Add one seed customer while preserving first occurrence order. */
void AppendUniqueSeed(std::vector<Customer>& seeds, const Customer& customer, int seedLimit) {
    if (std::cmp_greater_equal(seeds.size(), seedLimit) || std::ranges::find(seeds, customer) != seeds.cend()) {
        return;
    }
    seeds.push_back(customer);
}

/** @brief Collect customers sitting close to a different route as LNS seeds. */
std::vector<BoundaryRuinSeed> CollectBoundaryRuinSeeds(const Routes& routes, int candidateLimit) {
    std::vector<BoundaryRuinSeed> seeds;
    if (candidateLimit <= 0) {
        return seeds;
    }
    std::size_t sequence = 0;
    for (std::size_t routeIndex = 0; routeIndex < routes.size(); ++routeIndex) {
        const std::vector<Customer> routeCustomers = RouteCustomerVectorWithoutDepot(routes[routeIndex]);
        for (const Customer& customer : routeCustomers) {
            int crossDistance = std::numeric_limits<int>::max() / 4;
            for (std::size_t otherIndex = 0; otherIndex < routes.size(); ++otherIndex) {
                if (otherIndex == routeIndex) {
                    continue;
                }
                for (const Customer& otherCustomer : RouteCustomerVectorWithoutDepot(routes[otherIndex])) {
                    crossDistance = std::min(crossDistance, routes[routeIndex].GetTravelCost(customer, otherCustomer));
                }
            }
            seeds.push_back(BoundaryRuinSeed{
                .customer = customer,
                .crossDistance = crossDistance,
                .contribution = CustomerRemovalContribution(routes[routeIndex], customer),
                .sequence = sequence,
            });
            ++sequence;
        }
    }
    std::ranges::sort(seeds, [](const BoundaryRuinSeed& left, const BoundaryRuinSeed& right) {
        if (left.crossDistance != right.crossDistance) {
            return left.crossDistance < right.crossDistance;
        }
        if (left.contribution != right.contribution) {
            return left.contribution > right.contribution;
        }
        return left.sequence < right.sequence;
    });
    if (std::cmp_less(candidateLimit, seeds.size())) {
        seeds.resize(static_cast<std::size_t>(candidateLimit));
    }
    return seeds;
}

/** @brief Blend high-contribution and route-boundary seeds for related LNS. */
std::vector<Customer> BuildRelatedSeedCustomers(const Routes& routes, int seedLimit) {
    std::vector<Customer> seeds;
    if (seedLimit <= 0) {
        return seeds;
    }
    seeds.reserve(static_cast<std::size_t>(seedLimit));
    const std::vector<RuinCustomer> highContributionSeeds = CollectRuinCustomers(routes, seedLimit);
    const int highContributionLimit = (seedLimit + 1) / 2;
    for (const RuinCustomer& seed : highContributionSeeds) {
        if (std::cmp_equal(seeds.size(), highContributionLimit)) {
            break;
        }
        AppendUniqueSeed(seeds, seed.customer, seedLimit);
    }
    for (const BoundaryRuinSeed& seed : CollectBoundaryRuinSeeds(routes, seedLimit)) {
        AppendUniqueSeed(seeds, seed.customer, seedLimit);
    }
    for (const RuinCustomer& seed : highContributionSeeds) {
        AppendUniqueSeed(seeds, seed.customer, seedLimit);
    }
    return seeds;
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

/** @brief Return all non-depot customers across the route set. */
std::vector<Customer> CollectRouteCustomers(const Routes& routes) {
    std::vector<Customer> customers;
    for (const Route& route : routes) {
        const std::vector<Customer> routeCustomers = RouteCustomerVectorWithoutDepot(route);
        customers.insert(customers.end(), routeCustomers.cbegin(), routeCustomers.cend());
    }
    return customers;
}

/** @brief Build unique related-LNS removal sets from nearest customers around each seed. */
std::vector<std::vector<Customer>> BuildRelatedRemovalSets(const Routes& routes, int removalCount, int seedLimit) {
    const std::vector<Customer> seeds = BuildRelatedSeedCustomers(routes, seedLimit);
    const std::vector<Customer> allCustomers = CollectRouteCustomers(routes);
    std::vector<std::vector<Customer>> removalSets;
    if (std::cmp_less(allCustomers.size(), removalCount)) {
        return removalSets;
    }
    removalSets.reserve(seeds.size());
    for (const Customer& seed : seeds) {
        std::vector<std::pair<int, Customer>> related;
        related.reserve(allCustomers.size());
        for (const Customer& customer : allCustomers) {
            related.emplace_back(routes.front().GetTravelCost(seed, customer), customer);
        }
        std::ranges::sort(related, [](const auto& left, const auto& right) {
            if (left.first != right.first) {
                return left.first < right.first;
            }
            return left.second.name < right.second.name;
        });
        std::vector<Customer> removalSet;
        removalSet.reserve(static_cast<std::size_t>(removalCount));
        for (const auto& [_, customer] : related) {
            removalSet.push_back(customer);
            if (std::cmp_equal(removalSet.size(), removalCount)) {
                break;
            }
        }
        std::ranges::sort(removalSet,
                          [](const Customer& left, const Customer& right) { return left.name < right.name; });
        if (std::ranges::find(removalSets, removalSet) == removalSets.end()) {
            removalSets.push_back(std::move(removalSet));
        }
    }
    return removalSets;
}

/** @brief Reinsert removed customers using regret-2 insertion.
 *
 * The customer with the largest difference between its best and second-best
 * feasible insertions is placed first. This avoids consuming scarce capacity
 * with easy customers while hard-to-place customers still have options.
 */
bool InsertCustomersRegret(Routes& routes, std::vector<Customer> customers) {
    constexpr int infinity = std::numeric_limits<int>::max() / 4;
    while (!customers.empty()) {
        std::optional<RegretInsertion> selected;
        for (std::size_t customerIndex = 0; customerIndex < customers.size(); ++customerIndex) {
            int bestIncrease = infinity;
            int secondBestIncrease = infinity;
            std::size_t bestRouteIndex = 0;
            Route bestRoute = routes.front();
            for (std::size_t routeIndex = 0; routeIndex < routes.size(); ++routeIndex) {
                Route candidate = routes[routeIndex];
                if (!candidate.AddElem(customers[customerIndex])) {
                    continue;
                }
                const int increase = ActiveRouteCost(candidate) - ActiveRouteCost(routes[routeIndex]);
                if (increase < bestIncrease) {
                    secondBestIncrease = bestIncrease;
                    bestIncrease = increase;
                    bestRouteIndex = routeIndex;
                    bestRoute = std::move(candidate);
                } else if (increase < secondBestIncrease) {
                    secondBestIncrease = increase;
                }
            }
            if (bestIncrease == infinity) {
                continue;
            }
            const int regret = secondBestIncrease == infinity ? infinity : secondBestIncrease - bestIncrease;
            if (!selected.has_value() || regret > selected->regret ||
                (regret == selected->regret && bestIncrease < selected->bestIncrease) ||
                (regret == selected->regret && bestIncrease == selected->bestIncrease &&
                 customers[customerIndex].name < customers[selected->customerIndex].name)) {
                selected = RegretInsertion{
                    .customerIndex = customerIndex,
                    .routeIndex = bestRouteIndex,
                    .route = std::move(bestRoute),
                    .bestIncrease = bestIncrease,
                    .regret = regret,
                };
            }
        }
        if (!selected.has_value()) {
            return false;
        }
        routes[selected->routeIndex] = std::move(selected->route);
        customers.erase(customers.begin() + static_cast<std::ptrdiff_t>(selected->customerIndex));
    }
    return true;
}

/** @brief Reinsert customers while keeping several low-cost partial assignments.
 *
 * Regret insertion is fast but commits to one route choice at each step. The
 * beam version keeps a bounded set of partial reconstructions, which lets a
 * temporarily second-best insertion survive long enough to free capacity or
 * produce a better route-membership combination.
 */
bool InsertCustomersBeam(Routes& routes, const std::vector<Customer>& customers, int beamWidth) {
    if (customers.empty()) {
        return true;
    }
    if (beamWidth <= 0 || routes.empty()) {
        return false;
    }
    std::vector<BeamInsertionState> beam = {BeamInsertionState{
        .routes = routes,
        .remaining = customers,
        .cost = TotalRouteCost(routes),
        .sequence = 0,
    }};
    std::size_t sequence = 1;
    while (!beam.empty() && !beam.front().remaining.empty()) {
        std::vector<BeamInsertionState> nextBeam;
        for (const BeamInsertionState& state : beam) {
            for (std::size_t customerIndex = 0; customerIndex < state.remaining.size(); ++customerIndex) {
                const Customer& customer = state.remaining[customerIndex];
                for (std::size_t routeIndex = 0; routeIndex < state.routes.size(); ++routeIndex) {
                    Route candidateRoute = state.routes[routeIndex];
                    if (!candidateRoute.AddElem(customer)) {
                        continue;
                    }
                    Routes candidateRoutes = state.routes;
                    const int candidateCost =
                        state.cost - ActiveRouteCost(candidateRoutes[routeIndex]) + ActiveRouteCost(candidateRoute);
                    candidateRoutes[routeIndex] = std::move(candidateRoute);
                    std::vector<Customer> remaining = state.remaining;
                    remaining.erase(remaining.begin() + static_cast<std::ptrdiff_t>(customerIndex));
                    nextBeam.push_back(BeamInsertionState{
                        .routes = std::move(candidateRoutes),
                        .remaining = std::move(remaining),
                        .cost = candidateCost,
                        .sequence = sequence++,
                    });
                }
            }
        }
        if (nextBeam.empty()) {
            return false;
        }
        std::ranges::sort(nextBeam, [](const BeamInsertionState& left, const BeamInsertionState& right) {
            if (left.cost != right.cost) {
                return left.cost < right.cost;
            }
            if (left.remaining.size() != right.remaining.size()) {
                return left.remaining.size() < right.remaining.size();
            }
            return left.sequence < right.sequence;
        });
        if (std::cmp_greater(nextBeam.size(), beamWidth)) {
            nextBeam.resize(static_cast<std::size_t>(beamWidth));
        }
        beam = std::move(nextBeam);
    }
    routes = std::move(beam.front().routes);
    return true;
}

/** @brief Return the route index that currently contains a customer. */
std::optional<std::size_t> FindCustomerRouteIndex(const Routes& routes, const Customer& customer) {
    for (std::size_t routeIndex = 0; routeIndex < routes.size(); ++routeIndex) {
        if (routes[routeIndex].FindCustomer(customer)) {
            return routeIndex;
        }
    }
    return std::nullopt;
}

/** @brief Reinsert customers while preferring routes different from their origin. */
bool InsertCustomersRegretAvoidingOrigin(Routes& routes, std::vector<Customer> customers,
                                         const std::map<Customer, std::size_t>& originRouteIndex) {
    constexpr int infinity = std::numeric_limits<int>::max() / 4;
    while (!customers.empty()) {
        std::optional<RegretInsertion> selected;
        for (std::size_t customerIndex = 0; customerIndex < customers.size(); ++customerIndex) {
            int bestIncrease = infinity;
            int secondBestIncrease = infinity;
            std::size_t bestRouteIndex = 0;
            Route bestRoute = routes.front();
            int bestChangedIncrease = infinity;
            std::size_t bestChangedRouteIndex = 0;
            Route bestChangedRoute = routes.front();
            const auto origin = originRouteIndex.find(customers[customerIndex]);
            for (std::size_t routeIndex = 0; routeIndex < routes.size(); ++routeIndex) {
                Route candidate = routes[routeIndex];
                if (!candidate.AddElem(customers[customerIndex])) {
                    continue;
                }
                const int increase = ActiveRouteCost(candidate) - ActiveRouteCost(routes[routeIndex]);
                if (increase < bestIncrease) {
                    secondBestIncrease = bestIncrease;
                    bestIncrease = increase;
                    bestRouteIndex = routeIndex;
                    bestRoute = candidate;
                } else if (increase < secondBestIncrease) {
                    secondBestIncrease = increase;
                }
                if ((origin == originRouteIndex.end() || origin->second != routeIndex) &&
                    increase < bestChangedIncrease) {
                    bestChangedIncrease = increase;
                    bestChangedRouteIndex = routeIndex;
                    bestChangedRoute = std::move(candidate);
                }
            }
            if (bestIncrease == infinity) {
                continue;
            }
            const bool changedRouteAvailable = bestChangedIncrease != infinity;
            const int chosenIncrease = changedRouteAvailable ? bestChangedIncrease : bestIncrease;
            const int regret = secondBestIncrease == infinity ? infinity : secondBestIncrease - chosenIncrease;
            if (!selected.has_value() || regret > selected->regret ||
                (regret == selected->regret && chosenIncrease < selected->bestIncrease) ||
                (regret == selected->regret && chosenIncrease == selected->bestIncrease &&
                 customers[customerIndex].name < customers[selected->customerIndex].name)) {
                selected = RegretInsertion{
                    .customerIndex = customerIndex,
                    .routeIndex = changedRouteAvailable ? bestChangedRouteIndex : bestRouteIndex,
                    .route = changedRouteAvailable ? std::move(bestChangedRoute) : std::move(bestRoute),
                    .bestIncrease = chosenIncrease,
                    .regret = regret,
                };
            }
        }
        if (!selected.has_value()) {
            return false;
        }
        routes[selected->routeIndex] = std::move(selected->route);
        customers.erase(customers.begin() + static_cast<std::ptrdiff_t>(selected->customerIndex));
    }
    return true;
}

/** @brief Select nearby movable buffer customers for route elimination repair. */
std::vector<Customer> BuildRouteReductionBuffer(const Route& source, const Routes& destinations,
                                                int bufferCustomerCount) {
    if (bufferCustomerCount <= 0) {
        return {};
    }
    const std::vector<Customer> sourceCustomers = RouteCustomerVectorWithoutDepot(source);
    std::vector<RouteReductionBufferCustomer> candidates;
    std::size_t sequence = 0;
    for (const Route& route : destinations) {
        for (const Customer& customer : RouteCustomerVectorWithoutDepot(route)) {
            int relationCost = std::numeric_limits<int>::max() / 4;
            for (const Customer& sourceCustomer : sourceCustomers) {
                relationCost = std::min(relationCost, source.GetTravelCost(sourceCustomer, customer));
            }
            candidates.push_back(RouteReductionBufferCustomer{
                .customer = customer,
                .relationCost = relationCost,
                .contribution = CustomerRemovalContribution(route, customer),
                .sequence = sequence,
            });
            ++sequence;
        }
    }
    std::ranges::sort(candidates,
                      [](const RouteReductionBufferCustomer& left, const RouteReductionBufferCustomer& right) {
                          if (left.relationCost != right.relationCost) {
                              return left.relationCost < right.relationCost;
                          }
                          if (left.contribution != right.contribution) {
                              return left.contribution > right.contribution;
                          }
                          return left.sequence < right.sequence;
                      });
    std::vector<Customer> buffer;
    buffer.reserve(static_cast<std::size_t>(bufferCustomerCount));
    for (const RouteReductionBufferCustomer& candidate : candidates) {
        buffer.push_back(candidate.customer);
        if (std::cmp_equal(buffer.size(), bufferCustomerCount)) {
            break;
        }
    }
    return buffer;
}

/** @brief Delete one route with extra buffer removals, then regret-reinsert the whole pool. */
std::optional<RouteReduction> EvaluateBufferedRouteRemoval(const Routes& routes, std::size_t sourceIndex,
                                                           int bufferCustomerCount) {
    if (routes.size() <= 1 || sourceIndex >= routes.size()) {
        return std::nullopt;
    }
    std::vector<Customer> removedCustomers = RouteCustomerVectorWithoutDepot(routes[sourceIndex]);
    if (removedCustomers.empty()) {
        return std::nullopt;
    }
    Routes candidate;
    candidate.reserve(routes.size() - 1);
    for (std::size_t index = 0; index < routes.size(); ++index) {
        if (index != sourceIndex) {
            candidate.push_back(routes[index]);
        }
    }
    const std::vector<Customer> buffer = BuildRouteReductionBuffer(routes[sourceIndex], candidate, bufferCustomerCount);
    removedCustomers.insert(removedCustomers.end(), buffer.cbegin(), buffer.cend());
    for (const Customer& customer : buffer) {
        for (Route& route : candidate) {
            if (route.RemoveCustomer(customer)) {
                break;
            }
        }
    }
    if (!InsertCustomersRegret(candidate, removedCustomers)) {
        return std::nullopt;
    }
    std::erase_if(candidate, [](const Route& route) { return route.size() <= 2; });
    if (candidate.size() >= routes.size()) {
        return std::nullopt;
    }
    const int costDelta = TotalRouteCost(candidate) - TotalRouteCost(routes);
    return RouteReduction{
        .routes = std::move(candidate),
        .removedRouteIndex = static_cast<int>(sourceIndex),
        .costDelta = costDelta,
    };
}

/** @brief Build deterministic insertion orders for route elimination.
 *
 * Eliminating a route is a repair move, and the order used to reinsert removed
 * customers can decide whether the repair is feasible. These orders stay
 * generic: original route order, reverse order, demand-based orders, and
 * hardest-to-place-first based on current feasible insertion cost.
 */
std::vector<std::vector<Customer>> BuildRouteReductionOrders(const Route& source, const Routes& destinations) {
    std::vector<Customer> original;
    for (const Customer& customer : RouteCustomersWithoutDepot(source)) {
        original.push_back(customer);
    }
    std::vector<std::vector<Customer>> orders;
    if (original.empty()) {
        return orders;
    }
    orders.push_back(original);

    std::vector<Customer> reversed = original;
    std::ranges::reverse(reversed);
    orders.push_back(std::move(reversed));

    std::vector<Customer> demandDescending = original;
    std::ranges::sort(demandDescending, [](const Customer& left, const Customer& right) {
        if (left.request != right.request) {
            return left.request > right.request;
        }
        return left.name < right.name;
    });
    orders.push_back(std::move(demandDescending));

    std::vector<Customer> demandAscending = original;
    std::ranges::sort(demandAscending, [](const Customer& left, const Customer& right) {
        if (left.request != right.request) {
            return left.request < right.request;
        }
        return left.name < right.name;
    });
    orders.push_back(std::move(demandAscending));

    constexpr int infinity = std::numeric_limits<int>::max() / 4;
    std::vector<std::pair<int, Customer>> insertionDifficulty;
    insertionDifficulty.reserve(original.size());
    for (const Customer& customer : original) {
        int bestIncrease = infinity;
        for (const Route& route : destinations) {
            Route candidate = route;
            if (candidate.AddElem(customer)) {
                bestIncrease = std::min(bestIncrease, ActiveRouteCost(candidate) - ActiveRouteCost(route));
            }
        }
        insertionDifficulty.emplace_back(bestIncrease, customer);
    }
    std::ranges::sort(insertionDifficulty, [](const auto& left, const auto& right) {
        if (left.first != right.first) {
            return left.first > right.first;
        }
        return left.second.name < right.second.name;
    });
    std::vector<Customer> hardestFirst;
    hardestFirst.reserve(insertionDifficulty.size());
    for (const auto& [_, customer] : insertionDifficulty) {
        hardestFirst.push_back(customer);
    }
    orders.push_back(std::move(hardestFirst));

    std::ranges::sort(orders);
    const auto uniqueSubrange = std::ranges::unique(orders);
    orders.erase(uniqueSubrange.begin(), uniqueSubrange.end());
    return orders;
}

/** @brief Try to delete one route and insert its customers into all remaining routes.
 *
 * Route-count pressure is intentionally evaluated on a full route-set copy.
 * Reinsertions are greedy but global: every removed customer is placed into the
 * feasible route that adds the least cost at that moment, rather than forcing
 * the whole deleted route into one destination.
 */
std::optional<RouteReduction> EvaluateRouteRemoval(const Routes& routes, std::size_t sourceIndex, bool allowEjection) {
    if (routes.size() <= 1 || sourceIndex >= routes.size()) {
        return std::nullopt;
    }
    Routes destinations;
    destinations.reserve(routes.size() - 1);
    for (std::size_t index = 0; index < routes.size(); ++index) {
        if (index != sourceIndex) {
            destinations.push_back(routes[index]);
        }
    }
    const std::vector<std::vector<Customer>> orders = BuildRouteReductionOrders(routes[sourceIndex], destinations);
    if (orders.empty()) {
        return std::nullopt;
    }
    std::optional<RouteReduction> best;
    const int originalCost = TotalRouteCost(routes);

    auto updateBest = [&best, sourceIndex, originalCost](Routes candidate) {
        std::erase_if(candidate, [](const Route& route) { return route.size() <= 2; });
        const int costDelta = TotalRouteCost(candidate) - originalCost;
        if (!best.has_value() || costDelta < best->costDelta) {
            best = RouteReduction{
                .routes = std::move(candidate),
                .removedRouteIndex = static_cast<int>(sourceIndex),
                .costDelta = costDelta,
            };
        }
    };

    for (const std::vector<Customer>& order : orders) {
        Routes candidate = destinations;
        bool feasible = true;
        // First try direct insertion of the deleted route into the remaining
        // routes; this cheap path handles many eliminations without ejections.
        for (const Customer& customer : order) {
            if (!InsertCustomerBestRoute(candidate, customer)) {
                feasible = false;
                break;
            }
        }
        if (!feasible) {
            continue;
        }
        updateBest(std::move(candidate));
    }

    if (!allowEjection) {
        return best;
    }

    for (const std::vector<Customer>& order : orders) {
        Routes candidate = destinations;
        std::vector<Customer> pool = order;
        std::map<Customer, int> ejectionCount;
        const std::size_t stepLimit = pool.size() * routes.size();
        for (std::size_t step = 0; !pool.empty() && step < stepLimit; ++step) {
            std::optional<RouteEliminationMove> bestMove;
            // When direct insertion is blocked, temporarily eject one already
            // assigned customer and put that victim back into the pool.
            for (std::size_t poolIndex = 0; poolIndex < pool.size(); ++poolIndex) {
                const Customer& customer = pool[poolIndex];
                for (std::size_t routeIndex = 0; routeIndex < candidate.size(); ++routeIndex) {
                    Route directRoute = candidate[routeIndex];
                    if (directRoute.AddElem(customer)) {
                        const int costDelta = ActiveRouteCost(directRoute) - ActiveRouteCost(candidate[routeIndex]);
                        if (!bestMove.has_value() || costDelta < bestMove->costDelta ||
                            (costDelta == bestMove->costDelta && !bestMove->ejectedCustomer.has_value())) {
                            bestMove = RouteEliminationMove{
                                .poolIndex = poolIndex,
                                .routeIndex = routeIndex,
                                .route = std::move(directRoute),
                                .ejectedCustomer = std::nullopt,
                                .costDelta = costDelta,
                            };
                        }
                    }

                    const std::vector<Customer> victims = RouteCustomerVectorWithoutDepot(candidate[routeIndex]);
                    for (const Customer& victim : victims) {
                        if (victim == customer || ejectionCount[victim] >= 2) {
                            continue;
                        }
                        Route ejectionRoute = candidate[routeIndex];
                        if (!ejectionRoute.RemoveCustomer(victim) || !ejectionRoute.AddElem(customer)) {
                            continue;
                        }
                        const int costDelta = ActiveRouteCost(ejectionRoute) - ActiveRouteCost(candidate[routeIndex]);
                        const bool preferThisTie = bestMove.has_value() && costDelta == bestMove->costDelta &&
                                                   bestMove->ejectedCustomer.has_value() &&
                                                   victim.name < bestMove->ejectedCustomer->name;
                        if (!bestMove.has_value() || costDelta < bestMove->costDelta || preferThisTie) {
                            bestMove = RouteEliminationMove{
                                .poolIndex = poolIndex,
                                .routeIndex = routeIndex,
                                .route = std::move(ejectionRoute),
                                .ejectedCustomer = victim,
                                .costDelta = costDelta,
                            };
                        }
                    }
                }
            }
            if (!bestMove.has_value()) {
                break;
            }
            candidate[bestMove->routeIndex] = std::move(bestMove->route);
            if (bestMove->ejectedCustomer.has_value()) {
                // Limit repeated ejection of the same customer so repair attempts
                // cannot cycle among a small set of hard-to-place customers.
                ++ejectionCount[*bestMove->ejectedCustomer];
                pool[bestMove->poolIndex] = *bestMove->ejectedCustomer;
            } else {
                pool.erase(pool.begin() + static_cast<std::ptrdiff_t>(bestMove->poolIndex));
            }
            std::erase_if(candidate, [](const Route& route) { return route.size() <= 2; });
        }
        if (pool.empty()) {
            updateBest(std::move(candidate));
        }
    }
    const int sourceCustomerCount = static_cast<int>(RouteCustomerVectorWithoutDepot(routes[sourceIndex]).size());
    // Buffered removal frees extra capacity from neighboring routes before repair,
    // which can make a route deletion feasible when pure insertion cannot.
    for (int bufferCustomerCount = 1; bufferCustomerCount <= sourceCustomerCount; ++bufferCustomerCount) {
        std::optional<RouteReduction> buffered = EvaluateBufferedRouteRemoval(routes, sourceIndex, bufferCustomerCount);
        if (buffered.has_value() && (!best.has_value() || buffered->costDelta < best->costDelta)) {
            best = std::move(buffered);
        }
    }
    return best;
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

/** @brief Find the best two-route angular repartition for a potentially large route pair. */
std::optional<PairSplit> FindBestPairSweepSplit(const Route& source, const Route& dest, int sourceIndex,
                                                int destIndex) {
    std::vector<Customer> customers = RouteCustomerVectorWithoutDepot(source);
    const std::vector<Customer> destCustomers = RouteCustomerVectorWithoutDepot(dest);
    customers.insert(customers.end(), destCustomers.cbegin(), destCustomers.cend());
    constexpr std::size_t outputRouteCount = 2;
    if (customers.size() < outputRouteCount) {
        return std::nullopt;
    }
    const Customer depot = source.GetRoute()->front().first;
    std::ranges::sort(customers, [&depot](const Customer& left, const Customer& right) {
        const double leftAngle =
            std::atan2(static_cast<double>(left.y - depot.y), static_cast<double>(left.x - depot.x));
        const double rightAngle =
            std::atan2(static_cast<double>(right.y - depot.y), static_cast<double>(right.x - depot.x));
        if (leftAngle != rightAngle) {
            return leftAngle < rightAngle;
        }
        return left.name < right.name;
    });

    const int originalCost = source.GetTotalCost() + dest.GetTotalCost();
    const int capacity = source.GetInitialCapacity();
    constexpr std::size_t refinementCandidateLimit = 4;
    std::vector<PairSweepOrderCandidate> orderCandidates;
    const std::size_t count = customers.size();
    for (const int direction : {1, -1}) {
        for (std::size_t start = 0; start < count; ++start) {
            std::vector<Customer> ordered;
            ordered.reserve(count);
            for (std::size_t offset = 0; offset < count; ++offset) {
                const std::size_t index = direction > 0 ? (start + offset) % count : (start + count - offset) % count;
                ordered.push_back(customers[index]);
            }
            std::vector<int> demandPrefix(count + 1, 0);
            for (std::size_t index = 0; index < count; ++index) {
                demandPrefix[index + 1] = demandPrefix[index] + ordered[index].request;
            }
            for (std::size_t split = 1; split < count; ++split) {
                const int firstDemand = demandPrefix[split];
                const int secondDemand = demandPrefix[count] - demandPrefix[split];
                if (firstDemand > capacity || secondDemand > capacity) {
                    continue;
                }
                std::vector<Customer> first(ordered.cbegin(), ordered.cbegin() + static_cast<std::ptrdiff_t>(split));
                std::vector<Customer> second(ordered.cbegin() + static_cast<std::ptrdiff_t>(split), ordered.cend());
                const int estimatedCost =
                    CustomerOrderCost(source, depot, first) + CustomerOrderCost(source, depot, second);
                orderCandidates.push_back(PairSweepOrderCandidate{
                    .first = std::move(first),
                    .second = std::move(second),
                    .estimatedCost = estimatedCost,
                });
                std::ranges::sort(orderCandidates,
                                  [](const PairSweepOrderCandidate& left, const PairSweepOrderCandidate& right) {
                                      return left.estimatedCost < right.estimatedCost;
                                  });
                if (orderCandidates.size() > refinementCandidateLimit) {
                    orderCandidates.resize(refinementCandidateLimit);
                }
            }
        }
    }

    std::optional<PairSplit> best;
    for (PairSweepOrderCandidate& orderCandidate : orderCandidates) {
        std::vector<Customer> first = ImproveCustomerOrder2Opt(source, depot, std::move(orderCandidate.first));
        std::vector<Customer> second = ImproveCustomerOrder2Opt(source, depot, std::move(orderCandidate.second));
        Route candidateSource = source;
        Route candidateDest = dest;
        if (!candidateSource.RebuildRoute(BuildRouteList(depot, first)) ||
            !candidateDest.RebuildRoute(BuildRouteList(depot, second))) {
            continue;
        }
        OptimizeSingleRouteTsp(candidateSource, 14);
        OptimizeSingleRouteTsp(candidateDest, 14);
        const int candidateCost = candidateSource.GetTotalCost() + candidateDest.GetTotalCost();
        const int improvement = originalCost - candidateCost;
        if (improvement > 0 && (!best.has_value() || improvement > best->improvement)) {
            best = PairSplit{
                .source = std::move(candidateSource),
                .dest = std::move(candidateDest),
                .sourceIndex = sourceIndex,
                .destIndex = destIndex,
                .improvement = improvement,
            };
        }
    }
    return best;
}

/** @brief Build a bounded boundary pool for a three-route cluster. */
std::vector<ClusterBoundaryCustomer> BuildClusterBoundaryPool(const std::array<RouteSnapshot, 3>& cluster,
                                                              int maxBoundaryCustomers) {
    if (maxBoundaryCustomers <= 0) {
        return {};
    }
    auto betterBoundaryCustomer = [](const ClusterBoundaryCustomer& left, const ClusterBoundaryCustomer& right) {
        if (left.crossDistance != right.crossDistance) {
            return left.crossDistance < right.crossDistance;
        }
        if (left.contribution != right.contribution) {
            return left.contribution > right.contribution;
        }
        return left.customer.name < right.customer.name;
    };
    std::array<std::vector<ClusterBoundaryCustomer>, 3> routeRankings;
    std::vector<ClusterBoundaryCustomer> globalRanking;
    for (std::size_t routeSlot = 0; routeSlot < cluster.size(); ++routeSlot) {
        const std::vector<Customer> routeCustomers = RouteCustomerVectorWithoutDepot(cluster[routeSlot].route);
        routeRankings[routeSlot].reserve(routeCustomers.size());
        for (const Customer& customer : routeCustomers) {
            int crossDistance = std::numeric_limits<int>::max() / 4;
            for (std::size_t otherSlot = 0; otherSlot < cluster.size(); ++otherSlot) {
                if (otherSlot == routeSlot) {
                    continue;
                }
                for (const Customer& otherCustomer : RouteCustomerVectorWithoutDepot(cluster[otherSlot].route)) {
                    crossDistance =
                        std::min(crossDistance, cluster[routeSlot].route.GetTravelCost(customer, otherCustomer));
                }
            }
            routeRankings[routeSlot].push_back(ClusterBoundaryCustomer{
                .customer = customer,
                .routeSlot = routeSlot,
                .crossDistance = crossDistance,
                .contribution = CustomerRemovalContribution(cluster[routeSlot].route, customer),
            });
        }
        std::ranges::sort(routeRankings[routeSlot], betterBoundaryCustomer);
        globalRanking.insert(globalRanking.end(), routeRankings[routeSlot].cbegin(), routeRankings[routeSlot].cend());
    }

    std::vector<ClusterBoundaryCustomer> pool;
    pool.reserve(static_cast<std::size_t>(maxBoundaryCustomers));
    for (const std::vector<ClusterBoundaryCustomer>& routeRanking : routeRankings) {
        if (!routeRanking.empty() && std::cmp_less(pool.size(), maxBoundaryCustomers)) {
            pool.push_back(routeRanking.front());
        }
    }
    std::ranges::sort(globalRanking, betterBoundaryCustomer);
    for (const ClusterBoundaryCustomer& candidate : globalRanking) {
        if (!std::cmp_less(pool.size(), maxBoundaryCustomers)) {
            break;
        }
        const bool alreadySelected = std::ranges::any_of(pool, [&candidate](const ClusterBoundaryCustomer& selected) {
            return selected.routeSlot == candidate.routeSlot && selected.customer == candidate.customer;
        });
        if (!alreadySelected) {
            pool.push_back(candidate);
        }
    }
    return pool;
}

/** @brief Find the best circular sweep split for three geographically related routes. */
std::optional<RouteClusterSplit> FindBestRouteClusterSweepSplit(const std::array<RouteSnapshot, 3>& cluster) {
    std::vector<Customer> customers;
    for (const RouteSnapshot& snapshot : cluster) {
        const std::vector<Customer> routeCustomers = RouteCustomerVectorWithoutDepot(snapshot.route);
        customers.insert(customers.end(), routeCustomers.cbegin(), routeCustomers.cend());
    }
    if (customers.size() < cluster.size()) {
        return std::nullopt;
    }
    const Route& reference = cluster.front().route;
    const Customer depot = reference.GetRoute()->front().first;
    const int capacity = reference.GetInitialCapacity();
    const int originalCost =
        cluster[0].route.GetTotalCost() + cluster[1].route.GetTotalCost() + cluster[2].route.GetTotalCost();

    std::ranges::sort(customers, [&depot](const Customer& left, const Customer& right) {
        const double leftAngle =
            std::atan2(static_cast<double>(left.y - depot.y), static_cast<double>(left.x - depot.x));
        const double rightAngle =
            std::atan2(static_cast<double>(right.y - depot.y), static_cast<double>(right.x - depot.x));
        if (leftAngle != rightAngle) {
            return leftAngle < rightAngle;
        }
        return left.name < right.name;
    });

    std::vector<RouteClusterSweepOrderCandidate> orderCandidates;
    const std::size_t count = customers.size();
    constexpr std::size_t refinementCandidateLimit = 4;
    for (const int direction : {1, -1}) {
        for (std::size_t start = 0; start < count; ++start) {
            std::vector<Customer> ordered;
            ordered.reserve(count);
            for (std::size_t offset = 0; offset < count; ++offset) {
                const std::size_t index = direction > 0 ? (start + offset) % count : (start + count - offset) % count;
                ordered.push_back(customers[index]);
            }
            std::vector<int> demandPrefix(count + 1, 0);
            for (std::size_t index = 0; index < count; ++index) {
                demandPrefix[index + 1] = demandPrefix[index] + ordered[index].request;
            }
            for (std::size_t firstSplit = 1; firstSplit + 1 < count; ++firstSplit) {
                const int firstDemand = demandPrefix[firstSplit];
                if (firstDemand > capacity) {
                    break;
                }
                for (std::size_t secondSplit = firstSplit + 1; secondSplit < count; ++secondSplit) {
                    const int secondDemand = demandPrefix[secondSplit] - demandPrefix[firstSplit];
                    const int thirdDemand = demandPrefix[count] - demandPrefix[secondSplit];
                    if (secondDemand > capacity) {
                        break;
                    }
                    if (thirdDemand > capacity) {
                        continue;
                    }
                    std::array<std::vector<Customer>, 3> parts = {
                        std::vector<Customer>(ordered.cbegin(),
                                              ordered.cbegin() + static_cast<std::ptrdiff_t>(firstSplit)),
                        std::vector<Customer>(ordered.cbegin() + static_cast<std::ptrdiff_t>(firstSplit),
                                              ordered.cbegin() + static_cast<std::ptrdiff_t>(secondSplit)),
                        std::vector<Customer>(ordered.cbegin() + static_cast<std::ptrdiff_t>(secondSplit),
                                              ordered.cend()),
                    };
                    const int estimatedCost = CustomerOrderCost(reference, depot, parts[0]) +
                                              CustomerOrderCost(reference, depot, parts[1]) +
                                              CustomerOrderCost(reference, depot, parts[2]);
                    orderCandidates.push_back(RouteClusterSweepOrderCandidate{
                        .parts = std::move(parts),
                        .estimatedCost = estimatedCost,
                    });
                    std::ranges::sort(orderCandidates, [](const RouteClusterSweepOrderCandidate& left,
                                                          const RouteClusterSweepOrderCandidate& right) {
                        return left.estimatedCost < right.estimatedCost;
                    });
                    if (orderCandidates.size() > refinementCandidateLimit) {
                        orderCandidates.resize(refinementCandidateLimit);
                    }
                }
            }
        }
    }

    std::optional<RouteClusterSplit> best;
    for (RouteClusterSweepOrderCandidate& orderCandidate : orderCandidates) {
        std::array<Route, 3> candidateRoutes = {cluster[0].route, cluster[1].route, cluster[2].route};
        bool feasible = true;
        for (std::size_t routeSlot = 0; routeSlot < cluster.size(); ++routeSlot) {
            orderCandidate.parts[routeSlot] =
                ImproveCustomerOrder2Opt(reference, depot, std::move(orderCandidate.parts[routeSlot]));
            if (!candidateRoutes[routeSlot].RebuildRoute(BuildRouteList(depot, orderCandidate.parts[routeSlot]))) {
                feasible = false;
                break;
            }
        }
        if (!feasible) {
            continue;
        }
        const int candidateCost =
            candidateRoutes[0].GetTotalCost() + candidateRoutes[1].GetTotalCost() + candidateRoutes[2].GetTotalCost();
        const int improvement = originalCost - candidateCost;
        if (improvement > 0 && (!best.has_value() || improvement > best->improvement)) {
            best = RouteClusterSplit{
                .routes = std::move(candidateRoutes),
                .indexes = {cluster[0].index, cluster[1].index, cluster[2].index},
                .improvement = improvement,
            };
        }
    }
    return best;
}

/** @brief Find the best bounded boundary reassignment for three geographically related routes. */
std::optional<RouteClusterSplit> FindBestRouteClusterBoundarySplit(const std::array<RouteSnapshot, 3>& cluster,
                                                                   int maxBoundaryCustomers) {
    const std::vector<ClusterBoundaryCustomer> boundaryPool = BuildClusterBoundaryPool(cluster, maxBoundaryCustomers);
    if (boundaryPool.size() < cluster.size()) {
        return std::nullopt;
    }
    const int originalCost =
        cluster[0].route.GetTotalCost() + cluster[1].route.GetTotalCost() + cluster[2].route.GetTotalCost();
    std::size_t assignmentCount = 1;
    for (std::size_t index = 0; index < boundaryPool.size(); ++index) {
        assignmentCount *= cluster.size();
    }
    constexpr std::size_t refinementCandidateLimit = 4;
    std::vector<RouteClusterBoundaryCandidate> assignmentCandidates;
    std::optional<RouteClusterSplit> best;
    for (std::size_t assignment = 0; assignment < assignmentCount; ++assignment) {
        std::array<Route, 3> candidateRoutes = {cluster[0].route, cluster[1].route, cluster[2].route};
        std::array<std::vector<Customer>, 3> addToRoute;
        std::size_t encodedAssignment = assignment;
        bool movedCustomer = false;
        bool feasible = true;
        for (const ClusterBoundaryCustomer& boundary : boundaryPool) {
            // Interpret the assignment number as base-3 digits: each digit says
            // which route receives one boundary customer from the shared pool.
            const std::size_t assignedSlot = encodedAssignment % cluster.size();
            encodedAssignment /= cluster.size();
            if (assignedSlot == boundary.routeSlot) {
                continue;
            }
            movedCustomer = true;
            feasible = candidateRoutes[boundary.routeSlot].RemoveCustomer(boundary.customer) && feasible;
            addToRoute[assignedSlot].push_back(boundary.customer);
        }
        if (!movedCustomer || !feasible) {
            continue;
        }
        for (std::size_t routeSlot = 0; routeSlot < candidateRoutes.size(); ++routeSlot) {
            if ((candidateRoutes[routeSlot].size() <= 2 && addToRoute[routeSlot].empty()) ||
                !AddCustomersGreedy(candidateRoutes[routeSlot], std::move(addToRoute[routeSlot]))) {
                feasible = false;
                break;
            }
        }
        if (!feasible) {
            continue;
        }
        const int estimatedCost =
            candidateRoutes[0].GetTotalCost() + candidateRoutes[1].GetTotalCost() + candidateRoutes[2].GetTotalCost();
        assignmentCandidates.push_back(RouteClusterBoundaryCandidate{
            .routes = std::move(candidateRoutes),
            .estimatedCost = estimatedCost,
        });
        // Keep only a few cheapest boundary assignments for exact route-order
        // polishing; this prevents boundary splitting from becoming a full LNS.
        std::ranges::sort(assignmentCandidates,
                          [](const RouteClusterBoundaryCandidate& left, const RouteClusterBoundaryCandidate& right) {
                              return left.estimatedCost < right.estimatedCost;
                          });
        if (assignmentCandidates.size() > refinementCandidateLimit) {
            assignmentCandidates.erase(assignmentCandidates.begin() +
                                           static_cast<std::ptrdiff_t>(refinementCandidateLimit),
                                       assignmentCandidates.end());
        }
    }
    for (RouteClusterBoundaryCandidate& assignmentCandidate : assignmentCandidates) {
        for (Route& route : assignmentCandidate.routes) {
            OptimizeSingleRouteTsp(route, 14);
        }
        const int candidateCost = assignmentCandidate.routes[0].GetTotalCost() +
                                  assignmentCandidate.routes[1].GetTotalCost() +
                                  assignmentCandidate.routes[2].GetTotalCost();
        const int improvement = originalCost - candidateCost;
        if (improvement > 0 && (!best.has_value() || improvement > best->improvement)) {
            best = RouteClusterSplit{
                .routes = std::move(assignmentCandidate.routes),
                .indexes = {cluster[0].index, cluster[1].index, cluster[2].index},
                .improvement = improvement,
            };
        }
    }
    return best;
}

/** @brief Return the number of non-depot customers across a three-route cluster. */
std::size_t RouteClusterCustomerCount(const std::array<RouteSnapshot, 3>& cluster) {
    std::size_t count = 0;
    for (const RouteSnapshot& snapshot : cluster) {
        count += RouteCustomerVectorWithoutDepot(snapshot.route).size();
    }
    return count;
}

/** @brief Try one cyclic assignment of three customers across a route triple. */
std::optional<RouteClusterSplit> BuildCyclicExchange(const std::array<RouteSnapshot, 3>& cluster,
                                                     const std::array<std::list<Customer>, 3>& customerGroups,
                                                     const std::array<std::size_t, 3>& destinationSlots,
                                                     int originalCost) {
    std::array<Route, 3> candidateRoutes = {cluster[0].route, cluster[1].route, cluster[2].route};
    for (std::size_t routeSlot = 0; routeSlot < cluster.size(); ++routeSlot) {
        if (!RemoveSegment(candidateRoutes[routeSlot], customerGroups[routeSlot])) {
            return std::nullopt;
        }
    }
    for (std::size_t routeSlot = 0; routeSlot < cluster.size(); ++routeSlot) {
        const std::size_t destinationSlot = destinationSlots[routeSlot];
        if (!candidateRoutes[destinationSlot].AddElem(customerGroups[routeSlot])) {
            return std::nullopt;
        }
    }
    const int candidateCost =
        candidateRoutes[0].GetTotalCost() + candidateRoutes[1].GetTotalCost() + candidateRoutes[2].GetTotalCost();
    const int improvement = originalCost - candidateCost;
    if (improvement <= 0) {
        return std::nullopt;
    }
    return RouteClusterSplit{
        .routes = std::move(candidateRoutes),
        .indexes = {cluster[0].index, cluster[1].index, cluster[2].index},
        .improvement = improvement,
    };
}

/** @brief Find the best fixed-size cyclic exchange for a route triple. */
std::optional<RouteClusterSplit> FindBestCyclicExchange(const std::array<RouteSnapshot, 3>& cluster, int groupSize) {
    if (groupSize <= 0) {
        return std::nullopt;
    }
    const std::array<std::vector<std::list<Customer>>, 3> routeGroups = {
        BuildCustomerCombinations(cluster[0].route, groupSize),
        BuildCustomerCombinations(cluster[1].route, groupSize),
        BuildCustomerCombinations(cluster[2].route, groupSize),
    };
    for (const std::vector<std::list<Customer>>& groups : routeGroups) {
        if (groups.empty()) {
            return std::nullopt;
        }
    }
    const int originalCost =
        cluster[0].route.GetTotalCost() + cluster[1].route.GetTotalCost() + cluster[2].route.GetTotalCost();
    std::optional<RouteClusterSplit> best;
    for (const std::list<Customer>& first : routeGroups[0]) {
        for (const std::list<Customer>& second : routeGroups[1]) {
            for (const std::list<Customer>& third : routeGroups[2]) {
                const std::array<std::list<Customer>, 3> customerGroups = {first, second, third};
                for (const std::array<std::size_t, 3>& destinationSlots :
                     {std::array<std::size_t, 3>{1, 2, 0}, std::array<std::size_t, 3>{2, 0, 1}}) {
                    std::optional<RouteClusterSplit> candidate =
                        BuildCyclicExchange(cluster, customerGroups, destinationSlots, originalCost);
                    if (candidate.has_value() && (!best.has_value() || candidate->improvement > best->improvement)) {
                        best = std::move(candidate);
                    }
                }
            }
        }
    }
    return best;
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

/** @brief Evaluate one related-customer ruin-and-recreate candidate. */
std::optional<RuinRecreateResult>
EvaluateRelatedRuinRecreate(const Routes& routes, const std::vector<Customer>& customers, std::size_t sequence) {
    Routes candidate = routes;
    const int originalCost = TotalRouteCost(routes);
    for (const Customer& customer : customers) {
        for (Route& route : candidate) {
            if (route.RemoveCustomer(customer)) {
                break;
            }
        }
    }
    if (candidate.empty() || !InsertCustomersRegret(candidate, customers)) {
        return std::nullopt;
    }
    const int candidateCost = TotalRouteCost(candidate);
    if (candidateCost >= originalCost) {
        return std::nullopt;
    }
    return RuinRecreateResult{
        .routes = std::move(candidate),
        .improvement = originalCost - candidateCost,
        .sequence = sequence,
    };
}

/** @brief Evaluate one related-customer ruin-and-recreate candidate with beam insertion. */
std::optional<RuinRecreateResult> EvaluateRelatedBeamRuinRecreate(const Routes& routes,
                                                                  const std::vector<Customer>& customers,
                                                                  std::size_t sequence, int beamWidth) {
    Routes candidate = routes;
    const int originalCost = TotalRouteCost(routes);
    for (const Customer& customer : customers) {
        for (Route& route : candidate) {
            if (route.RemoveCustomer(customer)) {
                break;
            }
        }
    }
    if (candidate.empty() || !InsertCustomersBeam(candidate, customers, beamWidth)) {
        return std::nullopt;
    }
    const int candidateCost = TotalRouteCost(candidate);
    if (candidateCost >= originalCost) {
        return std::nullopt;
    }
    return RuinRecreateResult{
        .routes = std::move(candidate),
        .improvement = originalCost - candidateCost,
        .sequence = sequence,
    };
}

/** @brief Evaluate one related-customer perturbation candidate. */
std::optional<RuinRecreateResult>
EvaluateRelatedPerturbation(const Routes& routes, const std::vector<Customer>& customers, std::size_t sequence) {
    Routes candidate = routes;
    const int originalCost = TotalRouteCost(routes);
    std::map<Customer, std::size_t> originRouteIndex;
    for (const Customer& customer : customers) {
        std::optional<std::size_t> routeIndex = FindCustomerRouteIndex(candidate, customer);
        if (!routeIndex.has_value() || !candidate[*routeIndex].RemoveCustomer(customer)) {
            return std::nullopt;
        }
        originRouteIndex.emplace(customer, *routeIndex);
    }
    if (candidate.empty() || !InsertCustomersRegretAvoidingOrigin(candidate, customers, originRouteIndex)) {
        return std::nullopt;
    }
    bool changedMembership = false;
    for (const auto& [customer, origin] : originRouteIndex) {
        const std::optional<std::size_t> routeIndex = FindCustomerRouteIndex(candidate, customer);
        if (routeIndex.has_value() && *routeIndex != origin) {
            changedMembership = true;
            break;
        }
    }
    if (!changedMembership) {
        return std::nullopt;
    }
    std::erase_if(candidate, [](const Route& route) { return route.size() <= 2; });
    const int candidateCost = TotalRouteCost(candidate);
    return RuinRecreateResult{
        .routes = std::move(candidate),
        .improvement = originalCost - candidateCost,
        .sequence = sequence,
    };
}

/** @brief Return all customers ordered by polar angle around the depot. */
std::vector<Customer> CollectAngularCustomers(const Routes& routes) {
    std::vector<Customer> customers = CollectRouteCustomers(routes);
    if (customers.empty() || routes.empty()) {
        return customers;
    }
    const Customer depot = routes.front().GetRoute()->front().first;
    std::ranges::sort(customers, [&depot](const Customer& left, const Customer& right) {
        const double leftAngle =
            std::atan2(static_cast<double>(left.y - depot.y), static_cast<double>(left.x - depot.x));
        const double rightAngle =
            std::atan2(static_cast<double>(right.y - depot.y), static_cast<double>(right.x - depot.x));
        if (leftAngle != rightAngle) {
            return leftAngle < rightAngle;
        }
        return left.name < right.name;
    });
    return customers;
}

/** @brief Rebuild one angular customer sector away from its current routes.
 *
 * Related-customer ruin tends to remove dense local clusters. This variant cuts
 * a contiguous polar-angle sector around the depot, which targets route-boundary
 * errors common in Euclidean CVRPs. It may accept a temporary cost increase; the
 * following strict VND pass decides whether the new basin is useful.
 * @param[in] routes The routes to edit
 * @param[in] removalCount Number of sector customers removed together
 * @param[in] diversificationRank Rank used to rotate the sector deterministically
 * @return Cost improvement if positive, or a negative cost increase when diversified
 */
int OptimalMove::PerturbAngularRuinRecreate(Routes& routes, int removalCount, int diversificationRank) {
    if (removalCount <= 1 || routes.empty()) {
        Utils::Instance().logger("angular perturbation no move", Utils::VERBOSE);
        return 0;
    }
    const std::vector<Customer> customers = CollectAngularCustomers(routes);
    if (std::cmp_less(customers.size(), removalCount)) {
        Utils::Instance().logger("angular perturbation no move", Utils::VERBOSE);
        return 0;
    }

    const std::size_t window = static_cast<std::size_t>(removalCount);
    const std::size_t stride = std::max<std::size_t>(1, window / 2);
    const std::size_t start = (static_cast<std::size_t>(std::max(0, diversificationRank)) * stride) % customers.size();
    std::vector<Customer> removalSet;
    removalSet.reserve(window);
    for (std::size_t offset = 0; offset < window; ++offset) {
        removalSet.push_back(customers[(start + offset) % customers.size()]);
    }

    Routes candidate = routes;
    const int originalCost = TotalRouteCost(routes);
    std::map<Customer, std::size_t> originRouteIndex;
    for (const Customer& customer : removalSet) {
        std::optional<std::size_t> routeIndex = FindCustomerRouteIndex(candidate, customer);
        if (!routeIndex.has_value() || !candidate[*routeIndex].RemoveCustomer(customer)) {
            Utils::Instance().logger("angular perturbation no move", Utils::VERBOSE);
            return 0;
        }
        originRouteIndex.emplace(customer, *routeIndex);
    }
    if (candidate.empty() || !InsertCustomersRegretAvoidingOrigin(candidate, removalSet, originRouteIndex)) {
        Utils::Instance().logger("angular perturbation no move", Utils::VERBOSE);
        return 0;
    }
    bool changedMembership = false;
    for (const auto& [customer, origin] : originRouteIndex) {
        const std::optional<std::size_t> routeIndex = FindCustomerRouteIndex(candidate, customer);
        if (routeIndex.has_value() && *routeIndex != origin) {
            changedMembership = true;
            break;
        }
    }
    if (!changedMembership) {
        Utils::Instance().logger("angular perturbation no move", Utils::VERBOSE);
        return 0;
    }
    routes = std::move(candidate);
    this->CleanVoid(routes);
    const int improvement = originalCost - TotalRouteCost(routes);
    Utils::Instance().logger("angular perturbation changed: " + std::to_string(improvement), Utils::VERBOSE);
    return improvement;
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
        for (const std::list<Customer>& insertionSegment : SegmentOrientations(segment)) {
            Route candidateSource = source;
            Route candidateDest = dest;
            if (!RemoveSegment(candidateSource, segment) || !candidateDest.AddElem(insertionSegment)) {
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
                                                       int destIndex, int maxSegmentSize, bool force) {
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
                    const std::vector<std::list<Customer>> sourceOrientations = SegmentOrientations(sourceSegment);
                    const std::vector<std::list<Customer>> destOrientations = SegmentOrientations(destSegment);
                    for (const std::list<Customer>& sourceInsertion : sourceOrientations) {
                        for (const std::list<Customer>& destInsertion : destOrientations) {
                            Route candidateSource = source;
                            Route candidateDest = dest;
                            if (!RemoveSegment(candidateSource, sourceSegment) ||
                                !RemoveSegment(candidateDest, destSegment) || !candidateSource.AddElem(destInsertion) ||
                                !candidateDest.AddElem(sourceInsertion)) {
                                continue;
                            }
                            const int candidateCost = ActiveRouteCost(candidateSource) + ActiveRouteCost(candidateDest);
                            const int improvement = originalCost - candidateCost;
                            if ((force || improvement > 0) && (!best.has_value() || improvement > best->improvement)) {
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
            if ((!force && candidateCost < bestCost) || (force && (!ret || candidateCost < bestCost))) {
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
 * @param[in] force  If true, accept the least-cost feasible non-improving move.
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
 * @param[in] force  If true, accept the least-cost feasible non-improving move.
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
                if ((!force && candidateCost < bestCost) || (force && (!ret || candidateCost < bestCost))) {
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
 * @param[in] force  If true, accept the least-cost feasible non-improving move.
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
 * @param[in] force  If true, accept the least-cost feasible non-improving move.
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
            if ((!force && candidateCost < bestCost) || (force && (!bestFound || candidateCost < bestCost))) {
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
 * @param[in] force  If true, accept the least-cost feasible non-improving move.
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
 * @param[in] force  If true, accept the least-cost feasible non-improving move.
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

/** @brief Exchange arbitrary-size customer groups between every ordered route pair.
 *
 * This generalizes the fixed 1-2, 2-1, and 2-2 neighborhoods while reusing the
 * same route-copy evaluation path. It is kept as an explicit VND step so wider
 * group exchanges can be enabled only where their extra cost is justified.
 * @param[in] routes The routes to edit
 * @param[in] nInsert Number of customers moved from source to destination
 * @param[in] nRemove Number of customers moved from destination to source
 * @param[in] force If true, accepts the least-cost feasible exchange for diversification
 * @return Cost reduction if a move is applied, otherwise -1
 */
int OptimalMove::OptExchange(Routes& routes, int nInsert, int nRemove, bool force) {
    int diffCost = -1;
    Routes::iterator it = routes.begin();
    std::set<BestResult, decltype(comp)> b(comp);
    bool flag = false;
    ThreadPool pool(this->cores);
    for (int i = 0; it != routes.end(); std::advance(it, 1), ++i) {
        Routes::const_iterator jt = routes.cbegin();
        for (int j = 0; jt != routes.cend(); std::advance(jt, 1), ++j) {
            if (jt != it) {
                pool.AddTask([force, nInsert, nRemove, it, jt, i, j, &b, &flag, this]() {
                    const int originalCost = it->GetTotalCost() + jt->GetTotalCost();
                    Route tFrom = *it;
                    Route tTo = *jt;
                    if (AddRemoveFromTo(tFrom, tTo, nInsert, nRemove, force)) {
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
        Utils::Instance().logger("opt" + std::to_string(nInsert) + std::to_string(nRemove) +
                                     " improved: " + std::to_string(diffCost),
                                 Utils::VERBOSE);
    } else {
        Utils::Instance().logger("opt" + std::to_string(nInsert) + std::to_string(nRemove) + " no improvement",
                                 Utils::VERBOSE);
    }
    return diffCost;
}

/** @brief Exchange variable-length segments between routes.
 *
 * Searches all route pairs for the best move produced by exchanging consecutive
 * customer segments up to maxSegmentSize. In normal VND mode it only accepts
 * strict improvements. In forced mode it accepts the least-damaging feasible
 * cross-exchange so the outer search can leave a local minimum before another
 * strict VND pass.
 * @param[in] routes The routes to edit
 * @param[in] maxSegmentSize Maximum consecutive customers per exchanged segment
 * @param[in] force If true, allows a feasible non-improving exchange
 * @return Cost reduction if a move is applied, otherwise -1
 */
int OptimalMove::OptSwapSegments(Routes& routes, int maxSegmentSize, bool force) {
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
            pool.AddTask([&snapshots, sourceIndex, destIndex, maxSegmentSize, force, &candidates, &candidatesMutex]() {
                std::optional<SegmentExchange> candidate = FindBestSegmentExchange(
                    snapshots[sourceIndex].route, snapshots[destIndex].route, snapshots[sourceIndex].index,
                    snapshots[destIndex].index, maxSegmentSize, force);
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
    Utils::Instance().logger("segment exchange changed: " + std::to_string(best->improvement), Utils::VERBOSE);
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

/** @brief Remove related customer clusters and rebuild them with regret insertion.
 *
 * For each high-impact seed customer, this removes the seed plus its closest
 * route-set neighbors, then reinserts that small cluster by regret-2 insertion.
 * The move is a deterministic large-neighborhood search step aimed at fixing
 * route membership errors that single relocations and swaps cannot cross.
 * @param[in] routes The routes to edit
 * @param[in] removalCount Number of related customers removed together
 * @param[in] seedLimit Number of high-contribution seed customers considered
 * @return Positive cost reduction if routes are improved, otherwise -1
 */
int OptimalMove::OptRelatedRuinRecreate(Routes& routes, int removalCount, int seedLimit) {
    if (removalCount <= 1 || seedLimit <= 0 || routes.empty()) {
        Utils::Instance().logger("related ruin-recreate no improvement", Utils::VERBOSE);
        return -1;
    }
    const std::vector<std::vector<Customer>> removalSets = BuildRelatedRemovalSets(routes, removalCount, seedLimit);
    if (removalSets.empty()) {
        Utils::Instance().logger("related ruin-recreate no improvement", Utils::VERBOSE);
        return -1;
    }

    std::vector<RuinRecreateResult> candidates;
    std::mutex candidatesMutex;
    ThreadPool pool(this->cores);
    for (std::size_t sequence = 0; sequence < removalSets.size(); ++sequence) {
        pool.AddTask([&routes, &removalSets, sequence, &candidates, &candidatesMutex]() {
            std::optional<RuinRecreateResult> candidate =
                EvaluateRelatedRuinRecreate(routes, removalSets[sequence], sequence);
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
        Utils::Instance().logger("related ruin-recreate no improvement", Utils::VERBOSE);
        return -1;
    }
    routes = best->routes;
    this->CleanVoid(routes);
    Utils::Instance().logger("related ruin-recreate improved: " + std::to_string(best->improvement), Utils::VERBOSE);
    return best->improvement;
}

/** @brief Remove related clusters and rebuild them with bounded beam insertion.
 *
 * This is a stricter, more expensive companion to OptRelatedRuinRecreate. It
 * uses the same seed and removal-set construction, but evaluates several
 * route-assignment continuations for each removed cluster instead of committing
 * to the single regret insertion path.
 * @param[in] routes The routes to edit
 * @param[in] removalCount Number of related customers removed together
 * @param[in] seedLimit Number of seed customers considered
 * @param[in] beamWidth Number of partial reconstructions retained per insertion depth
 * @return Positive cost reduction if routes are improved, otherwise -1
 */
int OptimalMove::OptRelatedBeamRuinRecreate(Routes& routes, int removalCount, int seedLimit, int beamWidth) {
    if (removalCount <= 1 || seedLimit <= 0 || beamWidth <= 0 || routes.empty()) {
        Utils::Instance().logger("beam ruin-recreate no improvement", Utils::VERBOSE);
        return -1;
    }
    const std::vector<std::vector<Customer>> removalSets = BuildRelatedRemovalSets(routes, removalCount, seedLimit);
    if (removalSets.empty()) {
        Utils::Instance().logger("beam ruin-recreate no improvement", Utils::VERBOSE);
        return -1;
    }

    std::vector<RuinRecreateResult> candidates;
    std::mutex candidatesMutex;
    ThreadPool pool(this->cores);
    for (std::size_t sequence = 0; sequence < removalSets.size(); ++sequence) {
        pool.AddTask([&routes, &removalSets, sequence, beamWidth, &candidates, &candidatesMutex]() {
            std::optional<RuinRecreateResult> candidate =
                EvaluateRelatedBeamRuinRecreate(routes, removalSets[sequence], sequence, beamWidth);
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
        Utils::Instance().logger("beam ruin-recreate no improvement", Utils::VERBOSE);
        return -1;
    }
    routes = best->routes;
    this->CleanVoid(routes);
    Utils::Instance().logger("beam ruin-recreate improved: " + std::to_string(best->improvement), Utils::VERBOSE);
    return best->improvement;
}

/** @brief Rebuild one related customer cluster away from its current routes.
 *
 * This is a diversification move, so it may accept a temporary cost increase.
 * Customers are selected exactly like related ruin-and-recreate, but insertion
 * prefers feasible routes different from each customer's origin. The following
 * strict VND pass decides whether the new basin contains a better local optimum.
 * @param[in] routes The routes to edit
 * @param[in] removalCount Number of related customers removed together
 * @param[in] seedLimit Number of high-contribution seed customers considered
 * @param[in] diversificationRank Candidate rank used to vary repeated stagnant rounds
 * @return Cost improvement if positive, or a negative cost increase when diversified
 */
int OptimalMove::PerturbRelatedRuinRecreate(Routes& routes, int removalCount, int seedLimit, int diversificationRank) {
    if (removalCount <= 1 || seedLimit <= 0 || routes.empty()) {
        Utils::Instance().logger("related perturbation no move", Utils::VERBOSE);
        return 0;
    }
    const std::vector<std::vector<Customer>> removalSets = BuildRelatedRemovalSets(routes, removalCount, seedLimit);
    if (removalSets.empty()) {
        Utils::Instance().logger("related perturbation no move", Utils::VERBOSE);
        return 0;
    }

    std::vector<RuinRecreateResult> candidates;
    std::mutex candidatesMutex;
    ThreadPool pool(this->cores);
    for (std::size_t sequence = 0; sequence < removalSets.size(); ++sequence) {
        pool.AddTask([&routes, &removalSets, sequence, &candidates, &candidatesMutex]() {
            std::optional<RuinRecreateResult> candidate =
                EvaluateRelatedPerturbation(routes, removalSets[sequence], sequence);
            if (!candidate.has_value()) {
                return;
            }
            std::scoped_lock lock(candidatesMutex);
            candidates.push_back(std::move(*candidate));
        });
    }
    pool.JoinAll();
    if (candidates.empty()) {
        Utils::Instance().logger("related perturbation no move", Utils::VERBOSE);
        return 0;
    }
    std::ranges::sort(candidates, [](const RuinRecreateResult& left, const RuinRecreateResult& right) {
        if (left.improvement != right.improvement) {
            return left.improvement > right.improvement;
        }
        return left.sequence < right.sequence;
    });
    const std::size_t selectedIndex = static_cast<std::size_t>(std::max(0, diversificationRank)) % candidates.size();
    RuinRecreateResult& selected = candidates[selectedIndex];
    routes = std::move(selected.routes);
    this->CleanVoid(routes);
    Utils::Instance().logger("related perturbation changed: " + std::to_string(selected.improvement), Utils::VERBOSE);
    return selected.improvement;
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

/** @brief Reassign a bounded boundary-customer pool between large route pairs.
 *
 * The move first ranks route pairs by geometric closeness, then tries a bounded
 * side reassignment of only boundary-like customers for the most promising
 * pairs. This gives large route pairs a route-membership neighborhood without
 * paying the cost of a full exact pair split.
 * @param[in] routes The routes to edit
 * @param[in] maxBoundaryCustomers Maximum movable customers in the pair pool
 * @param[in] pairLimit Maximum route pairs evaluated in this call
 * @return Positive cost reduction if routes are improved, otherwise -1
 */
int OptimalMove::OptBoundaryPairSplit(Routes& routes, int maxBoundaryCustomers, int pairLimit) {
    if (maxBoundaryCustomers <= 1 || pairLimit <= 0) {
        Utils::Instance().logger("boundary pair split no improvement", Utils::VERBOSE);
        return -1;
    }
    const std::vector<RouteSnapshot> snapshots = SnapshotRoutes(routes);
    std::vector<PairSplitCandidatePair> pairCandidates;
    pairCandidates.reserve((snapshots.size() * snapshots.size()) / 2);
    for (std::size_t sourceIndex = 0; sourceIndex < snapshots.size(); ++sourceIndex) {
        for (std::size_t destIndex = sourceIndex + 1; destIndex < snapshots.size(); ++destIndex) {
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
    if (std::cmp_less(pairLimit, pairCandidates.size())) {
        pairCandidates.resize(static_cast<std::size_t>(pairLimit));
    }

    std::vector<BoundaryPairSplit> candidates;
    std::mutex candidatesMutex;
    ThreadPool pool(this->cores);
    for (const PairSplitCandidatePair& pair : pairCandidates) {
        pool.AddTask([&snapshots, pair, maxBoundaryCustomers, &candidates, &candidatesMutex]() {
            std::optional<BoundaryPairSplit> candidate = FindBestBoundaryPairSplit(
                snapshots[pair.sourceIndex].route, snapshots[pair.destIndex].route, snapshots[pair.sourceIndex].index,
                snapshots[pair.destIndex].index, maxBoundaryCustomers);
            if (!candidate.has_value()) {
                return;
            }
            std::scoped_lock lock(candidatesMutex);
            candidates.push_back(std::move(*candidate));
        });
    }
    pool.JoinAll();
    const auto best =
        std::ranges::max_element(candidates, [](const BoundaryPairSplit& left, const BoundaryPairSplit& right) {
            if (left.improvement != right.improvement) {
                return left.improvement < right.improvement;
            }
            if (left.sourceIndex != right.sourceIndex) {
                return left.sourceIndex > right.sourceIndex;
            }
            return left.destIndex > right.destIndex;
        });
    if (best == candidates.end()) {
        Utils::Instance().logger("boundary pair split no improvement", Utils::VERBOSE);
        return -1;
    }
    Routes::iterator source = routes.begin();
    std::advance(source, best->sourceIndex);
    *source = best->source;
    Routes::iterator dest = routes.begin();
    std::advance(dest, best->destIndex);
    *dest = best->dest;
    this->CleanVoid(routes);
    Utils::Instance().logger("boundary pair split improved: " + std::to_string(best->improvement), Utils::VERBOSE);
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

/** @brief Repartition route pairs with circular sweep splits.
 *
 * This is a cheaper fallback for route pairs too large for exact PairSplit. It
 * pools both memberships, scans circular angular orders, splits them into two
 * capacity-feasible routes, and applies the best strict improvement.
 * @param[in] routes The routes to edit
 * @return Positive cost reduction if routes are improved, otherwise -1
 */
int OptimalMove::OptPairSweepSplit(Routes& routes) {
    const std::vector<RouteSnapshot> snapshots = SnapshotRoutes(routes);
    std::vector<PairSplit> candidates;
    std::mutex candidatesMutex;
    ThreadPool pool(this->cores);
    for (std::size_t sourceIndex = 0; sourceIndex < snapshots.size(); ++sourceIndex) {
        for (std::size_t destIndex = sourceIndex + 1; destIndex < snapshots.size(); ++destIndex) {
            pool.AddTask([&snapshots, sourceIndex, destIndex, &candidates, &candidatesMutex]() {
                std::optional<PairSplit> candidate =
                    FindBestPairSweepSplit(snapshots[sourceIndex].route, snapshots[destIndex].route,
                                           snapshots[sourceIndex].index, snapshots[destIndex].index);
                if (!candidate.has_value()) {
                    return;
                }
                std::scoped_lock lock(candidatesMutex);
                candidates.push_back(std::move(*candidate));
            });
        }
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
        Utils::Instance().logger("pair sweep split no improvement", Utils::VERBOSE);
        return -1;
    }
    Routes::iterator source = routes.begin();
    std::advance(source, best->sourceIndex);
    *source = best->source;
    Routes::iterator dest = routes.begin();
    std::advance(dest, best->destIndex);
    *dest = best->dest;
    this->CleanVoid(routes);
    Utils::Instance().logger("pair sweep split improved: " + std::to_string(best->improvement), Utils::VERBOSE);
    return best->improvement;
}

/** @brief Repartition geographically close route triples with circular sweep splits.
 *
 * This catches cyclic membership errors that no strictly improving pair move can
 * see. Candidate triples are built from nearest route neighborhoods so the move
 * stays focused on the geographic interactions it is meant to repair.
 * @param[in] routes The routes to edit
 * @param[in] maxBoundaryCustomers Maximum movable customers in each triple
 * @return Positive cost reduction if routes are improved, otherwise -1
 */
int OptimalMove::OptRouteClusterSplit(Routes& routes, int maxBoundaryCustomers) {
    if (maxBoundaryCustomers <= 0 || routes.size() < 3) {
        Utils::Instance().logger("route cluster split no improvement", Utils::VERBOSE);
        return -1;
    }
    constexpr int maxClusterBoundaryCustomers = 5;
    maxBoundaryCustomers = std::min(maxBoundaryCustomers, maxClusterBoundaryCustomers);
    constexpr std::size_t sweepToBoundarySizeMultiplier = 4;
    const std::vector<RouteSnapshot> snapshots = SnapshotRoutes(routes);
    std::vector<RouteClusterCandidate> clusterCandidates = BuildRouteClusterCandidates(snapshots);
    const std::size_t clusterLimit = std::min(clusterCandidates.size(), std::max<std::size_t>(8, snapshots.size() * 2));
    clusterCandidates.resize(clusterLimit);
    std::vector<RouteClusterSplit> candidates;
    std::mutex candidatesMutex;
    ThreadPool pool(this->cores);
    for (const RouteClusterCandidate& candidate : clusterCandidates) {
        pool.AddTask([&snapshots, candidate, maxBoundaryCustomers, &candidates, &candidatesMutex]() {
            const std::array<RouteSnapshot, 3> cluster = {
                snapshots[candidate.routeIndices[0]],
                snapshots[candidate.routeIndices[1]],
                snapshots[candidate.routeIndices[2]],
            };
            const std::size_t exactCustomerLimit = static_cast<std::size_t>(maxBoundaryCustomers) * cluster.size();
            std::optional<RouteClusterSplit> result;
            if (RouteClusterCustomerCount(cluster) <= exactCustomerLimit) {
                // Small triples are solved exactly because the subset DP can
                // evaluate every feasible three-way repartition cheaply enough.
                result = FindBestExactRouteClusterSplit(cluster, exactCustomerLimit);
            }
            if (!result.has_value() &&
                RouteClusterCustomerCount(cluster) <=
                    static_cast<std::size_t>(maxBoundaryCustomers) * sweepToBoundarySizeMultiplier) {
                // Medium triples fall back to angular splits, which are cheaper
                // but still repair broad geographic membership mistakes.
                result = FindBestRouteClusterSweepSplit(cluster);
            }
            std::optional<RouteClusterSplit> boundaryResult =
                FindBestRouteClusterBoundarySplit(cluster, maxBoundaryCustomers);
            if (boundaryResult.has_value() &&
                (!result.has_value() || boundaryResult->improvement > result->improvement)) {
                result = std::move(boundaryResult);
            }
            if (!result.has_value()) {
                return;
            }
            std::scoped_lock lock(candidatesMutex);
            candidates.push_back(std::move(*result));
        });
    }
    pool.JoinAll();
    const auto best =
        std::ranges::max_element(candidates, [](const RouteClusterSplit& left, const RouteClusterSplit& right) {
            if (left.improvement != right.improvement) {
                return left.improvement < right.improvement;
            }
            return left.indexes > right.indexes;
        });
    if (best == candidates.end()) {
        Utils::Instance().logger("route cluster split no improvement", Utils::VERBOSE);
        return -1;
    }
    for (std::size_t routeIndex = 0; routeIndex < best->routes.size(); ++routeIndex) {
        Routes::iterator target = routes.begin();
        std::advance(target, best->indexes[routeIndex]);
        *target = best->routes[routeIndex];
    }
    this->CleanVoid(routes);
    Utils::Instance().logger("route cluster split improved: " + std::to_string(best->improvement), Utils::VERBOSE);
    return best->improvement;
}

/** @brief Shift fixed-size customer groups around each route triple.
 *
 * Evaluates every route triple and both cyclic directions in parallel. The
 * group size lets this cover the single-customer case as well as small segment
 * exchanges that pairwise moves can miss.
 * @param[in] routes The routes to edit
 * @param[in] groupSize Number of customers moved from each route
 * @return Positive cost reduction if routes are improved, otherwise -1
 */
int OptimalMove::OptCyclicExchange(Routes& routes, int groupSize) {
    if (groupSize <= 0 || routes.size() < 3) {
        Utils::Instance().logger("cyclic exchange no improvement", Utils::VERBOSE);
        return -1;
    }
    const std::vector<RouteSnapshot> snapshots = SnapshotRoutes(routes);
    std::vector<RouteClusterSplit> candidates;
    std::mutex candidatesMutex;
    ThreadPool pool(this->cores);
    for (std::size_t first = 0; first + 2 < snapshots.size(); ++first) {
        for (std::size_t second = first + 1; second + 1 < snapshots.size(); ++second) {
            for (std::size_t third = second + 1; third < snapshots.size(); ++third) {
                pool.AddTask([&snapshots, first, second, third, groupSize, &candidates, &candidatesMutex]() {
                    const std::array<RouteSnapshot, 3> cluster = {
                        snapshots[first],
                        snapshots[second],
                        snapshots[third],
                    };
                    std::optional<RouteClusterSplit> candidate = FindBestCyclicExchange(cluster, groupSize);
                    if (!candidate.has_value()) {
                        return;
                    }
                    std::scoped_lock lock(candidatesMutex);
                    candidates.push_back(std::move(*candidate));
                });
            }
        }
    }
    pool.JoinAll();
    const auto best =
        std::ranges::max_element(candidates, [](const RouteClusterSplit& left, const RouteClusterSplit& right) {
            if (left.improvement != right.improvement) {
                return left.improvement < right.improvement;
            }
            return left.indexes > right.indexes;
        });
    if (best == candidates.end()) {
        Utils::Instance().logger("cyclic exchange no improvement", Utils::VERBOSE);
        return -1;
    }
    for (std::size_t routeIndex = 0; routeIndex < best->routes.size(); ++routeIndex) {
        Routes::iterator target = routes.begin();
        std::advance(target, best->indexes[routeIndex]);
        *target = best->routes[routeIndex];
    }
    this->CleanVoid(routes);
    Utils::Instance().logger("cyclic exchange improved: " + std::to_string(best->improvement), Utils::VERBOSE);
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

/** @brief Remove routes that can be fully inserted into other routes.
 *
 * Targets the smallest route first and deletes it only when all of its
 * customers can be inserted into another feasible route.
 * @param[in] routes The routes to edit
 * @return Number of routes removed
 */
int OptimalMove::ReduceRoutes(Routes& routes, std::size_t minimumRouteCount) {
    if (minimumRouteCount > 0 && routes.size() <= minimumRouteCount) {
        Utils::Instance().logger("Route reduction no improvement", Utils::VERBOSE);
        return 0;
    }
    int removedRoutes = 0;
    while (true) {
        const bool routeCountAboveLowerBound = minimumRouteCount > 0 && routes.size() > minimumRouteCount;
        std::optional<RouteReduction> best;
        std::vector<std::size_t> sourceIndices;
        sourceIndices.reserve(routes.size());
        for (std::size_t index = 0; index < routes.size(); ++index) {
            if (routes[index].size() > 2) {
                sourceIndices.push_back(index);
            }
        }
        std::ranges::sort(sourceIndices, [&routes](std::size_t left, std::size_t right) {
            if (routes[left].size() != routes[right].size()) {
                return routes[left].size() < routes[right].size();
            }
            if (routes[left].GetTotalCost() != routes[right].GetTotalCost()) {
                return routes[left].GetTotalCost() < routes[right].GetTotalCost();
            }
            return left < right;
        });
        // When the solution uses too many routes, scan every active route as a
        // deletion source. A bounded shortlist is faster, but it can miss the
        // only route whose customers can be absorbed by the rest of the solution.
        for (const std::size_t sourceIndex : sourceIndices) {
            std::optional<RouteReduction> candidate =
                EvaluateRouteRemoval(routes, sourceIndex, routeCountAboveLowerBound);
            if (!candidate.has_value()) {
                continue;
            }
            if (!best.has_value() || candidate->costDelta < best->costDelta ||
                (candidate->costDelta == best->costDelta && candidate->removedRouteIndex < best->removedRouteIndex)) {
                best = std::move(candidate);
            }
        }
        if (!best.has_value()) {
            break;
        }
        const bool improvesCost = best->costDelta <= 0;
        if (!routeCountAboveLowerBound && !improvesCost) {
            break;
        }
        routes = std::move(best->routes);
        ++removedRoutes;
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
