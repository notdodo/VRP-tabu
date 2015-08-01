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
    start = 0;
    std::cout << start << std::endl;
    while(j!=start) {
        *it++;
        j++;
    }
    j = 0;
    Customer last;
    Route r(this->capacity, this->workTime, this->graph);
    Map::const_iterator m = this->InsertStep(depot, it, it, r, s);
    this->routes.push_back(r);
    if (m == s.cbegin())
        std::cout << "QUi" << m->second << it->second << std::endl;
    while (j < this->vehicles && !s.empty()) {
        std::cout << "FINE ROUTE: " << m->second << " " << it->second << std::endl;
        Route v(this->capacity, this->workTime, this->graph);
        if (s.size() == 1) {
            last = m->second;
            v.Travel(depot, last);
            v.CloseTravel(last);
            s.clear();
        }else {

            m = this->InsertStep(depot, it, m, v, s);
        }
        this->routes.push_back(v);
        j++;
    }
    for(auto &e : this->routes) e.PrintRoute();
}

Map::const_iterator VRP::InsertStep(Customer depot, Map::iterator stop, Map::const_iterator i, Route &r, Map &distances) {
    Customer from, to;
    Map::const_iterator index = i;
    Map::const_iterator end = distances.cend();
    end--;
    Map::const_iterator last = end;
    end = distances.cend();
    Map::const_iterator fallback = index;
    to = index->second;
    if (!r.Travel(depot, to)) {
        std::string s;
        throw s = "One customer is unreachable!!!";
    }
    // depot, to
    if (index != last)
        index++;
    /*if (index == end && to == stop->second) {
        std::cout << to << from << stop->second <<std::endl;
        index = distances.begin();
    }*/
    while (index != stop && !distances.empty()) {
        from = to;
        fallback = index;
        to = index->second;
        index++;
        // il problema è qui: quando ho end() devo andare con to in v2,
        // ma v2 è lo stop quindi devo chiudere la route

        // ho from == to == v10 (end)
        if (index == distances.cend()) {
            --index;
            to = index->second;
            if (!r.Travel(from, to)) {
                r.CloseTravel(from);
                std::cout << "UNO " << from << to << fallback->second << std::endl;
                break;
            } else {
                index = distances.begin();
            }
        } else {
            if (!r.Travel(from, to)) {
                r.CloseTravel(from);
                if (stop != distances.cbegin())
                    stop--;
                if (stop->second == to) {
                    // cannot insert the last customer, need to a new route
                    distances.clear();
                    Map::iterator last = distances.insert({0, to});
                    stop = last;
                    std::cout << "NON INSERITO2 " << from << to << std::endl;
                    return stop;
                }
                break;
            }else {
                // se inserito e
                std::cout << to << from << stop->second <<std::endl;
                if (stop != distances.cbegin())
                    stop--;
                if (stop == fallback) {
                    std::cout << "FERMO2 " << to << from << stop->second <<std::endl;
                    if (!r.CloseTravel(to, depot)) {
                        std::cout << "ONONONONON" << std::endl;
                    }else {
                        distances.clear();
                    }
                }
                stop++;
                //std::cout << "INSERITO " << from << to;
            }
            //std::cout << " " << index->second << fallback->second << std::endl;
        }
    }
    return fallback;
}

/* destructor */
VRP::~VRP() {
}