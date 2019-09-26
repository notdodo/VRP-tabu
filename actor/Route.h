#ifndef Route_H
#define Route_H

#include "Graph.h"
#include <list>
#include <iomanip>
#include <algorithm>
#include <cmath>

typedef std::pair<Customer, int> StepType;

class Route {
private:
    /** @brief ###Overriding of the "<<" operator to print the route */
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
    int initialCapacity;                            /**< Initial capacity of the route, equals to VRP.capacity */
    float initialWorkTime;                          /**< Total work time for driver, equals to VRP.workTime */
    int capacity;                                   /**< Capacity remaining */
    float workTime;                                 /**< Work time remaining */
    int totalCost;                                  /**< Total cost of the route: sum of the weight */
    float averageCost;                              /**< Average of all path costs */
    float TRAVEL_COST;                              /**< Cost parameter for each travel */
    float ALPHA;                                    /**< Alpha parameter for route evaluation */
    Graph graph;                                    /**< Graph of the customers */
protected:
    std::list<StepType> route;                      /**< This list represent the route */
    void EmptyRoute(const Customer);
public:
    /** @brief ###Overriding of the "=" operator for assign a route to another */
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
        this->averageCost = r.averageCost;
        return *this;
    }
	/** @brief ###Overriding '!=' operator to evaluate two routes */
    bool operator!=(const Route &r) const {
        auto j = r.route.cbegin();
        for (auto &i : route) {
            if (i.first != j->first)
                return true;
            ++j;
        }
        return false;
    }

    /** @brief ###Overriding '==' operator to evaluate two routes */
    bool operator==(const Route &r) const {
        auto i = route.cbegin();
        auto j = r.route.cbegin();
        for (; i != route.cend(); ++i) {
            if (i->first != j->first)
                return false;
            ++j;
        }
        return true;
    }
    /** @brief ###Overriding '<' operator to evaluate two routes */
    bool operator < (const Route &r) const {
        return GetTotalCost() < r.GetTotalCost();
    }
    /** @brief ###Overriding '>=' operator to evaluate two routes */
    bool operator <= (const Route &r) const {
        return GetTotalCost() >= r.GetTotalCost();
    }

    Route(const int, const float, const Graph, const float, const float);       //!< constructor
    Route(const Route&) = default;
    void CloseTravel(const Customer);
    bool CloseTravel(const Customer, const Customer);
    void PrintRoute() const;
    bool Travel(const Customer, const Customer);
    int size() const;
    std::list<StepType>* GetRoute();
    bool AddElem(const Customer);
    bool AddElem(const std::list<Customer> &);
    void RemoveCustomer(std::list<StepType>::iterator &);
    bool RemoveCustomer(const Customer);
    int GetTotalCost() const;
    void SetAverageCost();
    float GetAverageCost() const;
    void GetUnderAverageCustomers(std::list<Customer> &);
    float GetDistanceFrom(Route);
	bool FindCustomer(const Customer);
    bool RebuildRoute(std::list<Customer>);
    float Evaluate();
};

#endif /* Route_H */
