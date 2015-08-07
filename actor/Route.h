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
    // overriding =
    Route& operator=(const Route &r) {
        this->capacity = r.capacity;
        this->workTime = r.workTime;
        this->totalCost = r.totalCost;
        this->fitness = r.fitness;
        this->route = r.route;
        return *this;
    }
    int initialCapacity;
    int capacity;
    int initialWorkTime;
    int workTime;
    int totalTime;
    int totalCost;
    // cost of traveling
    const float TRAVEL_COST = 0.3;
    const float cWeight = 6.0f;
    const float tWeight = 8.0f;
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
    void PrintRoute(std::list<StepType>);
    // insert the travel from customer A to B, if possibile
    bool Travel(const Customer, const Customer);
    // create an empty route
    void EmptyRoute(const Customer);
    void SetFitness();
    int size() const;
    std::list<StepType>* GetRoute();
    std::list<StepType> CopyRoute();
    bool AddElem(const Customer);
    void RemoveCustomer(std::list<StepType>::iterator &);
};

#endif /* Route_H */