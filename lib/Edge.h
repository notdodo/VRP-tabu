#ifndef Edge_H
#define Edge_H

#include "Customer.h"

/** @brief Weighted graph edge used by Vertex adjacency maps. */
class Edge {
  public:
    /** @brief Token type that restricts Edge construction to Vertex. */
    class ConstructionToken {
      private:
        ConstructionToken() = default;
        friend class Vertex;
    };

    /** @brief Copy an edge. */
    Edge(const Edge&);
    Edge& operator=(const Edge&) = default;

    /** @brief Create an edge with a fixed weight; callable only by Vertex. */
    Edge(const ConstructionToken&, int);

    int weight; /**< Weight of the Edge */
  private:
};

#endif /* Edge_H */
