#include "Edge.h"

Edge::ConstructionToken::ConstructionToken() = default;

Edge::Edge(const Edge &) = default;

/** @brief Constructor of Edge.
 *
 * @param w Weight of the arch
 */
Edge::Edge(const ConstructionToken &, const int &w) {
    this->weight = w;
}