#ifndef Vertex_H
#define Vertex_H

#include <map>
#include <vector>
#include "Edge.h"

/** @brief Graph vertex containing outgoing weighted edges from one customer. */
class Vertex {
  public:
    /** @brief Token type that restricts Vertex construction to Graph. */
    class ConstructionToken {
      private:
        ConstructionToken() = default;
        friend class Graph;
    };

    /** @brief Create an empty vertex; callable only by Graph. */
    Vertex(ConstructionToken&);

    /** @brief Insert or update an outgoing edge to a customer. */
    void InsertEdge(Customer&, int);

    /** @brief Remove an outgoing edge. */
    void RemoveEdge(Customer&);

    /** @brief Return the weight for an outgoing edge. */
    [[nodiscard]] int GetWeight(Customer&);

    /** @brief Return all outgoing edges without copying the adjacency map. */
    [[nodiscard]] const std::map<Customer, Edge>& GetEdges() const;

  private:
    std::map<Customer, Edge> edges; /**< List of all the edges from customer */
};

#endif /* Vertex_H */
