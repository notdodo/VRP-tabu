#ifndef Route_H
#define Route_H

#include "Graph.h"
#include <iomanip>
#include <list>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

using StepType = std::pair<Customer, int>;
using RouteList = std::vector<StepType>;

/** @brief Capacity- and time-constrained vehicle route.
 *
 * A route stores an ordered sequence of customer steps, including the depot at both
 * ends once closed. It owns the remaining capacity/work-time accounting and can
 * rebuild itself from a customer sequence while checking feasibility.
 */
class Route {
  private:
    /** @brief Print route cost and arc sequence. */
    friend std::ostream& operator<<(std::ostream& out, const Route& r) {
        if (r.route.size() > 1) {
            out << "Cost:" << std::setw(5) << std::to_string(r.totalCost) << " ";
            for (const auto& i : r.route)
                if (i.second > 0)
                    out << i.first << " -(" << std::to_string(i.second) << ")-> ";
                else
                    out << i.first;
        };
        return out;
    }
    int initialCapacity;   /**< Initial capacity of the route, equals to VRP.capacity */
    float initialWorkTime; /**< Total work time for driver, equals to VRP.workTime */
    int capacity;          /**< Capacity remaining */
    float workTime;        /**< Work time remaining */
    int totalCost;         /**< Total cost of the route: sum of the weight */
    float TRAVEL_COST;     /**< Cost parameter for each travel */
    float ALPHA;           /**< Alpha parameter for route evaluation */
    const Graph* graph;    /**< Shared immutable graph used for cost lookups */
  protected:
    RouteList route; /**< Ordered route steps */

    /** @brief Reset this route to a depot-only state. */
    void EmptyRoute(const Customer&);

  public:
    /** @brief Assign all route state, constraints, and shared graph pointer. */
    Route& operator=(const Route&) = default;

    /** @brief Check whether two routes visit a different customer sequence. */
    bool operator!=(const Route& r) const {
        if (this->route.size() != r.route.size()) {
            return true;
        }
        auto j = r.route.cbegin();
        for (auto& i : route) {
            if (i.first != j->first)
                return true;
            ++j;
        }
        return false;
    }

    /** @brief Check whether two routes visit the same customer sequence. */
    bool operator==(const Route& r) const {
        if (this->route.size() != r.route.size()) {
            return false;
        }
        auto i = route.cbegin();
        auto j = r.route.cbegin();
        for (; i != route.cend(); ++i) {
            if (i->first != j->first)
                return false;
            ++j;
        }
        return true;
    }

    /** @brief Order routes by increasing total cost. */
    bool operator<(const Route& r) const { return GetTotalCost() < r.GetTotalCost(); }

    /** @brief Compare routes by decreasing-or-equal total cost. */
    bool operator<=(const Route& r) const { return GetTotalCost() >= r.GetTotalCost(); }

    /** @brief Create an empty route with vehicle constraints and scoring parameters. */
    Route(const int, const float, const Graph&, const float, const float);

    Route(const Route&) = default;
    Route(Route&&) noexcept = default;
    Route& operator=(Route&&) noexcept = default;

    /** @brief Try to close a route by travelling from one customer to the depot.
     *
     * @return true when capacity/work-time constraints allow the closing leg.
     */
    bool CloseTravel(const Customer&, const Customer&);

    /** @brief Try to append an arc between two customers.
     *
     * @return true when the travel keeps the route feasible.
     */
    bool Travel(const Customer&, const Customer&);

    /** @brief Return the number of stored route steps. */
    [[nodiscard]] int size() const;

    /** @brief Return a mutable pointer to the underlying step sequence. */
    [[nodiscard]] RouteList* GetRoute();

    /** @brief Return a read-only pointer to the underlying step sequence. */
    [[nodiscard]] const RouteList* GetRoute() const;

    /** @brief Try to append one customer before route closure. */
    bool AddElem(const Customer&);

    /** @brief Try to append a sequence of customers in order. */
    bool AddElem(const std::list<Customer>&);

    /** @brief Remove the customer at the given route iterator and repair arc costs. */
    void RemoveCustomer(RouteList::iterator&);

    /** @brief Remove a specific customer from the route.
     *
     * @return true when the customer was found and removed.
     */
    bool RemoveCustomer(const Customer&);

    /** @brief Return the current sum of route arc costs. */
    [[nodiscard]] int GetTotalCost() const;

    /** @brief Return the vehicle capacity this route was created with. */
    [[nodiscard]] int GetInitialCapacity() const;

    /** @brief Return the graph travel cost between two customers. */
    [[nodiscard]] int GetTravelCost(const Customer&, const Customer&) const;

    /** @brief Compute a route-to-route distance used for route balancing heuristics. */
    [[nodiscard]] float GetDistanceFrom(const Route&) const;

    /** @brief Check whether this route contains a customer. */
    [[nodiscard]] bool FindCustomer(const Customer&) const;

    /** @brief Replace the route with a complete depot-to-depot customer sequence.
     *
     * @return true when the rebuilt route is feasible.
     */
    bool RebuildRoute(const std::list<Customer>&);

    /** @brief Return the penalized objective value for this route. */
    [[nodiscard]] float Evaluate() const;
};

using Routes = std::vector<Route>;

#endif /* Route_H */
