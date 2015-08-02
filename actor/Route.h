#ifndef Route_H
#define Route_H

#include <list>
#include "Graph.h"

typedef std::pair<Customer, int> StepType;

class Route {
private:
    /* overloading << */
    friend std::ostream& operator<<(std::ostream& out, const Route &r) {
        std::flush(std::cout);
        for (auto i : r.route)
            if (i.second > 0)
                out << i.first << " -(" << i.second << ")-> ";
            else
                out << i.first;
        return out;
    }
    int capacity;
    int workTime;
    int totalCapacity;
    int totalTime;
    int totalCost;
    const float TRAVEL_COST = 0.3;
    Graph graph;
protected:

public:
    void CloseTravel(Customer);
    bool CloseTravel(Customer, Customer);
    bool CheckConstraints(Customer, Customer);
    void PrintRoute();
    bool Travel(Customer, Customer);
    void EmptyRoute(Customer);
    /* per ora list, alternativa: deque */
    std::list<StepType> route;
    Route(int, int, const Graph);
};

#endif /* Route_H */