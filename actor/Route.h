#ifndef Route_H
#define Route_H

#include "Customer.h"
#include "Graph.h"
#include <list>

typedef std::pair<Customer, int> StepType;

class Route {
private:
    /* overloading << */
    /*friend std::ostream& operator<<(std::ostream& out, const Route &r) {
        std::flush(std::cout);
        for (auto i : r.route)
            if (i.second > 0)
                out << i.first << " -(" << i.second << ")-> ";
            else
                out << i.first;
        return out;
    }*/
    int capacity;
    int workTime;
    int totalCapacity;
    int totalTime;
    int totalCost;
    const float TRAVEL_COST = 0.3;
    Graph graph;
protected:
    bool CheckViolations(StepType, Customer);
    bool check(Customer, Customer, Customer);
public:
    void PrintRoute();
    bool Travel(Customer, Customer);
    int ReplaceLastWithDepot(Customer, Customer);
    /* per ora list, alternativa: deque */
    std::list<StepType> route;
    Route(const int &, const int &, const Graph&);
};

#endif /* Route_H */