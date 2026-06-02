#ifndef TabuSearch_H
#define TabuSearch_H

#include "Route.h"
#include "TabuList.h"
#include <set>

/** @brief Tabu-search metaheuristic over a set of vehicle routes.
 *
 * The search explores customer relocation moves, scores them with route cost
 * and diversification penalties, and uses TabuList to avoid short cycling.
 */
class TabuSearch {
  private:
    const Graph* graph;
    TabuList tabulist;      /**< List of all tabu moves */
    int numCustomers;       /**< Number of customers */
    float lambda = 0.0001f; /**< Parameter for penalization of moves */

    /** @brief Evaluate a route set using the tabu-search objective. */
    float Evaluate(const Routes&);

  public:
    /** @brief Create a tabu-search engine for a graph and customer count. */
    TabuSearch(const Graph& g, const int n) : graph(&g), numCustomers(n) {};

    /** @brief Improve routes by running tabu search for a fixed number of iterations. */
    void Tabu(Routes&, int);
};

#endif /* TabuSearch_H */
