#include "VRP.h"

/* constructor */
VRP::VRP(const Graph g, const int n, const int v, const int c, const int t) {
    this->graph = g;
    this->numVertices = n;
    this->vehicles = v;
    this->capacity = c;
    this->workTime = t;
}

/* get random, add customers j,j+1,..., n,1,...,j-1 */
/* check violations of time, capacity */
int VRP::InitSolutions() {
    srand(time(NULL));
    int start;
    Customer depot, last;
    /* keyring of distances */
    std::vector<int> keys;
    // the iterator for the sorted customer
    Map::iterator it;
    /* distances of customers sorted from depot */
    Map dist = this->graph.sortV0();
    /* get the depot and remove it from the map */
    depot = dist.begin()->second;
    dist.erase(dist.begin());
    /* choosing a random customer, from 0 to numVertices-1 */
    start = rand() % (this->numVertices-1);
    std::cout << start << std::endl;
    it = dist.begin();
    // getting the index (customer) to start with
    std::advance(it, start);
    int j = 1;
    // preparing the route
    Route r(this->capacity, this->workTime, this->graph);
    // doing the first step, from to first customer
    Map::const_iterator m = this->InsertStep(depot, it, it, r, dist);
    // saving the route
    r.SetFitness();
    this->routes.push_back(r);
    // for all vehicles, or one the Map dist is empty
    while (j < this->vehicles && !dist.empty()) {
        Route v(this->capacity, this->workTime, this->graph);
        // if is remaing only one customer add it to route
        if (dist.size() == 1) {
            last = m->second;
            v.Travel(depot, last);
            v.CloseTravel(last);
            dist.clear();
        }else {
            m = this->InsertStep(depot, it, m, v, dist);
        }
        v.SetFitness();
        this->routes.push_back(v);
        // counting the vehicles
        j++;
    }
    if (j < this->vehicles)
        j = -1;
    else if (j == this->vehicles && dist.size() == 0)
        j = 0;
    else
        j = 1;
    return j;
}

Map::const_iterator VRP::InsertStep(Customer depot, Map::iterator stop, Map::const_iterator i, Route &r, Map &distances) {
    Customer from, to;
    Map::const_iterator index = i;
    Map::const_iterator end = distances.cend(); end--;
    Map::const_iterator last = end;
    Map::const_iterator fallback = index;
    // init the route with the travel from depot to first customer
    to = index->second;
    if (!r.Travel(depot, to)) {
        // if the customer is unreachable stop the program
        throw std::string("Customer" + to.name + " is unreachable!!!");
    }
    // increment the index, computing the next customer
    if (index != last)
        index++;
    else
        index = distances.begin();
    // the route need only the last customer (last before the stop)
    if (index == distances.cbegin() && index == stop) {
        r.CloseTravel(to);
        // clear the map to stop the loop
        distances.clear();
    }
    while (index != stop && !distances.empty()) {
        // set up from and to customers
        from = to;
        // a fallback index to recover the state
        fallback = index;
        to = index->second;
        // incrementing (if possible) the index for the next step of the route
        index++;
        if (index == distances.cend()) {
            --index;
            to = index->second;
            // if cannot add the customer, close the route and return
            if (!r.Travel(from, to)) {
                r.CloseTravel(from);
                return fallback;
            } else {
                // otherwise index move to the next customer (the first one)
                index = distances.begin();
            }
        } else {
            if (!r.Travel(from, to)) {
                r.CloseTravel(from);
                // the route is close but, if the last customer to serve is the last one return it
                if (stop != distances.cbegin())
                    std::advance(stop, -1);
                if (stop->second == to) {
                    // cannot insert the last customer, need to create a new route, only for it
                    distances.clear();
                    // a map with only this customer
                    Map::iterator last = distances.insert({0, to});
                    fallback = last;
                }
                break;
            }else {
                // the customer is inserted
                if (stop != distances.cbegin())
                    std::advance(stop, -1);
                // but if remain only one customer to serve, serve it
                if (stop == fallback) {
                    if (r.CloseTravel(to, depot)) {
                        distances.clear();
                    }
                } else {
                    std::advance(stop, 1);
                }
            }
        }
    }
    // return the index fallback to start a new route
    return fallback;
}

