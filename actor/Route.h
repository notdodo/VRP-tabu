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
            out << "Fit: " << std::to_string(r.fitness) << " ";
            for (auto i : r.route)
                if (i.second > 0)
                    out << i.first << " -(" << std::to_string(i.second) << ")-> ";
                else
                    out << i.first;
        }
        return out;
    }
    int initialCapacity;
    int initialWorkTime;
    // capacity adn workTime remain
    int capacity;
    int workTime;
    int totalCost;
    // cost of traveling
    const float TRAVEL_COST = 0.3;
    const double cWeight = 6;
    const double tWeight = 8;
    Graph graph;
protected:
    std::list<StepType> route;
public:
    // overriding =
    Route& operator=(const Route &r) {
        this->capacity = r.capacity;
        this->workTime = r.workTime;
        this->totalCost = r.totalCost;
        this->fitness = r.fitness;
        this->route = r.route;
        this->initialCapacity = r.initialCapacity;
        this->initialWorkTime = r.initialWorkTime;
        this->graph = r.graph;
        return *this;
    }
    double fitness;
    // constructor
    Route(int, int, const Graph);
    void CloseTravel(const Customer);
    bool CloseTravel(const Customer, const Customer);
    void PrintRoute();
    void PrintRoute(std::list<StepType>);
    bool Travel(const Customer, const Customer);
    void EmptyRoute(const Customer);
    void SetFitness();
    int size() const;
    std::list<StepType>* GetRoute();
    std::list<StepType> CopyRoute();
    bool AddElem(const Customer);
    bool AddElem(const Customer, const Customer);
    void RemoveCustomer(std::list<StepType>::iterator &);
    void RemoveCustomer(const Customer);
};

#endif /* Route_H */