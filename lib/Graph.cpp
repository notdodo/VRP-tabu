/*****************************************************************************
    This file is part of VRP.

    VRP is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    VRP is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with VRP.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/

#include "Graph.h"
#include <algorithm>
#include <stdexcept>

namespace {
/** @brief Deterministic ordering for equal-cost neighbor entries. */
bool CustomerCostLess(const std::pair<int, Customer>& left, const std::pair<int, Customer>& right) {
    if (left.first != right.first) {
        return left.first < right.first;
    }
    return left.second.name < right.second.name;
}
} // namespace

/** @brief Insert a vertex.
 *
 * Create and insert a vertex in the graph.
 * @param[in] cust The customer who form the vertex
 */
void Graph::InsertVertex(Customer& cust) {
    Vertex::ConstructionToken c;
    Vertex v{c};
    InsertVertex(cust, v);
}

/** @brief Insert a prebuilt vertex.
 *
 * Insert a vertex in the graph.
 * @param[in] c The customer who form the vertex
 * @param[in] v The vertex created
 */
void Graph::InsertVertex(Customer& c, Vertex& v) {
    if (this->vertexes.contains(c)) {
        return;
    }
    const std::size_t oldSize = this->customers.size();
    this->customers.push_back(c);
    this->vertexIndex.emplace(c, oldSize);
    this->ResizeCostMatrix(oldSize, oldSize + 1);
    this->costMatrix[oldSize * (oldSize + 1) + oldSize] = 0;
    this->vertexes.emplace(c, v);
    this->InvalidateNeighborhoods();
}

/** @brief Insert an edge.
 *
 * Insert an edge with weight from a customer to another.
 * @param[in] node The starting customer
 * @param[in] new_edge The destination customer
 * @param[in] weight The weight of the edge
 */
void Graph::InsertEdge(Customer& node, Customer& new_edge, int weight) {
    auto it = vertexes.find(node);
    if (node.name != new_edge.name && it != vertexes.end()) {
        it->second.InsertEdge(new_edge, weight);
        const std::size_t fromIndex = this->IndexOf(node);
        const std::size_t toIndex = this->IndexOf(new_edge);
        this->costMatrix[fromIndex * this->customers.size() + toIndex] = weight;
        this->InvalidateNeighborhoods();
    }
}

/** @brief Remove an edge.
 *
 * @param[in] node The customer which the edge start
 * @param[in] edge The customer which the edge finish
 */
void Graph::RemoveEdge(Customer& node, Customer& edge) {
    auto it = vertexes.find(node);
    if (it != vertexes.end()) {
        it->second.RemoveEdge(edge);
        const std::size_t fromIndex = this->IndexOf(node);
        const std::size_t toIndex = this->IndexOf(edge);
        this->costMatrix[fromIndex * this->customers.size() + toIndex] = MissingCost;
        this->InvalidateNeighborhoods();
    }
}

/** @brief Sort the customers by distance from the depot.
 *
 * This function sorts the customer by distance from the depot;
 * the order is crescent.
 * @return The map of customer sorted
 */
std::multimap<int, Customer> Graph::sortV0() {
    std::multimap<int, Customer> v;
    if (this->customers.empty()) {
        return v;
    }
    // The input parser inserts the depot first; keep that stable instead of relying on map order.
    Customer c = this->customers.front();
    // insert the depot
    v.emplace(std::pair<int, Customer>(0, c));
    const std::size_t depotIndex = this->IndexOf(c);
    for (std::size_t index = 0; index < this->customers.size(); ++index) {
        if (index == depotIndex) {
            continue;
        }
        const int cost = this->costMatrix[depotIndex * this->customers.size() + index];
        if (cost != MissingCost) {
            v.emplace(std::pair<int, Customer>(cost, this->customers[index]));
        }
    }
    return v;
}

/** @brief Find the neighborhood of a customer.
 *
 * This function sorts the neighborhood of a customer by distance;
 * the order is crescent.
 * @return The map of customer sorted
 */
std::multimap<int, Customer> Graph::GetNeighborhood(const Customer& c) const {
    std::multimap<int, Customer> mm;
    for (const auto& [cost, customer] : this->GetNeighborhoodVector(c)) {
        mm.emplace(std::pair<int, Customer>(cost, customer));
    }
    return mm;
}

/** @brief Return neighbors of a customer as a sorted cached vector. */
const CostNeighborhood& Graph::GetNeighborhoodVector(const Customer& c) const {
    this->RebuildNeighborhoods();
    return this->neighborhoods[this->IndexOf(c)];
}

/** @brief Build the sorted-neighborhood cache before parallel readers use it. */
void Graph::PrepareNeighborhoods() const { this->RebuildNeighborhoods(); }

/** @brief Return the weight of an edge.
 *
 * This function compute the cost of travelling from a customer to another.
 * @param[in] from The starting customer
 * @param[in] to The ending customer.
 * @return The cost of the travel
 */
std::pair<Customer, int> Graph::GetCosts(const Customer& from, const Customer& to) const {
    return {to, this->GetCost(from, to)};
}

/** @brief Return the O(1) matrix travel cost between two customers. */
int Graph::GetCost(const Customer& from, const Customer& to) const {
    const std::size_t size = this->customers.size();
    const int cost = this->costMatrix[this->IndexOf(from) * size + this->IndexOf(to)];
    if (cost == MissingCost) {
        throw std::runtime_error("Missing travel cost from " + from.name + " to " + to.name);
    }
    return cost;
}

/** @brief Resize the row-major matrix without losing existing costs. */
void Graph::ResizeCostMatrix(std::size_t oldSize, std::size_t newSize) {
    std::vector<int> resized(newSize * newSize, MissingCost);
    for (std::size_t row = 0; row < oldSize; ++row) {
        for (std::size_t col = 0; col < oldSize; ++col) {
            resized[row * newSize + col] = this->costMatrix[row * oldSize + col];
        }
    }
    this->costMatrix = std::move(resized);
}

/** @brief Return a customer's compact matrix index. */
std::size_t Graph::IndexOf(const Customer& customer) const { return this->vertexIndex.at(customer); }

/** @brief Mark cached neighborhoods stale after graph mutation. */
void Graph::InvalidateNeighborhoods() {
    this->neighborhoodsDirty = true;
    this->neighborhoods.clear();
}

/** @brief Rebuild all sorted neighborhoods from the compact matrix. */
void Graph::RebuildNeighborhoods() const {
    if (!this->neighborhoodsDirty) {
        return;
    }
    std::scoped_lock lock(*this->neighborhoodsMutex);
    // cppcheck-suppress identicalConditionAfterEarlyExit
    if (!this->neighborhoodsDirty) {
        return;
    }
    this->neighborhoods.assign(this->customers.size(), {});
    if (this->customers.empty()) {
        this->neighborhoodsDirty = false;
        return;
    }
    const Customer& depot = this->customers.front();
    const std::size_t size = this->customers.size();
    for (std::size_t from = 0; from < size; ++from) {
        CostNeighborhood& neighborhood = this->neighborhoods[from];
        neighborhood.reserve(size > 0 ? size - 1 : 0);
        for (std::size_t to = 0; to < size; ++to) {
            const int cost = this->costMatrix[from * size + to];
            if (from != to && cost != MissingCost && this->customers[to] != depot) {
                neighborhood.emplace_back(cost, this->customers[to]);
            }
        }
        std::ranges::sort(neighborhood, CustomerCostLess);
    }
    this->neighborhoodsDirty = false;
}
