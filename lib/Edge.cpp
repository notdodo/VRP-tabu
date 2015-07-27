#include "Edge.h"

/* constructor */
Edge::ConstructionToken::ConstructionToken() = default;

Edge::Edge(const Edge &) = default;

Edge::Edge(const ConstructionToken &, const int &w) {
    this->weight = w;
}