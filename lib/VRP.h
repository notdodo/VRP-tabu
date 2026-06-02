#ifndef VRP_H
#define VRP_H

#include "Graph.h"
#include "OptimalMove.h"
#include "TabuSearch.h"
#include <optional>

/** @brief Vehicle Routing Problem model and solver orchestration.
 *
 * VRP owns the graph, current solution, best solution, and search parameters.
 * It builds the initial solution, applies local-search neighborhoods, and runs
 * tabu search while preserving the incumbent best route set.
 */
class VRP {
  private:
    Graph graph;                          /**< Graph of customers */
    Routes routes;                        /**< Vector of all active routes */
    int numVertices = 0;                  /**< Number of customers */
    int vehicles = 0;                     /**< Number of vehicles */
    int capacity = 0;                     /**< Capacity of each vehicle */
    int minimumRoutes = 0;                /**< Capacity lower bound for the number of required routes */
    float workTime = 0.0F;                /**< Work time for each driver */
    float costTravel = 0.0F;              /**< Cost parameter for each travel */
    float alphaParam = 0.0F;              /**< Alpha parameter for route evaluation */
    Routes bestRoutes;                    /**< Best route configuration found so far */
    Routes routeArchive;                  /**< Routes seen during search for recombination */
    std::optional<TabuSearch> tabuSearch; /**< Persistent tabu memory across outer search iterations */
    int freshTabuRestartsUsed = 0;        /**< Number of bounded incumbent restarts already consumed */
    int totalCost = 0;                    /**< Total cost of routes */

    /** @brief Store route candidates from a complete solution for later recombination. */
    void ArchiveRoutes(const Routes&);

    /** @brief Recombine archived route candidates into a better complete solution. */
    bool RecombineArchivedRoutes();

  public:
    /** @brief Create an empty VRP object. */
    VRP() = default;

    /** @brief Create a VRP model from graph data and solver parameters. */
    VRP(Graph&&, const int, const int, const int, const int, const float, const bool, const float, const float);

    /** @brief Build an initial solution with savings and sweep heuristics. */
    int InitSolutionsSavings();

    /** @brief Run tabu search over the current routes. */
    void RunTabuSearch(int);

    /** @brief Run the configured sequence of local-search optimization moves.
     *
     * @return true when at least one move improved the solution.
     */
    bool RunOpts(int, bool, int);

    /** @brief Return the total cost of the current route set. */
    [[nodiscard]] int GetTotalCost();

    /** @brief Return the number of non-depot customers in the instance. */
    [[nodiscard]] int GetNumberOfCustomers() const;

    /** @brief Return a mutable pointer to the current route set. */
    [[nodiscard]] Routes* GetRoutes();

    /** @brief Return a mutable pointer to the best route set found so far. */
    [[nodiscard]] Routes* GetBestRoutes();

    /** @brief Replace the best solution when the current one is feasible and cheaper. */
    bool UpdateBest();

    /** @brief Restore the current route set from the best solution found so far. */
    void RestoreBest();

    /** @brief Restart once from the incumbent with fresh tabu memory. */
    bool RestartFromBestWithFreshTabu();

    /** @brief Try bounded incumbent-only diversification branches and keep only strict improvements. */
    bool IntensifyBestWithDiversifiedBranches(int, int);

    ~VRP() = default;
};

#endif /* VRP_H */
