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
    it = dist.begin();
    std::advance(it, start);
    // getting the index (customer) to start with
    /*while(j != start) {
        *it++;
        j++;
    }*/
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
        r.SetFitness();
        this->routes.push_back(v);
        // counting the vehicles
        j++;
    }
    /*for (int i = j; i < this->vehicles; i++) {
        Route empty(this->capacity, this->workTime, this->graph);
        empty.EmptyRoute(depot);
        this->routes.push_back(empty);
    }*/
    if (j < this->vehicles)
        j = -1;
    else if (j == this->vehicles && dist.size() == 0)
        j = 0;
    else
        j = 1;
    for (auto &e: this->routes) e.PrintRoute();
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
            } else {
                // otherwise index move to the next customer (the first one)
                index = distances.begin();
            }
        } else {
            if (!r.Travel(from, to)) {
                r.CloseTravel(from);
                // the route is close but, if the last customer to serve is the last one return it
                if (stop != distances.cbegin())
                    stop--;
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
                    stop--;
                // but if remain only one customer to serve, serve it
                if (stop == fallback) {
                    if (r.CloseTravel(to, depot)) {
                        distances.clear();
                    }
                } else {
                    stop++;
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
std::list<Route> VRP::OrderFitness() {
    std::list<Route> ordered = this->routes;
    ordered.sort([](Route const &lhs, Route const &rhs) {
        return lhs.fitness > rhs.fitness;
    });
    return ordered;
}

// move a Customer from a route to another
void VRP::Opt10() {
    typedef std::list<std::pair<Customer, int>> RouteElem;
    std::list<Route> ordered = this->OrderFitness();
    for (Route &e : ordered) {
        // depot -> customer -> depot, no Opt10
        if (e.size() > 3) {
            // choose an element of the route, no first, no last
            int index =  rand() % (e.size() - 2) + 1;
            RouteElem *route = e.GetRoute();
            RouteElem::iterator it = route->begin();
            // it is the customer to move
            std::advance(it, index);
        }
    }
}

/* destructor */
VRP::~VRP() {
}