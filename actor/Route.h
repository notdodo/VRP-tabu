#ifndef Route_H
#define Route_H

#include "Graph.h"
#include <list>
#include <iomanip>
#include <algorithm>

typedef std::pair<Customer, int> StepType;

class Route {
private:
    /** @brief overriding of the "<<" operator to print the route */
    friend std::ostream& operator<<(std::ostream& out, const Route &r) {
        if (r.route.size() > 1) {
            out << "Cost:" << std::setw(5) << std::to_string(r.totalCost) << " ";
            for (auto i : r.route)
                if (i.second > 0)
                    out << i.first << " -(" << std::to_string(i.second) << ")-> ";
                else
                    out << i.first;
        };
        return out;
    }
    int initialCapacity;            /**< Initial capacity of the route, equals to VRP.capacity */
    float initialWorkTime;          /**< Total work time for driver, equals to VRP.workTime */
    int capacity;                   /**< Capacity remaining */
    float workTime;                 /**< Work time remaining */
    int totalCost;                  /**< Total cost of the route: sum of the weight */
    float averageCost;              /**< Average of all path costs */
    float TRAVEL_COST = 0.3f;       /**< Cost parameter for each travel */
    float ALPHA = 0.4f;             /**< Alpha parameter for route evaluation */
    int routeNumber;				/**< Identifier of the route */
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
        this->TRAVEL_COST = r.TRAVEL_COST;
        this->ALPHA = r.ALPHA;
        return *this;
    }
    Route(const int, const float, const Graph, const int, const float, const float);       //!< constructor
    void CloseTravel(const Customer);
    bool CloseTravel(const Customer, const Customer);
    void PrintRoute();
    void PrintRoute(std::list<StepType>);
    bool Travel(Customer, const Customer);
    void EmptyRoute(const Customer);
    int size() const;
    std::list<StepType>* GetRoute();
    Route CopyRoute() const;
    bool AddElem(const Customer);
    bool AddElem(const std::list<Customer> &);
    bool AddElem(const Customer, const Customer);
    void RemoveCustomer(std::list<StepType>::iterator &);
    bool RemoveCustomer(const Customer);
    int GetTotalCost() const;
    void SetAverageCost();
    void GetUnderAverageCustomers(std::list<Customer> &);
	int GetNumber() { return this->routeNumber; }
	bool FindCustomer(const Customer &);
    bool RebuildRoute(std::list<Customer>);
    float Evaluate();
};

#endif /* Route_H */