std::list<Route>* VRP::GetRoutes() {
    return &this->routes;
}

// sort routes by fitness (descending order)
void VRP::OrderFitness() {
    this->routes.sort([](Route const &lhs, Route const &rhs) {
        return lhs.fitness > rhs.fitness;
    });
}

// move a Customer from a route to another
void VRP::Opt10() {
    std::list<Route>::iterator i = this->routes.begin();
    bool ret;
    for (; i != this->routes.cend(); i++) {
        // depot -> customer -> depot, no Opt10
        if (i->size() > 3) {
            auto from = i;
            std::advance(i, 1);
            if (i == this->routes.cend()) break;
            // avoid to move the customer inserted
            ret = Move1FromTo(*from, *i);
            if (ret)
                std::advance(i, 1);
        }
    }
}

bool VRP::Move1FromTo(Route &r1, Route &r2) {
    bool ret = false;
    // no first, no last (no depot)
    int index = rand() % (r1.size() - 2) + 1;
    RouteList *from = r1.GetRoute();
    RouteList::iterator it = from->begin();
    std::advance(it, index);
    Customer f = it->first;
    if (r2.AddElem(f)) {
        r1.RemoveCustomer(it);
        ret = true;
    }
    return ret;
}

// swap two customers of two different routes
void VRP::Opt11() {
    std::list<Route>::iterator i = this->routes.begin();
    bool ret;
    for (; i != this->routes.cend(); i++) {
        auto from = i;
        std::advance(i, 1);
        if (i == this->routes.cend()) break;
        // avoid to move the customer inserted
        ret = SwapFromTo(*from, *i);
        if (ret)
            std::advance(i, 1);
    }
}

// pick a customer from r1 and r2 and swap them
bool VRP::SwapFromTo(Route &r1, Route &r2) {
    bool ret = false;
    int index1 = rand() % (r1.size() - 2) + 1;
    int index2 = rand() % (r2.size() - 2) + 1;
    // customer from r1
    RouteList *from = r1.GetRoute();
    RouteList::iterator itFrom = from->begin();
    std::advance(itFrom, index1);
    Customer f = itFrom->first;
    // customer from r2
    RouteList *to = r2.GetRoute();
    RouteList::iterator itTo = to->begin();
    std::advance(itTo, index2);
    Customer t = itTo->first;
    if (r2.AddElem(f, t)) {
        if (r1.AddElem(t, f))
            ret = true;
        else
            r2.RemoveCustomer(f);
    }
    return ret;
}

void VRP::Opt21() {
    std::list<Route>::iterator i = this->routes.begin();
    bool ret;
    for (; i != this->routes.cend(); i++) {
        auto from = i;
        std::advance(i, 1);
        if (i == this->routes.cend()) break;
        // if 'from' route has two or more customers
        if ((*from).size() > 4) {
            ret = SwapFromTo(*from, *i);
            if (ret) ret = Move1FromTo(*from, *i);
        }else {
            std::advance(i, 1);
        }
        if (ret)
            std::advance(i, 1);
    }
}

void VRP::Opt12() {
    std::list<Route>::iterator i = this->routes.begin();
    bool ret;
    for (; i != this->routes.cend(); i++) {
        auto from = i;
        std::advance(i, 1);
        if (i == this->routes.cend()) break;
        // if 'from' route has two or more customers
        if ((*from).size() > 4) {
            ret = SwapFromTo(*from, *i);
            if (ret) ret = Move1FromTo(*i, *from);
        }else {
            std::advance(i, 1);
        }
        if (ret)
            std::advance(i, 1);
    }
}

void VRP::Opt22() {
    std::list<Route>::iterator i = this->routes.begin();
    bool ret;
    for (; i != this->routes.cend(); i++) {
        auto from = i;
        std::advance(i, 1);
        if (i == this->routes.cend()) break;
        // if 'from' route has two or more customers
        if ((*from).size() > 4) {
            ret = SwapFromTo(*from, *i);
            if (ret) ret = SwapFromTo(*from, *i);
        }else {
            std::advance(i, 1);
        }
        if (ret)
            std::advance(i, 1);
    }
}

/* destructor */
VRP::~VRP() {
}