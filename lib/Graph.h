#ifndef Graph_H
#define Graph_H

#include "Vertex.h"
#include <limits>
#include <memory>
#include <mutex>

using CostNeighborhood = std::vector<std::pair<int, Customer>>;

/** @brief Complete directed cost graph over customers.
 *
 * The graph stores customer vertices and weighted edges used by route
 * feasibility checks and all local-search cost evaluations. A compact cost
 * matrix mirrors the edge maps so hot-path route moves can read costs in O(1).
 */
class Graph {
  public:
    Graph() = default;

    /** @brief Insert a customer as a graph vertex. */
    void InsertVertex(Customer&);

    /** @brief Insert or update a weighted edge between two customers. */
    void InsertEdge(Customer&, Customer&, int);

    /** @brief Remove an edge between two customers. */
    void RemoveEdge(Customer&, Customer&);

    /** @brief Return depot-sorted customers by edge cost; duplicate costs are preserved. */
    std::multimap<int, Customer> sortV0();

    /** @brief Return neighbors of a customer sorted by travel cost. */
    std::multimap<int, Customer> GetNeighborhood(const Customer&) const;

    /** @brief Return neighbors as a cached sorted vector for hot-path scans. */
    const CostNeighborhood& GetNeighborhoodVector(const Customer&) const;

    /** @brief Build immutable lookup caches before parallel search reads them. */
    void PrepareNeighborhoods() const;

    /** @brief Return the destination customer and travel cost for an edge lookup. */
    std::pair<Customer, int> GetCosts(const Customer&, const Customer&) const;

    /** @brief Return the matrix travel cost for an edge lookup. */
    int GetCost(const Customer&, const Customer&) const;

  protected:
    /** @brief Insert a prebuilt vertex for a customer. */
    void InsertVertex(Customer&, Vertex&);

  private:
    static constexpr int MissingCost = std::numeric_limits<int>::max() / 4;

    /** @brief Resize the square cost matrix while preserving existing costs. */
    void ResizeCostMatrix(std::size_t, std::size_t);

    /** @brief Return a customer's compact matrix index. */
    std::size_t IndexOf(const Customer&) const;

    /** @brief Mark cached neighborhoods stale after graph mutation. */
    void InvalidateNeighborhoods();

    /** @brief Rebuild sorted neighborhoods from the compact cost matrix. */
    void RebuildNeighborhoods() const;

    std::map<Customer, Vertex> vertexes; /**< Map of vertexes: for each customer the vertex in input or output */
    std::map<Customer, std::size_t> vertexIndex;         /**< Stable compact index for each customer */
    std::vector<Customer> customers;                     /**< Customers in insertion order, depot first */
    std::vector<int> costMatrix;                         /**< Dense row-major travel-cost matrix */
    mutable std::vector<CostNeighborhood> neighborhoods; /**< Cached sorted neighborhoods */
    mutable bool neighborhoodsDirty = true;              /**< True when neighborhoods must be rebuilt */
    mutable std::shared_ptr<std::mutex> neighborhoodsMutex =
        std::make_shared<std::mutex>(); /**< Protects lazy cache rebuilds */
};

#endif /* Graph_H */
