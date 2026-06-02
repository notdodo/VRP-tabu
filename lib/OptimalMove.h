#ifndef OptimalMove_H
#define OptimalMove_H

#include "Route.h"
#include "Utils.h"
#include "../lib/ThreadPool.h"
#include <algorithm>
#include <mutex>
#include <thread>

/** @brief Candidate replacement for one ordered route pair. */
struct BestResult {
    int sourceIndex;
    int destIndex;
    int originalCost;
    Route source;
    Route dest;

    /** @brief Return the combined cost after applying the replacement routes. */
    [[nodiscard]] int NewCost() const { return source.GetTotalCost() + dest.GetTotalCost(); }

    /** @brief Return the total cost reduction produced by this candidate. */
    [[nodiscard]] int Improvement() const { return originalCost - NewCost(); }
};

/** @brief Collection of local-search neighborhoods for improving VRP routes.
 *
 * Each public method applies one move family to the route set and returns the
 * cost improvement when applicable. Inter-route neighborhoods are evaluated in
 * parallel where route pairs can be considered independently.
 */
class OptimalMove {
  private:
    std::mutex mtx;
    const unsigned cores;

    /** @brief Return a route with the segment between two customers reversed. */
    Route Opt2Swap(Route, const Customer&, const Customer&);

    /** @brief Return a route modified by one 3-opt reconnection candidate. */
    Route Opt3Swap(Route, const Customer&, const Customer&, const Customer&, const Customer&);

    /** @brief Move one customer from source to destination when feasible or forced. */
    bool Move1FromTo(Route&, Route&, bool);

    /** @brief Swap one customer between two routes when feasible or forced. */
    bool SwapFromTo(Route&, Route&, bool);

    /** @brief Exchange variable-size customer groups between two routes. */
    bool AddRemoveFromTo(Route&, Route&, int, int, bool);

  public:
    /** @brief Create a move engine sized to the available hardware concurrency. */
    OptimalMove() : cores(std::max(1U, std::thread::hardware_concurrency())) {};

    /** @brief Remove routes that contain no customers. */
    void CleanVoid(Routes&);

    /** @brief Apply best one-customer source-to-destination relocation. */
    int Opt10(Routes&, bool);

    /** @brief Apply best one-for-one customer swap between routes. */
    int Opt11(Routes&, bool);

    /** @brief Apply best two-from-source one-from-destination exchange. */
    int Opt21(Routes&, bool);

    /** @brief Apply best one-from-source two-from-destination exchange. */
    int Opt12(Routes&, bool);

    /** @brief Apply best two-for-two exchange between routes. */
    int Opt22(Routes&, bool);

    /** @brief Apply best n-for-m customer exchange between route pairs. */
    int OptExchange(Routes&, int, int, bool);

    /** @brief Swap contiguous customer segments between routes. */
    int OptSwapSegments(Routes&, int, bool force = false);

    /** @brief Remove costly customers and reinsert them with best insertion. */
    int OptRuinRecreate(Routes&, int, int);

    /** @brief Remove related customer clusters and reinsert them by regret. */
    int OptRelatedRuinRecreate(Routes&, int, int);

    /** @brief Remove related customer clusters and reinsert them with a small beam search. */
    int OptRelatedBeamRuinRecreate(Routes&, int, int, int);

    /** @brief Diversify by rebuilding a related customer cluster in different routes. */
    int PerturbRelatedRuinRecreate(Routes&, int, int, int);

    /** @brief Diversify by rebuilding an angular sector in different routes. */
    int PerturbAngularRuinRecreate(Routes&, int, int);

    /** @brief Apply 2-opt* by exchanging route tails between two routes. */
    int Opt2Star(Routes&);

    /** @brief Reassign a bounded boundary-customer pool between large route pairs. */
    int OptBoundaryPairSplit(Routes&, int, int);

    /** @brief Repartition bounded route pairs with exact per-route ordering. */
    int OptPairSplit(Routes&, int);

    /** @brief Repartition large route pairs with sweep splits and 2-opt ordering. */
    int OptPairSweepSplit(Routes&);

    /** @brief Repartition geographically close route triples with sweep splits. */
    int OptRouteClusterSplit(Routes&, int);

    /** @brief Apply best fixed-size cyclic exchange across route triples. */
    int OptCyclicExchange(Routes&, int);

    /** @brief Relocate a contiguous customer segment to another route. */
    int OptRelocateSegment(Routes&, int);

    /** @brief Optimize small individual routes as TSP subproblems. */
    int OptRouteTsp(Routes&, int);

    /** @brief Remove routes by reinserting all customers into the remaining routes. */
    int ReduceRoutes(Routes&, std::size_t minimumRouteCount = 0);

    /** @brief Apply intra-route 2-opt improvements. */
    bool Opt2(Routes&);

    /** @brief Apply intra-route 3-opt improvements. */
    bool Opt3(Routes&);
};

#endif /* OptimalMove_H */
