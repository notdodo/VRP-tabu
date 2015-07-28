#ifndef Route_H
#define Route_H

#include "Customer.h"
#include <list>

class Route {
private:
    int totalCost;
public:
    /* per ora list, alternativa: deque */
    std::list<std::pair<Customer, int>> route;
    Route();
};

#endif /* Route_H */