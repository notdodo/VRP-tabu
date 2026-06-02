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

#include "Utils.h"
#include <cstring>
#include <fstream>
#include <limits>
#include <set>
#include <utility>

namespace {
using Json = nlohmann::json;

constexpr const char* kInvalidFileFormat = "Invalid file format!";
constexpr const char* kUsage = "Usage: ./VRP [-v] data.json";

int JsonSizeToInt(std::size_t size) {
    if (size > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        throw std::runtime_error(kInvalidFileFormat);
    }
    return static_cast<int>(size);
}

void RequireNonNegative(int value) {
    if (value < 0) {
        throw std::runtime_error(kInvalidFileFormat);
    }
}

void RequirePositive(int value) {
    if (value <= 0) {
        throw std::runtime_error(kInvalidFileFormat);
    }
}
} // namespace

/** @brief Instantiate all parameters from command-line input and JSON.
 *
 * Parse the input file in JSON format and instantiates all variables
 * for the algorithm.
 * @param[in] argc Number of arguments passed through command line.
 * @param[in] argv Input file (json).
 * @param[in] costTravel Cost parameter for each travel.
 * @param[in] alphaParam Alpha parameter for route evaluation.
 * @return The pointer to VRP class
 */
VRP* Utils::InitParameters(int argc, char** argv, const float costTravel, const float alphaParam) {
    /* error string */
    std::string s;
    VRP* v = nullptr;
    Graph g;
    int fileIndex = 0;
    if (argc == 2) {
        fileIndex = 1;
    } else if (argc == 3 && strcmp(argv[1], "-v") == 0) {
        this->verbose = true;
        fileIndex = 2;
    } else {
        throw std::runtime_error(kUsage);
    }
    std::string file(argv[fileIndex]);
    std::size_t found = file.find_last_of("/\\");
    this->filename = file.substr(found + 1);
    std::ifstream input(argv[fileIndex]);
    if (!input) {
        if (argc == 3)
            throw std::runtime_error("The file " + std::string(argv[fileIndex]) + " doesn't exist.");
        else
            throw std::runtime_error("No input file.");
    }

    try {
        input >> this->d;
    } catch (const Json::parse_error& e) {
        throw std::runtime_error("Error(byte " + std::to_string(e.byte) + "): " + std::string(e.what()));
    }

    try {
        s = kInvalidFileFormat;
        const Json& vertices = this->d.at("vertices");
        if (!vertices.is_array() || vertices.empty()) {
            throw std::runtime_error(s);
        }
        int numVertices = JsonSizeToInt(vertices.size());
        if (numVertices < 2) {
            throw std::runtime_error(s);
        }
        std::vector<Customer> customers(static_cast<std::size_t>(numVertices));
        std::set<std::string> names;
        /* inserting the depot = v0 */
        const Json& depot = vertices.at(0);
        std::string depotName = depot.at("name").get<std::string>();
        if (depotName.empty() || !names.insert(depotName).second) {
            throw std::runtime_error(s);
        }
        customers[0] = Customer(std::move(depotName), depot.at("x").get<int>(), depot.at("y").get<int>());
        g.InsertVertex(customers[0]);
        bool flagTime = false;
        /* parsing all customers */
        for (int i = 1; i < numVertices; i++) {
            const Json& item = vertices.at(static_cast<std::size_t>(i));
            std::string n = item.at("name").get<std::string>();
            if (n.empty() || !names.insert(n).second) {
                throw std::runtime_error(s);
            }
            int x = item.at("x").get<int>();
            int y = item.at("y").get<int>();
            int request = item.at("request").get<int>();
            int serviceTime = item.at("time").get<int>();
            RequireNonNegative(request);
            RequireNonNegative(serviceTime);
            customers[static_cast<std::size_t>(i)] = Customer(std::move(n), x, y, request, serviceTime);
            // if no service time
            if (!flagTime && serviceTime > 0)
                flagTime = true;
            g.InsertVertex(customers[static_cast<std::size_t>(i)]);
        }

        /* parsing all costs, graph edge */
        const Json& costs = this->d.at("costs");
        if (!costs.is_array()) {
            throw std::runtime_error(s);
        }
        int numCosts = JsonSizeToInt(costs.size());
        if (numCosts != numVertices) {
            throw std::runtime_error(s);
        }
        std::vector<std::vector<bool>> seenCosts(static_cast<std::size_t>(numVertices),
                                                 std::vector<bool>(static_cast<std::size_t>(numVertices), false));
        for (int i = 0; i < numCosts; i++) {
            const Json& row = costs.at(static_cast<std::size_t>(i));
            if (!row.is_array() || JsonSizeToInt(row.size()) != numVertices - 1) {
                throw std::runtime_error(s);
            }
            for (int j = 0; j < numCosts - 1; j++) {
                const Json& edge = row.at(static_cast<std::size_t>(j));
                /* start, end, cost */
                int from = edge.at("from").get<int>();
                int to = edge.at("to").get<int>();
                int value = edge.at("value").get<int>();
                if (from < 0 || to < 0 || from >= numVertices || to >= numVertices || from == to) {
                    throw std::runtime_error(s);
                }
                RequireNonNegative(value);
                if (seenCosts[static_cast<std::size_t>(from)][static_cast<std::size_t>(to)]) {
                    throw std::runtime_error(s);
                }
                seenCosts[static_cast<std::size_t>(from)][static_cast<std::size_t>(to)] = true;
                /* add edge with weight */
                g.InsertEdge(customers[static_cast<std::size_t>(from)], customers[static_cast<std::size_t>(to)], value);
            }
        }
        for (int from = 0; from < numVertices; ++from) {
            for (int to = 0; to < numVertices; ++to) {
                if (from != to && !seenCosts[static_cast<std::size_t>(from)][static_cast<std::size_t>(to)]) {
                    throw std::runtime_error(s);
                }
            }
        }

        /* creating the VRP */
        const int vehicles = this->d.at("vehicles").get<int>();
        const int capacity = this->d.at("capacity").get<int>();
        const int workTime = this->d.at("worktime").get<int>();
        RequirePositive(vehicles);
        RequirePositive(capacity);
        RequireNonNegative(workTime);
        int totalDemand = 0;
        for (int i = 1; i < numVertices; ++i) {
            totalDemand += customers[static_cast<std::size_t>(i)].request;
        }
        const int minimumRoutes = (totalDemand + capacity - 1) / capacity;
        v = new VRP(std::move(g), numVertices, vehicles, capacity, minimumRoutes, static_cast<float>(workTime),
                    flagTime, costTravel, alphaParam);
    } catch (const Json::exception& e) {
        throw std::runtime_error(s + " " + std::string(e.what()));
    }
    return v;
}

/** @brief Save the result.
 *
 * Saves the routes into the input JSON file.
 * @param[in] routes The routes list to save to the file
 * @param[in] t      Execution partial time
 */
void Utils::SaveResult(const Routes& routes, long long t) {
    std::string path = "vrp-init/" + this->filename;
    std::ofstream output(path);
    if (output) {
        this->d.erase("routes");
        this->d.erase("costs");
        this->d.erase("time");
        Json route = Json::array();
        Json totalCosts = Json::array();
        for (const Route& routeElem : routes) {
            Json r = Json::array();
            totalCosts.push_back(routeElem.GetTotalCost());
            for (const auto& e : *routeElem.GetRoute()) {
                r.push_back(e.first.name);
            }
            route.push_back(std::move(r));
        }
        this->d["routes"] = std::move(route);
        this->d["costs"] = std::move(totalCosts);
        this->d["time"] = t;
        output << this->d.dump(4) << '\n';
        if (!output) {
            throw std::runtime_error("Error writing file! (Bad permissions)");
        }
        this->logger("Saving the routes inside vrp-init/" + this->filename, this->VERBOSE);
    } else {
        throw std::runtime_error("Error writing file! (Bad permissions)");
    }
}
