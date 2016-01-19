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

#include "Vertex.h"

/** @brief ###Constructor of Vertex */
Vertex::Vertex(ConstructionToken &) { }

/** @brief ###Insert an Edge.
 *
 * Insert a weighted edge which end to a customer.
 * @param[in] end_point The destination customer
 * @param[in] weight The weight of the edge
 */
void Vertex::InsertEdge(Customer &end_point, int weight) {
    Edge new_edge { Edge::ConstructionToken{}, weight };
	std::pair<Customer, Edge> temp(end_point, new_edge);
    edges.insert(temp);
}

/** @brief ###Remove an edge.
 *
 * @param[in] edge The customer which the edge end
 */
void Vertex::RemoveEdge(Customer &edge) {
    edges.erase(edge);
}

/** @brief ###Get the weight of and edge.
 *
 * @param[in] c Customer at the end of the edge
 * @return The weight of the customer
 */
int Vertex::GetWeight(Customer &c) {
    return edges.find(c)->second.weight;
}

/** @brief ###Get the map of the edges */
std::map<Customer, Edge> Vertex::GetEdges() const {
    return this->edges;
}
