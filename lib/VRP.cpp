#include "VRP.h"

/** @brief Constructor of VRP.
 *
 * @param g The graph generate from the input
 * @param n The number of Customers
 * @param v The number of vehicles
 * @param c The capacity of each vehicle
 * @param t The work time of each driver
 */
VRP::VRP(const Graph g, const int n, const int v, const int c, const int t) {
    this->graph = g;
    this->numVertices = n;
    this->vehicles = v;
    this->capacity = c;
    this->workTime = t;
}

/** @brief This function creates the routes.
 *
 *  This function creates the initial solution of the algorithm.
 *  Customers are sorted in increasing order of distance from the depot.
 *  Starting with a random customer the route is created inserting one customer
 *  at time. Whenever the insertion of the customer would lead to a violation of
 *  the capacity or the working time a new route is created.
 *  @return Error or Warning code.
 */
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

/** @brief This function creates a route.
 *
 * Starting from a customer this function creates the route until
 * a constraint result invalid looping the list of sorted customers.
 * @param depot The depot
 * @param stop The last customer, in the list in the j-1, where j is the initial random customer
 * @param i The customer which starts the route
 * @param r The route to create
 * @param distances The sorted list of customers
 */
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
                // but if remain only one customer to serve, try to serve it
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

/** @brief Return the routes.
 *
 * @return The pointer to the routes' list
 */
std::list<Route>* VRP::GetRoutes() {
    return &this->routes;
}

/** @brief Sort the list of routes by fitness.
 *
 * This function sort the routes by fitness in descending order.
 */
void VRP::OrderFitness() {
    this->routes.sort([](Route const &lhs, Route const &rhs) {
        return lhs.fitness > rhs.fitness;
    });
}

/** @brief Remove all void routes. */
void VRP::CleanVoid() {
    this->routes.remove_if([](Route r){ return r.size() < 2; });
}

/** @brief Move a customer from a route to another.
 *
 * This opt function try to move, for every route, a customer
 * from a route to another and remove empty route.
 */
void VRP::Opt10() {
    std::list<Route>::iterator i = this->routes.begin();
    bool ret;
    for (; i != this->routes.cend(); i++) {
        auto from = i;
        std::advance(i, 1);
        if (i == this->routes.cend()) break;
        ret = Move1FromTo(*from, *i);
        // avoid to move the customer inserted
        if (ret) std::advance(i, 2);
        if (i == this->routes.cend()) break;
    }
    this->CleanVoid();
}

/** @brief Move a customer from a route to another.
 *
 * This function choose a random customer from the first route and
 * try to inserts it to the second route. Once the customer is inserted
 * is deleted from the first route.
 * @param r1 Route where to choose a random customer
 * @param r2 Route destination
 * @return True if the customer is moved.
 */
bool VRP::Move1FromTo(Route &r1, Route &r2) {
    bool ret = false;
    // no first, no last (no depot)
    int index = rand() % (r1.size() - 2) + 1;
    RouteList *from = r1.GetRoute();
    RouteList::iterator it = from->begin();
    std::advance(it, index);
    Customer f = it->first;
    if (r2.AddElem(f)) {
        // if the element is added remove the one on the first route
        r1.RemoveCustomer(it);
        ret = true;
    }
    return ret;
}

/** @brief Swap two customers from routes.
 *
 * This opt function try to swap, for every route, a random customer
 * from a route with another random customer from the next.
 */
void VRP::Opt11() {
    std::list<Route>::iterator i = this->routes.begin();
    bool ret;
    for (; i != this->routes.cend(); i++) {
        auto from = i;
        std::advance(i, 1);
        if (i == this->routes.cend()) break;
        ret = SwapFromTo(*from, *i);
        // if inserted move to another route
        if (ret)
            std::advance(i, 2);
    }
}

/** @brief Swap two random customer from two routes.
 *
 * This function choose two random customers from two routes and tries to swap them.
 * @param r1 First route
 * @param r2 Second route
 * @return True is the swap is successful
 */
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
    // copy r2 for fallback
    Route fallbackTo = r2;
    RouteList::iterator itTo = to->begin();
    std::advance(itTo, index2);
    Customer t = itTo->first;
    // with size<=3 deleting the single customer invalidate the route
    if (r1.size() >= 4 && r2.size() >= 4) {
        // remove the customer from route immediatly
        if (r2.AddElem(f, t)) {
            if (r1.AddElem(t, f)) {
                ret = true;
            }else
                r2 = fallbackTo;
        }
    }else {
        // add 'f'
        if (r2.AddElem(f)) {
            // add 't'
            if (r1.AddElem(t)) {
                // remove 'f' and 't'
                ret = true;
                r1.RemoveCustomer(f);
                r2.RemoveCustomer(t);
            }else
                r2.RemoveCustomer(f); // fallback to init
        }
    }
    return ret;
}

/** @brief This function combine Opt01 and Opt11.
 *
 * This function swap two customers from two routes and moves one
 * customer from the first to the second.
 */
void VRP::Opt12() {
    bool ret;
    std::list<Route>::iterator i = this->routes.begin();
    for (; i != this->routes.cend(); i++) {
        auto from = i;
        Route fallbackFrom = *from;
        std::advance(i, 1);
        auto to = i;
        if (i == this->routes.cend()) break;
        // if 'from' route has two or more customers
        if ((*from).size() > 4) {
            Route fallbackTo = *to;
            // swap two customer
            ret = SwapFromTo(*from, *to);
            // add one customer the the second route
            if (ret)
                ret = Move1FromTo(*from, *to);
            else {
                *from = fallbackFrom;
                *to = fallbackTo;
            }
            if (ret) std::advance(i, 2);
        }
    }
}

/** @brief This function combine Opt01 and Opt11.
 *
 * This function swap two customers from two routes and moves one
 * customer from the second to the first.
 */
void VRP::Opt21() {
    bool ret;
    std::list<Route>::iterator i = this->routes.begin();
    for (; i != this->routes.cend(); i++) {
        auto from = i;
        Route fallbackFrom = *from;
        std::advance(i, 1);
        auto to = i;
        if (i == this->routes.cend()) break;
        // if 'from' route has two or more customers
        if ((*from).size() > 4) {
            Route fallbackTo = *to;
            ret = SwapFromTo(*from, *to);
            if (ret)
                ret = Move1FromTo(*to, *from);
            else {
                *from = fallbackFrom;
                *to = fallbackTo;
            }
            if (ret) std::advance(i, 2);
        }
    }
}

/** @brief This function combine Opt01 and Opt11.
 *
 * This function swaps two customers from the first route
 * with two customers from the second.
 */
void VRP::Opt22() {
    std::list<Route>::iterator i = this->routes.begin();
    bool ret;
    for (; i != this->routes.cend(); i++) {
        auto from = i;
        Route fallbackFrom = *from;
        std::advance(i, 1);
        auto to = i;
        if (to == this->routes.cend()) break;
        // if 'from' route has two or more customers
        if ((*from).size() > 4) {
            Route fallbackTo = *to;
            ret = SwapFromTo(*from, *to);
            if (ret) ret = SwapFromTo(*from, *to);
            else {
                *from = fallbackFrom;
                *to = fallbackTo;
            }
            if (ret) std::advance(i, 2);
        }
    }
}

int VRP::GetTotalCost() const {
    int tCost = 0;
    for (auto e : this->routes) {
        tCost += e.GetTotalCost();
    }
    return tCost;
}

/* destructor */
VRP::~VRP() {
}