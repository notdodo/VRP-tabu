#include "VRP.h"

/* constructor */
VRP::VRP(const Graph &g, int n, int v, int c, int t) {
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
    Route r(this->capacity, this->workTime, this->graph);
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
    start = 9;
    it = s.begin();
    int j = 0;
    while(it != s.end() && j!=start) {
        it++;
        j++;
    }
    /* from this customer create routes */
    this->InsertStep(depot, it, r, s);
    std::cout << r << std::endl;
}

bool VRP::InsertStep(Customer depot, Map::iterator &it, Route &r, Map &distances) {
    Customer from, to;
    bool flag = true;
    Map::iterator i = it;
    /* from depot to random customer j */
    flag = r.InsertStep(this->graph.GetCosts(depot, i->second), i->second);
    /* from j to n */
    while(i != distances.end() && flag) {
        from = i->second;
        i++;
        if (i == distances.end()) {
            flag = r.InsertStep(this->graph.GetCosts(from, distances.begin()->second), distances.begin()->second);
            to = from;
        }else {
            to = i->second;
            flag = r.InsertStep(this->graph.GetCosts(from, to), to);
        }
    }
    /* from 0 to j-1 */
    if (it != distances.begin() && flag) {
        i = distances.begin();
        it--;
        while(i != it && flag) {
            if (to == from) {
                to = i->second;
            }else {
                from = i->second;
                i++;
                if (i != distances.end()) {
                    to = i->second;
                    flag = r.InsertStep(this->graph.GetCosts(from, to), to);
                }
            }
        }
        to = i->second;
        if (it == i) {
            flag = r.InsertStep(this->graph.GetCosts(to, from), to);
        }
    }
    flag = r.InsertStep(this->graph.GetCosts(to, depot), depot);
    return flag;
}

/* destructor */
VRP::~VRP() {
}