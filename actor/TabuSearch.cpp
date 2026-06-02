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
#include "../lib/ThreadPool.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iterator>
#include <list>
#include <map>
#include <mutex>
#include <optional>
#include <ranges>
#include <thread>
#include <utility>
#include <vector>

namespace {
/** @brief Candidate-generation work unit for one destination customer.
 *
 * A job keeps the route that may receive a customer, the customer whose
 * neighborhood is scanned, and the sequence offset used to restore the original
 * sequential scan order after parallel evaluation.
 */
struct TabuCandidateJob {
    int destRouteIndex;
    Customer anchorCustomer;
    std::size_t sequenceStart;
    int neighborhoodLimit;
    int segmentNeighborhoodLimit;
};

/** @brief Fully evaluated tabu candidate move.
 *
 * Worker threads only create these immutable results. The main tabu loop later
 * reduces them sequentially, preserving deterministic best-candidate selection
 * and keeping all tabu-list mutations on the owner thread.
 */
struct TabuCandidateResult {
    std::size_t sequence;
    float assessment;
    float penalizationScore;
    float score;
    Route sourceRoute;
    Route destRoute;
    int sourceRouteIndex;
    int destRouteIndex;
    std::vector<Move> moves;
};

// Tabu candidate breadth. Or-opt conventionally relocates strings of length 1..3;
// the variant count reserves deterministic sequence slots for one single-customer
// relocation plus the two- and three-customer segment candidates.
constexpr int kMaxTabuOrOptLength = 3;
constexpr int kTabuCandidateVariants = 1 + 2 + 3;

// Segment relocations are more expensive than single-customer moves, so only a
// focused fraction of each nearest-neighbor candidate list is expanded to them.
constexpr int kTabuSegmentNeighborDivisor = 3;

// Per-call tabu budget. The lower bound avoids tiny calls on small instances;
// the route/customer term lets larger route sets spend more time exploring.
constexpr int kMinTabuCallMilliseconds = 1000;
constexpr int kTabuMillisecondsPerCustomerRoute = 4;

// Cap the extra tenure produced by rejected-candidate pressure so diversification
// cannot freeze too much of the neighborhood after one noisy iteration.
constexpr float kMaxDiversificationTenureMultiplier = 1.0F;

/** @brief Return non-depot customers from a route in route order. */
std::vector<Customer> RouteCustomersWithoutDepot(const Route& route) {
    std::vector<Customer> customers;
    customers.reserve(route.GetRoute()->size());
    const Customer& depot = route.GetRoute()->front().first;
    for (const StepType& step : *route.GetRoute()) {
        if (step.first != depot) {
            customers.push_back(step.first);
        }
    }
    return customers;
}

/** @brief Build Or-opt segment candidates that contain a selected customer. */
std::vector<std::list<Customer>> BuildTabuSegmentsContaining(const Route& route, const Customer& customer,
                                                             int maxSegmentLength) {
    const std::vector<Customer> customers = RouteCustomersWithoutDepot(route);
    const auto found = std::ranges::find(customers, customer);
    if (found == customers.cend()) {
        return {};
    }
    const std::size_t customerIndex = static_cast<std::size_t>(std::distance(customers.cbegin(), found));
    std::vector<std::list<Customer>> segments;
    for (int length = 2; length <= maxSegmentLength; ++length) {
        const std::size_t segmentLength = static_cast<std::size_t>(length);
        if (customers.size() < segmentLength) {
            continue;
        }
        const std::size_t firstStart = customerIndex >= segmentLength - 1 ? customerIndex - (segmentLength - 1) : 0;
        const std::size_t lastStart = std::min(customerIndex, customers.size() - segmentLength);
        // Enumerate every route-order segment that contains the anchor customer;
        // this gives tabu an Or-opt move while keeping the candidate count bounded.
        for (std::size_t start = firstStart; start <= lastStart; ++start) {
            std::list<Customer> segment;
            for (std::size_t offset = 0; offset < segmentLength; ++offset) {
                segment.push_back(customers[start + offset]);
            }
            segments.push_back(std::move(segment));
        }
    }
    return segments;
}

/** @brief Check tabu memory and aspiration for a set of customer-route attributes. */
bool IsTabuWithoutAspiration(const std::vector<Move>& moves, const TabuList& tabulist, float assessment,
                             float bestFitness) {
    const bool tabu = std::ranges::any_of(moves, [&tabulist](const Move& move) { return tabulist.Find(move); });
    return tabu && assessment >= bestFitness;
}

/** @brief Sum tabu penalization for accepted move attributes. */
float MovePenalization(const std::vector<Move>& moves, const TabuList& tabulist) {
    float penalization = 0.0F;
    for (const Move& move : moves) {
        penalization += tabulist.Check(move);
    }
    return penalization;
}

/** @brief Evaluate one tabu relocation or Or-opt segment relocation candidate. */
std::optional<TabuCandidateResult> EvaluateTabuSegmentCandidate(const Routes& baseRoutes, const TabuList& tabulist,
                                                                float lambda, float currentFitness, float bestFitness,
                                                                float diversificationScale, int sourceRouteIndex,
                                                                int destRouteIndex, const std::list<Customer>& segment,
                                                                std::size_t sequence) {
    const Route& baseSourceRoute = baseRoutes[static_cast<std::size_t>(sourceRouteIndex)];
    const Route& baseDestRoute = baseRoutes[static_cast<std::size_t>(destRouteIndex)];
    Route candidateSourceRoute = baseSourceRoute;
    std::vector<Move> moves;
    moves.reserve(segment.size());
    // Remove the whole segment first so destination insertion tests the true
    // post-removal capacities and route costs.
    for (const Customer& customer : segment) {
        if (!candidateSourceRoute.RemoveCustomer(customer)) {
            return std::nullopt;
        }
        moves.push_back({{customer, destRouteIndex}, sourceRouteIndex});
    }

    Route candidateDestRoute = baseDestRoute;
    if (!candidateDestRoute.AddElem(segment)) {
        return std::nullopt;
    }
    // Score only the two changed routes against the already known full-solution
    // fitness; this avoids rebuilding the whole route set for every candidate.
    const float assessment = currentFitness - baseSourceRoute.Evaluate() - baseDestRoute.Evaluate() +
                             candidateSourceRoute.Evaluate() + candidateDestRoute.Evaluate();
    if (IsTabuWithoutAspiration(moves, tabulist, assessment, bestFitness)) {
        return std::nullopt;
    }
    // Penalization is added after feasibility and aspiration, so tabu memory
    // changes ranking without hiding a globally improving move.
    const float penalization = MovePenalization(moves, tabulist);
    const float weightedPenalization = lambda * currentFitness * diversificationScale * penalization;
    return TabuCandidateResult{
        .sequence = sequence,
        .assessment = assessment,
        .penalizationScore = weightedPenalization,
        .score = assessment + weightedPenalization,
        .sourceRoute = std::move(candidateSourceRoute),
        .destRoute = std::move(candidateDestRoute),
        .sourceRouteIndex = sourceRouteIndex,
        .destRouteIndex = destRouteIndex,
        .moves = std::move(moves),
    };
}

/** @brief Apply a two-route candidate to a full route set only when needed. */
Routes ApplyTabuCandidate(const Routes& base, const TabuCandidateResult& candidate) {
    Routes routes = base;
    routes[static_cast<std::size_t>(candidate.sourceRouteIndex)] = candidate.sourceRoute;
    routes[static_cast<std::size_t>(candidate.destRouteIndex)] = candidate.destRoute;
    return routes;
}

} // namespace

