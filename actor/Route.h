#ifndef Route_H
#define Route_H

#include <list>
#include <iomanip>
#include "Graph.h"

typedef std::pair<Customer, int> StepType;

class Route {
private:
    /** @brief overriding of the "<<" operator for printing the route */
    friend std::ostream& operator<<(std::ostream& out, const Route &r) {
        if (r.route.size() > 1) {
            out << "Cost:" << std::setw(5) << std::to_string(r.totalCost) << " ";
            for (auto i : r.route)
                if (i.second > 0)
                    out << i.first << " -(" << std::to_string(i.second) << ")-> ";
                else
                    out << i.first;
        }
        return out;
    }
    int initialCapacity;            /**< Initial capacity of the route, equals to VRP.capacity */
    float initialWorkTime;          /**< Total work time for driver, equals to VRP.workTime */
    int capacity;                   /**< Capacity remaining */
    float workTime;                 /**< Work time remaining */
    int totalCost;                  /**< Total cost of the route: sum of the weight */
    float averageCost;              /**< Average of all path costs */
    // cost of traveling
    const float TRAVEL_COST = 0.3f; /**< Parameter for each travel */
    Graph graph;                    /**< Graph of the customers */
protected:
    std::list<StepType> route;      /**< This list represent the route */
public:
    /** @brief overriding of the "=" operator for assign a route to another */
    Route& operator=(const Route &r) {
        this->capacity = r.capacity;
        this->workTime = r.workTime;
        this->totalCost = r.totalCost;
        this->route = r.route;
        this->initialCapacity = r.initialCapacity;
        this->initialWorkTime = r.initialWorkTime;
        this->graph = r.graph;
        return *this;
    }
    // constructor
    Route(int, int, const Graph);       //!< constructor
    void CloseTravel(const Customer);
    bool CloseTravel(const Customer, const Customer);
    void PrintRoute();
    void PrintRoute(std::list<StepType>);
    bool Travel(const Customer, const Customer);
    void EmptyRoute(const Customer);
    int size() const;
    std::list<StepType>* GetRoute();
    Route CopyRoute() const;
    bool AddElem(const Customer);
    bool AddElem(const std::list<Customer>);
    bool AddElem(const Customer, const Customer);
    void RemoveCustomer(std::list<StepType>::iterator &);
    void RemoveCustomer(const Customer);
    int GetTotalCost() const;
    void SetAverageCost();
    void GetUnderAverageCustomers(std::list<Customer> &);
    bool RebuildRoute(std::list<Customer>);
};

#endif /* Route_H */