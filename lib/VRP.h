#ifndef VRP_H
#define VRP_H

#include "Graph.h"
#include "Utils.h"
#include "TabuList.h"
#include "OptimalMove.h"
#include "TabuSearch.h"
#include "../lib/ThreadPool.h"

using Map = std::multimap<int, Customer>;

/** @brief Vehicle Routing Problem model and solver orchestration.
 *
 * VRP owns the graph, current solution, best solution, and search parameters.
 * It builds initial solutions, applies local-search neighborhoods, runs tabu
 * search, and validates that route sets keep every customer exactly once.
 */
class VRP {
  private:
    Graph graph;                  /**< Graph of customers */
    Routes routes;                /**< Vector of all active routes */
    int numVertices = 0;          /**< Number of customers */
    int vehicles = 0;             /**< Number of vehicles */
    int capacity = 0;             /**< Capacity of each vehicle */
    float workTime = 0.0F;        /**< Work time for each driver */
    float costTravel = 0.0F;      /**< Cost parameter for each travel */
    float alphaParam = 0.0F;      /**< Alpha parameter for route evaluation */
    float averageDistance = 0.0F; /**< Average distance of all routes */
    Routes bestRoutes;            /**< Best route configuration found so far */
    int totalCost = 0;            /**< Total cost of routes */

    /** @brief Verify route feasibility and customer coverage invariants. */
    bool CheckIntegrity();

    /** @brief Sort routes by cost to make later heuristics deterministic. */
    void OrderByCosts();

  public:
    /** @brief Create an empty VRP object. */
    VRP() = default;

    /** @brief Create a VRP model from graph data and solver parameters. */
    VRP(Graph&&, const int, const int, const int, const float, const bool, const float, const float);

    /** @brief Build the initial solution using the default constructor heuristic. */
    int InitSolutions();

    /** @brief Convert a shortest-path customer order into a route set candidate. */
    void CreateBest(const std::set<Customer>&, std::list<int>, const Customer&);

    /** @brief Build one nearest-neighbor route starting from a sorted-customer index. */
    int init(int);

    /** @brief Build an initial solution through repeated nearest-neighbor starts. */
    int InitSolutionsNeigh();

    /** @brief Build an initial solution with savings and sweep heuristics. */
    int InitSolutionsSavings();

    /** @brief Run tabu search over the current routes. */
    void RunTabuSearch(int);

    /** @brief Run the configured sequence of local-search optimization moves.
     *
     * @return true when at least one move improved the solution.
     */
    bool RunOpts(int, bool);

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

    ~VRP() = default;
};

#endif /* VRP_H */
