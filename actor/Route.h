#ifndef Route_H
#define Route_H

#include <list>
#include "Graph.h"

typedef std::pair<Customer, int> StepType;

class Route {
private:
    /** @brief overriding of the "<<" operator for printing the route */
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
    int initialCapacity;            /**< Initial capacity of the route, equals to VRP.capacity */
    int initialWorkTime;            /**< Total work time for driver, equals to VRP.workTime */
    int capacity;                   /**< Capacity remaining */
    int workTime;                   /**< Work time remaining */
    int totalCost;                  /**< Total cost of the route: sum of the weight */
    // cost of traveling
    const float TRAVEL_COST = 0.3;  /**< Parameter for each travel */
    const double cWeight = 6;       /**< Weight of the capacity remain */
    const double tWeight = 8;       /**< Weight of the working time remain */
    Graph graph;                    /**< Graph of the customers */
protected:
    std::list<StepType> route;      /**< This list represent the route */
public:
    /** @brief overriding of the "=" operator for assign a route to another */
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
    double fitness;                 /**< Goodness of a route */
    // constructor
    Route(int, int, const Graph);       //!< constructor
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