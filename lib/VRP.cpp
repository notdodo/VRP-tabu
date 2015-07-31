#include "VRP.h"

/* constructor */
VRP::VRP(Graph g, int n, int v, int c, int t) {
    this->graph = g;
    this->numVertices = n;
    this->vehicles = v;
    this->capacity = c;
    this->workTime = t;
    this->InitSolutions();
}

/* get random, add customers j,j+1,..., n,1,...,j-1 */
/* check violations of time, capacity */
/* minimize the increase total time travel */
void VRP::InitSolutions() {
    //Route r(this->capacity, this->workTime, this->graph);
    srand(time(NULL));
    int start;
    /* distances of customers from depot */
    Map s = this->graph.sortV0();
    /* get the depot and remove it from the map */
    Customer depot = s.begin()->second;
    s.erase(s.begin());
    /* keyring of distances multimap s */
    std::vector<int> keys;
    Map::iterator it;
    /* choosing a random customer, from 0 to numVertices-1 */
    start = rand() % (this->numVertices-1);
    it = s.begin();
    int j = 0;
    start = 3;
    std::cout << start << std::endl;
    while(j!=start) {
        *it++;
        j++;
    }
    /* for each vehicles create routes */
    j = 0;
    /* from it create the routes */
    Route r(this->capacity, this->workTime, this->graph);
    Map::iterator i = this->InsertStep(depot, it, it, r, s);
    this->routes.push_back(r);
    while(j < this->vehicles && i != it) {
        Route r(this->capacity, this->workTime, this->graph);
        i = this->InsertStep(depot, it, i, r, s);
        this->routes.push_back(r);
        j++;
    }
    for (auto &e : this->routes) e.PrintRoute();
}

Map::iterator VRP::InsertStep(Customer depot, Map::iterator stop, Map::iterator i, Route &r, Map &distances) {
    Customer from, to;
    bool flag = true;
    Map::iterator fallback = i;
    /* from depot to random customer j */
    to = i->second;
    flag = r.Travel(depot, to);
    i++;
    if (i == stop) {
        flag = r.Travel(to, depot);
        if (flag)
            return stop;
    }
    while (flag) {
        from = to;
        if (i == distances.cend())
            i = distances.begin();
        to = i->second;
        fallback = i;
        i++;
        flag = r.Travel(from, to);
        if (flag && to == stop->second) {
            flag = r.Travel(from, depot);
            if (!flag) {
                int diff = r.ReplaceLastWithDepot(from, depot);
                while(diff > 0) {
                    diff--;
                    fallback--;
                }
            }
        }
    }
    return fallback;
}

/* destructor */
VRP::~VRP() {
}