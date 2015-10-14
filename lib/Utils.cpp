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

/** @brief Instantiate all parameters.
 *
 * Parse the input file in JSON format and instantiates all variables
 * for the algorithm.
 * @param argv Input file (json)
 * @return The pointer to VRP class
 */
VRP* Utils::InitParameters(char **argv) {
    /* error string */
    std::string s;
    VRP *v;
    Graph g;
    int fileIndex = 1;
    /* open the file */
    if (strcmp(argv[1], "-v") == 0) {
        this->verbose = true;
        fileIndex = 2;
    }
    FILE* fp = fopen(argv[fileIndex], "r");
    if (fp == NULL) {
        s = "The file " + std::string(argv[fileIndex]) +  " doesn't exist.";
        throw s;
    }else {
        /* little buffer more fread, big buffer less fread() big access time */
        char readBuffer[65536];
        rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));
        this->d.ParseStream(is);
        fclose(fp);
        /* check error */
        if (this->d.HasParseError()) {
            s = "Error(offset" + std::string((char*)this->d.GetErrorOffset()) +
                "): " + GetParseError_En(this->d.GetParseError());
            throw s;
        }else {
            s = "Invalid file format!";
            int numVertices = this->d["vertices"].Size();
            Customer *customers = new Customer[numVertices];
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
                    throw s;
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
            delete [] customers;
            if (this->d["vehicles"].IsInt() &&
                this->d["capacity"].IsInt() && this->d["worktime"].IsInt()) {
                /* creating the VRP */
                v = new VRP(g,
                      numVertices,
                      this->d["vehicles"].GetInt(),
                      this->d["capacity"].GetInt(),
                      this->d["worktime"].GetInt(), flagTime);
            }else {
                throw s;
            }
        }
    }
    return v;
}

/** @brief Save the result.
 *
 * Saves the routes into the input JSON file.
 * @param routes The routes list to save to the file
 */
void Utils::SaveResult(std::list<Route> routes) {
    FILE* fp = fopen("output.json", "w");
    if (fp != NULL) {
        char writeBuffer[65536];
        rapidjson::FileWriteStream os(fp, writeBuffer, sizeof(writeBuffer));
        rapidjson::PrettyWriter<rapidjson::FileWriteStream> writer(os);
        rapidjson::Document::AllocatorType &allocator = this->d.GetAllocator();
        rapidjson::Value route(rapidjson::kArrayType);
        rapidjson::Value totalCosts(rapidjson::kArrayType);
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
        d.Accept(writer);
        fclose(fp);
    }else {
        throw "Error writing file! (Bad permissions)\n";
    }
}