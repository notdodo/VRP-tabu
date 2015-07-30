#ifndef Route_H
#define Route_H

#include "Customer.h"
#include "../lib/Graph.h"
#include <list>

typedef std::pair<Customer, int> StepType;

class Route {
private:
    /* overloading << */
    friend std::ostream& operator<<(std::ostream& out, const Route &r) {
        std::list<StepType>::const_iterator i = r.route.begin();
        out << std::endl;
        for (; i != r.route.end(); ++i) {
            if (i->second > 0)
                out << i->first << " -(" << i->second << ")-> ";
            else
                out << i->first;
        }
        return out;
    }
    int capacity;
    int workTime;
    int totalCapacity;
    int totalTime;
    int totalCost;
    Graph graph;
protected:
    bool CheckViolations(const StepType&, const  Customer&);
public:
    bool InsertStep(StepType, Customer);
    /* per ora list, alternativa: deque */
    std::list<StepType> route;
    Route(const int &, const int &, const Graph&);
};

#endif /* Route_H */