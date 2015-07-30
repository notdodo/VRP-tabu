#include "Graph.h"

void Graph::InsertVertex(Customer cust) {
    Vertex::ConstructionToken c;
    Vertex v { c };
    InsertVertex(cust, v);
}

void Graph::InsertVertex(Customer c, Vertex v) {
    std::pair<Customer, Vertex> temp (c, v);
    vertexes.insert(temp);
}

void Graph::InsertEdge(Customer node, Customer new_edge, int weight) {
    auto it = vertexes.find(node);
    if (node.name != new_edge.name &&
       it != vertexes.end())
        it->second.InsertEdge(new_edge, weight);
}


void Graph::RemoveEdge(Customer node, Customer edge) {
    auto it = vertexes.find(node);
    if(it != vertexes.end())
        it->second.RemoveEdge(edge);
}

std::multimap<int, Customer> Graph::sortV0() {
    std::multimap<int, Customer> v;
    /* get depot */
    Customer c = vertexes.begin()->first;
    v.insert(std::pair<int, Customer>(0, c));
    Vertex it = vertexes.find(c)->second;
    for (auto& edge : it.GetEdges()) {
        v.insert(std::pair<int, Customer>(edge.second.weight, edge.first));
    }
    return v;
}

/* return the weight of edges &from &to */
std::pair<Customer, int> Graph::GetCosts(const Customer &from, const Customer &to) {
    /* get all edges from &from */
    Vertex it = vertexes.find(from)->second;
    return {from, it.GetEdges().find(to)->second.weight};
}

std::string Graph::ToPrint() {
    std::string out = "";
    std::vector<Customer> end_points;
    /* for each pair Customer-Vertex */
    for(auto&  pair : this->vertexes) {
        end_points = pair.second.copy_edges();
        out += pair.first.name + " : "; /* Customer name */
        for(auto& edge : end_points) {
            int n = pair.second.GetWeight(edge); /* edge weight */
            out += "\t -(" + std::to_string(n) + ")-> " + edge.name;
            out += "\r\n";
        }
        out += "\n";
    }
    return out;
}

/*Graph Graph::transpose() const {
    Graph Graph_T;
    //Vertex
    for(auto& pair : vertexes) {
        Graph_T.insert_vertex(pair.first);
    }
    //Edges
    std::vector<std::string> end_points;
    for(auto& pair : vertexes) {
        end_points = pair.second.copy_edges();
        for(auto &edge : end_points)
            Graph_T.insert_edge(edge, pair.first);
    }
    return Graph_T;
}

Graph Graph::merge(const Graph &G2)  const {
    Graph merge_graphs;

    //Merge vertexes
    for(auto& pair : vertexes)
        merge_graphs.insert_vertex(pair.first);

    for(auto& pair : G2.vertexes)
      merge_graphs.insert_vertex(pair.first);

    //Merge edges
    std::vector<std::string> end_points;
    for(auto& pair : vertexes) {
        end_points = pair.second.copy_edges();
        for(auto & edge : end_points) {
            merge_graphs.insert_edge( pair.first, edge);
        }
    }

    for(auto& pair : G2.vertexes) {
        end_points = pair.second.copy_edges();
        for(auto & edge : end_points) {
            merge_graphs.insert_edge( pair.first, edge);
        }
    }
    return merge_graphs;
}

Graph Graph::inverse() const {
    //Create a Graph temp which is complete
    Graph temp;

    for(auto& pair : vertexes) {
        temp.insert_vertex(pair.first);
    }

    for(auto& vertex1 : vertexes)
        for(auto vertex2 : vertexes)
         temp.insert_edge(vertex1.first, vertex2.first);

    //Remove all edges in temp that also are in (*this)
    std::vector<std::string> end_points;
    for(auto& pair : vertexes) {
        end_points = pair.second.copy_edges();
        for(auto edge : end_points) {
            temp.remove_edge(pair.first, edge);
        }
    }

   return temp;
} */