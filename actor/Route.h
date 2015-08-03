#ifndef Route_H
#define Route_H

#include <list>
#include "Graph.h"

typedef std::pair<Customer, int> StepType;

class Route {
private:
    /* overloading << */
    friend std::ostream& operator<<(std::ostream& out, const Route &r) {
        if (r.route.size() > 1) {
            std::flush(std::cout);
            for (auto i : r.route)
                if (i.second > 0)
                    out << i.first << " -(" << i.second << ")-> ";
                else
                    out << i.first;
        }
        return out;
    }
    int initialCapacity;
    int capacity;
    int initialWorkTime;
    int workTime;
    int totalCapacity;
    int totalTime;
    int totalCost;
    // cost of traveling
    const float TRAVEL_COST = 0.3;
    Graph graph;
protected:
    std::list<StepType> route;
public:
    float fitness;
    /* constructor */
    Route(int, int, const Graph);
    // add the travel from the customer to the depot
    void CloseTravel(const Customer);
    // add the travel from the first customer to the second, then to the depot
    bool CloseTravel(const Customer, const Customer);
    void PrintRoute();
    // insert the travel from customer A to B, if possibile
    bool Travel(const Customer, const Customer);
    // create an empty route
    void EmptyRoute(const Customer);
    void SetFitness();
    int size() const;
    std::list<StepType>* GetRoute();
    bool MoveCustomer(const Customer);
};

#endif /* Route_H */