/** @brief Search for a better solution and update the tabu list.
 *
 * Runs an iterated tabu search over the current routes. Each iteration builds
 * independent customer-relocation candidates, evaluates those candidates in
 * parallel, then reduces the results in the original scan order so equal-score
 * decisions remain deterministic. The tabu list is intentionally read during
 * candidate evaluation but updated only after the parallel workers finish.
 *
 * If no improving candidate is found, the search diversifies by selecting one
 * of the tracked non-best candidates and adjusting the tabu tenure.
 */
void TabuSearch::Tabu(Routes& routes, int times) {
    if (routes.empty()) {
        return;
    }
    constexpr std::size_t maxNonBest = 20;
    const float tabuTime = static_cast<float>(this->numCustomers) * 0.70F;
    const auto startedAt = std::chrono::steady_clock::now();
    const auto maxCallDuration = std::chrono::milliseconds(
        std::max(kMinTabuCallMilliseconds,
                 this->numCustomers * static_cast<int>(routes.size()) * kTabuMillisecondsPerCustomerRoute));
    // Scan a route-shape-sized nearest-neighbor subset; candidate evaluation is parallel.
    const int averageRouteCustomers =
        std::max(1, (this->numCustomers + static_cast<int>(routes.size()) - 1) / static_cast<int>(routes.size()));
    const int spatialSpread = static_cast<int>(std::sqrt(static_cast<float>(this->numCustomers))) / 2;
    const int neighborsToEvaluate = std::min(this->numCustomers - 1, averageRouteCustomers + spatialSpread);
    const int segmentNeighborsToEvaluate = std::max(1, neighborsToEvaluate / kTabuSegmentNeighborDivisor);
    // Work on a local route set so failed or exploratory moves never mutate the caller.
    Routes s = routes;
    Routes sbest = routes;
    auto nonBestComp = [](const std::pair<float, Routes>& l, const std::pair<float, Routes>& r) -> bool {
        return l.first < r.first;
    };
    std::set<std::pair<float, Routes>, decltype(nonBestComp)> nonBest(nonBestComp);
    float bestFitness = 0;
    int iterations = 0;
    while (iterations < times && std::chrono::steady_clock::now() - startedAt < maxCallDuration) {
        iterations++;
        // create a local working copy of solutions
        const float currentFitness = this->Evaluate(s);
        const float diversificationScale =
            std::sqrt(static_cast<float>(this->numCustomers) * static_cast<float>(s.size()));
        float fitnessBestCandidate = currentFitness;
        const TabuCandidateResult* bestCandidateResult = nullptr;
        std::vector<Move> bestMoves;
        bool hasImprovement = false;
        float diverParam = 0;

        std::vector<TabuCandidateJob> candidateJobs;
        std::map<Customer, int> customerRouteIndex;
        std::size_t sequenceStart = 0;
        int routeIndex = 0;
        for (const Route& route : s) {
            if (route.size() <= 2) {
                ++routeIndex;
                continue;
            }
            RouteList::const_iterator itc = route.GetRoute()->cbegin();
            const Customer depot = route.GetRoute()->front().first;
            for (++itc; itc->first != depot; ++itc) {
                customerRouteIndex.emplace(itc->first, routeIndex);
                candidateJobs.emplace_back(TabuCandidateJob{
                    .destRouteIndex = routeIndex,
                    .anchorCustomer = itc->first,
                    .sequenceStart = sequenceStart,
                    .neighborhoodLimit = neighborsToEvaluate,
                    .segmentNeighborhoodLimit = segmentNeighborsToEvaluate,
                });
                // Reserve stable sequence slots before workers run; this keeps
                // tie-breaking reproducible despite parallel candidate discovery.
                sequenceStart += static_cast<std::size_t>(neighborsToEvaluate * kTabuCandidateVariants);
            }
            ++routeIndex;
        }

        std::mutex candidateMutex;
        std::vector<TabuCandidateResult> candidateResults;
        candidateResults.reserve(sequenceStart);
        this->graph->PrepareNeighborhoods();
        const unsigned workerCount = std::max(1U, std::thread::hardware_concurrency());
        ThreadPool pool(workerCount);
        const std::size_t chunkSize =
            std::max<std::size_t>(1, (candidateJobs.size() + static_cast<std::size_t>(workerCount) - 1) /
                                         static_cast<std::size_t>(workerCount));
        // Chunk jobs instead of launching one task per customer; this lowers
        // thread-pool overhead on large instances with many candidate anchors.
        for (std::size_t chunkStart = 0; chunkStart < candidateJobs.size(); chunkStart += chunkSize) {
            const std::size_t chunkEnd = std::min(candidateJobs.size(), chunkStart + chunkSize);
            pool.AddTask([this, &s, &customerRouteIndex, currentFitness, bestFitness, diversificationScale,
                          &candidateMutex, &candidateResults, &candidateJobs, chunkStart, chunkEnd]() {
                std::vector<TabuCandidateResult> localResults;
                for (std::size_t jobIndex = chunkStart; jobIndex < chunkEnd; ++jobIndex) {
                    const TabuCandidateJob& job = candidateJobs[jobIndex];
                    const CostNeighborhood& neigh = this->graph->GetNeighborhoodVector(job.anchorCustomer);
                    auto in = neigh.cbegin();
                    int evaluatedNeighbors = 0;
                    for (; in != neigh.cend() && evaluatedNeighbors < job.neighborhoodLimit; ++in) {
                        const auto sourceRoute = customerRouteIndex.find(in->second);
                        if (sourceRoute == customerRouteIndex.end()) {
                            continue;
                        }
                        const int sourceRouteIndex = sourceRoute->second;
                        if (sourceRouteIndex < 0 || sourceRouteIndex == job.destRouteIndex) {
                            continue;
                        }
                        const std::size_t sequence =
                            job.sequenceStart + (static_cast<std::size_t>(evaluatedNeighbors) * kTabuCandidateVariants);
                        ++evaluatedNeighbors;
                        std::list<Customer> singleCustomer = {in->second};
                        std::optional<TabuCandidateResult> singleCandidate = EvaluateTabuSegmentCandidate(
                            s, this->tabulist, this->lambda, currentFitness, bestFitness, diversificationScale,
                            sourceRouteIndex, job.destRouteIndex, singleCustomer, sequence);
                        if (singleCandidate.has_value()) {
                            localResults.push_back(std::move(*singleCandidate));
                        }
                        if (evaluatedNeighbors > job.segmentNeighborhoodLimit) {
                            continue;
                        }
                        const std::vector<std::list<Customer>> segments = BuildTabuSegmentsContaining(
                            s[static_cast<std::size_t>(sourceRouteIndex)], in->second, kMaxTabuOrOptLength);
                        std::size_t variant = 1;
                        for (const std::list<Customer>& segment : segments) {
                            std::optional<TabuCandidateResult> segmentCandidate = EvaluateTabuSegmentCandidate(
                                s, this->tabulist, this->lambda, currentFitness, bestFitness, diversificationScale,
                                sourceRouteIndex, job.destRouteIndex, segment, sequence + variant);
                            if (segmentCandidate.has_value()) {
                                localResults.push_back(std::move(*segmentCandidate));
                            }
                            // Variants after the single-customer slot preserve a
                            // deterministic ordering among segment sizes/starts.
                            ++variant;
                        }
                    }
                }
                if (!localResults.empty()) {
                    std::scoped_lock lock(candidateMutex);
                    for (TabuCandidateResult& result : localResults) {
                        candidateResults.push_back(std::move(result));
                    }
                }
            });
        }
        pool.JoinAll();

        // Worker completion order is nondeterministic; sequence restores the old serial scan order.
        std::ranges::sort(candidateResults, [](const TabuCandidateResult& left, const TabuCandidateResult& right) {
            return left.sequence < right.sequence;
        });
        for (const TabuCandidateResult& candidate : candidateResults) {
            if (!hasImprovement || candidate.score < fitnessBestCandidate) {
                fitnessBestCandidate = candidate.score;
                bestCandidateResult = &candidate;
                bestMoves = candidate.moves;
                hasImprovement = true;
            } else {
                // Keep a small set of worse feasible candidates as restart
                // points; they deliberately preserve diversity, not quality.
                const bool nonBestHasRoom = nonBest.size() < maxNonBest;
                const bool candidateCanSurvive = !nonBest.empty() && candidate.assessment > nonBest.begin()->first;
                if (nonBestHasRoom || candidateCanSurvive) {
                    Routes candidateRoutes = ApplyTabuCandidate(s, candidate);
                    nonBest.insert({candidate.assessment, std::move(candidateRoutes)});
                    while (nonBest.size() > maxNonBest) {
                        nonBest.erase(nonBest.begin());
                    }
                }
                diverParam += candidate.penalizationScore;
            }
        }
        if (!hasImprovement || bestCandidateResult == nullptr) {
            break;
        }
        Routes bestCandidate = ApplyTabuCandidate(s, *bestCandidateResult);
        s = bestCandidate;
        std::ranges::sort(bestMoves);
        const auto uniqueSubrange = std::ranges::unique(bestMoves);
        // Parallel evaluation can discover the same move more than once through different anchors.
        bestMoves.erase(uniqueSubrange.begin(), uniqueSubrange.end());
        const float bestFitnessCandidate = this->Evaluate(bestCandidate);
        if (bestFitnessCandidate < bestFitness || bestFitness == 0) {
            sbest = bestCandidate;
            bestFitness = bestFitnessCandidate;
        } else if (bestFitnessCandidate == bestFitness && !nonBest.empty()) {
            auto nb = nonBest.begin();
            // The set is sorted by assessment; the last entry is deliberately more divergent.
            std::advance(nb, nonBest.size() - 1);
            s = nb->second;
            nonBest.erase(nb);
        }
        const float diversificationTenure = std::min(diverParam / 2.0F, tabuTime * kMaxDiversificationTenureMultiplier);
        std::ranges::for_each(bestMoves, [this, tabuTime, diversificationTenure](Move& m) {
            this->tabulist.AddTabu(m, tabuTime + diversificationTenure);
        });
        this->tabulist.Clean();
    }
    if (this->Evaluate(sbest) == this->Evaluate(routes) && !nonBest.empty()) {
        auto nb = nonBest.begin();
        // No strict improvement was found, so restart from a worse stored candidate to diversify.
        std::advance(nb, nonBest.size() - 1);
        routes = nb->second;
        nonBest.erase(nb);
        this->tabulist.IncrementSize();
    } else {
        routes = sbest;
        this->tabulist.DecrementSize();
    }
    std::erase_if(routes, [](const Route& route) { return route.size() <= 2; });
}

/** @brief Evaluate the assessment of the solution.
 *
 * This function assess the quality of the solution
 */
float TabuSearch::Evaluate(const Routes& l) {
    float tot = 0;
    for (const Route& route : l) {
        tot += route.Evaluate();
    }
    return tot;
}
