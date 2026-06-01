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
#include <cmath>
#include <map>
#include <mutex>
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
    Move move;
};

/** @brief Apply a two-route candidate to a full route set only when needed. */
Routes ApplyTabuCandidate(const Routes& base, const TabuCandidateResult& candidate) {
    Routes routes = base;
    routes[static_cast<std::size_t>(candidate.sourceRouteIndex)] = candidate.sourceRoute;
    routes[static_cast<std::size_t>(candidate.destRouteIndex)] = candidate.destRoute;
    std::erase_if(routes, [](const Route& route) { return route.size() <= 2; });
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
    // Scan a nearest-neighbor subset; candidate evaluation is parallel.
    const int neighborsToEvaluate = std::max(1, this->numCustomers / static_cast<int>(routes.size()));
    // Work on a local route set so failed or exploratory moves never mutate the caller.
    Routes s = routes;
    Routes sbest = routes;
    auto nonBestComp = [](const std::pair<float, Routes>& l, const std::pair<float, Routes>& r) -> bool {
        return l.first < r.first;
    };
    std::set<std::pair<float, Routes>, decltype(nonBestComp)> nonBest(nonBestComp);
    float bestFitness = 0;
    int iterations = 0;
    while (iterations < times) {
        iterations++;
        // create a local working copy of solutions
        const float currentFitness = this->Evaluate(s);
        const float diversificationScale =
            std::sqrt(static_cast<float>(this->numCustomers) * static_cast<float>(s.size()));
        float fitnessBestCandidate = currentFitness;
        Routes bestCandidate = s;
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
                });
                // Reserve a stable sequence range for this job before it runs on a worker.
                sequenceStart += static_cast<std::size_t>(neighborsToEvaluate);
            }
            ++routeIndex;
        }

        std::mutex candidateMutex;
        std::vector<TabuCandidateResult> candidateResults;
        candidateResults.reserve(sequenceStart);
        this->graph->PrepareNeighborhoods();
        ThreadPool pool(std::thread::hardware_concurrency());
        for (const TabuCandidateJob& job : candidateJobs) {
            pool.AddTask([this, &s, &customerRouteIndex, currentFitness, diversificationScale, &candidateMutex,
                          &candidateResults, job]() {
                std::vector<TabuCandidateResult> localResults;
                const CostNeighborhood& neigh = this->graph->GetNeighborhoodVector(job.anchorCustomer);
                auto in = neigh.cbegin();
                for (int i = 0; in != neigh.cend() && i < job.neighborhoodLimit; ++in, ++i) {
                    float penalization = 0.0F;
                    const Route& baseDestRoute = s[static_cast<std::size_t>(job.destRouteIndex)];
                    if (baseDestRoute.FindCustomer(in->second)) {
                        continue;
                    }
                    const auto sourceRoute = customerRouteIndex.find(in->second);
                    if (sourceRoute == customerRouteIndex.end()) {
                        continue;
                    }
                    const int sourceRouteIndex = sourceRoute->second;
                    if (sourceRouteIndex < 0 || sourceRouteIndex == job.destRouteIndex) {
                        continue;
                    }
                    const Route& baseSourceRoute = s[static_cast<std::size_t>(sourceRouteIndex)];
                    Route candidateSourceRoute = baseSourceRoute;
                    Route candidateDestRoute = baseDestRoute;
                    const Move move = {{in->second, job.destRouteIndex}, sourceRouteIndex};
                    const float previousDestFitness = baseDestRoute.Evaluate();
                    // Aspiration rule: reject a tabu move only when it also worsens the destination route.
                    if (candidateSourceRoute.RemoveCustomer(in->second) && candidateDestRoute.AddElem(in->second) &&
                        !(this->tabulist.Find(move) && candidateDestRoute.Evaluate() > previousDestFitness)) {
                        penalization += this->tabulist.Check(move);
                        const float assessment = currentFitness - baseSourceRoute.Evaluate() -
                                                 baseDestRoute.Evaluate() + candidateSourceRoute.Evaluate() +
                                                 candidateDestRoute.Evaluate();
                        const float weightedPenalization =
                            this->lambda * currentFitness * diversificationScale * penalization;
                        localResults.emplace_back(TabuCandidateResult{
                            .sequence = job.sequenceStart + static_cast<std::size_t>(i),
                            .assessment = assessment,
                            .penalizationScore = weightedPenalization,
                            .score = assessment + weightedPenalization,
                            .sourceRoute = std::move(candidateSourceRoute),
                            .destRoute = std::move(candidateDestRoute),
                            .sourceRouteIndex = sourceRouteIndex,
                            .destRouteIndex = job.destRouteIndex,
                            .move = move,
                        });
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
                bestCandidate = ApplyTabuCandidate(s, candidate);
                bestMoves = {candidate.move};
                hasImprovement = true;
            } else {
                Routes candidateRoutes = ApplyTabuCandidate(s, candidate);
                nonBest.insert({candidate.assessment, std::move(candidateRoutes)});
                while (nonBest.size() > maxNonBest) {
                    nonBest.erase(nonBest.begin());
                }
                diverParam += candidate.penalizationScore;
            }
        }
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
        std::ranges::for_each(
            bestMoves, [this, tabuTime, diverParam](Move& m) { this->tabulist.AddTabu(m, tabuTime + diverParam / 2); });
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
