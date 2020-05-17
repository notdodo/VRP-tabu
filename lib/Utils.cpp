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

/** @brief ###Instantiate all parameters.
 *
 * Parse the input file in JSON format and instantiates all variables
 * for the algorithm.
 * @param[in] argc Number of arguments passed through command line.
 * @param[in] argv Input file (json).
 * @param[in] costTravel Cost parameter for each travel.
 * @param[in] alphaParam Alpha parameter for router evaluation.
 * @return The pointer to VRP class
 */
VRP* Utils::InitParameters(int argc, char **argv, const float costTravel, const float alphaParam) {
    /* error string */
    std::string s;
    VRP *v;
    Graph g;
    int fileIndex = 1;
    if (argc != 2 && argc != 3) {
        throw std::runtime_error("Usage: ./VRP [-v] data.json");
    }

    if (strcmp(argv[1], "-v") == 0) {
        this->verbose = true;
        fileIndex = 2;
    }
    /* open the file */
    std::string file(argv[fileIndex]);
    std::size_t found = file.find_last_of("/\\");
    this->filename = file.substr(found+1);
    FILE *fp = fopen(argv[fileIndex], "r");
    if (fp == NULL) {
        if (argc == 3)
            throw std::runtime_error("The file " + std::string(argv[fileIndex]) +  " doesn't exist.");
        else
            throw std::runtime_error("No input file.");
    }else {
        /* little buffer more fread, big buffer less fread() big access time */
        char readBuffer[65536];
        rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));
        this->d.ParseStream(is);
        fclose(fp);
        /* check error */
        if (this->d.HasParseError()) {
            throw std::runtime_error("Error(offset" + std::string((char*)this->d.GetErrorOffset()) +
                "): " + GetParseError_En(this->d.GetParseError()));
        }else {
            s = "Invalid file format!";
            int numVertices = this->d["vertices"].Size();
			std::vector<Customer> customers(numVertices);
            /* inserting the depot = v0 */
            customers[0] = Customer(this->d["vertices"][0]["name"].GetString(),
                                    this->d["vertices"][0]["x"].GetInt(),
                                    this->d["vertices"][0]["y"].GetInt());
            g.InsertVertex(customers[0]);
            int x, y, request, serviceTime;
            bool flagTime = false;
            /* parsing all customers */
            for (int i = 1; i < numVertices; i++) {
                auto& item = d["vertices"][i];
                if (item["name"].IsString() && item["x"].IsInt() &&
                    item["y"].IsInt() && item["request"].IsInt() &&
                    item["time"].IsInt()) {
                        /* name, x, y, request, service time */
                        std::string n = this->d["vertices"][i]["name"].GetString();
                        x = this->d["vertices"][i]["x"].GetInt();
                        y = this->d["vertices"][i]["y"].GetInt();
                        request = this->d["vertices"][i]["request"].GetInt();
                        serviceTime = this->d["vertices"][i]["time"].GetInt();
                        customers[i] = Customer(n, x, y, request, serviceTime);
                        // if no service time
                        if (!flagTime && serviceTime > 0)
                            flagTime = true;
                        g.InsertVertex(customers[i]);
                }else {
                    throw std::runtime_error(s);
                }
            }
            /* parsing all costs, graph edge */
            int numCosts = this->d["costs"].Size();
            int from, to, value;
            for (int i = 0; i < numCosts; i++) {
                for (int j = 0; j < numCosts-1; j++) {
                    /* start, end, cost */
                    from = this->d["costs"][i][j]["from"].GetInt();
                    to = this->d["costs"][i][j]["to"].GetInt();
                    value = this->d["costs"][i][j]["value"].GetInt();
                    /* add egde with weight */
                    g.InsertEdge(customers[from], customers[to], value);
                }
            }
            if (this->d["vehicles"].IsInt() &&
                this->d["capacity"].IsInt() && this->d["worktime"].IsInt()) {
                /* creating the VRP */
                v = new VRP(g,
                      numVertices,
                      this->d["vehicles"].GetInt(),
                      this->d["capacity"].GetInt(),
                      this->d["worktime"].GetInt(),
                      flagTime,
                      costTravel, alphaParam);
            }else {
                throw std::runtime_error(s);
            }
        }
    }
    return v;
}

/** @brief ###Save the result.
 *
 * Saves the routes into the input JSON file.
 * @param[in] routes The routes list to save to the file
 * @param[in] t      Execution partial time
 */
void Utils::SaveResult(const std::list<Route> routes, int t) {
    std::string path = "vrp-init/" + this->filename;
    FILE *fp = fopen(path.c_str(), "w");
    if (fp != NULL) {
        char writeBuffer[65536];
        rapidjson::FileWriteStream os(fp, writeBuffer, sizeof(writeBuffer));
        rapidjson::PrettyWriter<rapidjson::FileWriteStream> writer(os);
        rapidjson::Document::AllocatorType &allocator = this->d.GetAllocator();
        rapidjson::Value route(rapidjson::kArrayType);
        rapidjson::Value totalCosts(rapidjson::kArrayType);
        rapidjson::Value timeExec(t);
        // for each route
        for (Route routeElem : routes) {
            rapidjson::Value r(rapidjson::kArrayType);
            totalCosts.PushBack(routeElem.GetTotalCost(), allocator);
            // for each customer
            for (auto &e : *routeElem.GetRoute()) {
                rapidjson::Value customer;
                customer.SetString(e.first.name.c_str(), e.first.name.length(), allocator);
                r.PushBack(customer, allocator);
            }
            route.PushBack(r, allocator);
        }
        d.AddMember("routes", route, allocator);
        d.AddMember("costs", totalCosts, allocator);
        d.AddMember("time", timeExec, allocator);
        d.Accept(writer);
        fclose(fp);
        this->logger("Saving the routes inside vrp-init/" + this->filename, this->VERBOSE);
    }else {
        throw std::runtime_error("Error writing file! (Bad permissions)");
    }
}
