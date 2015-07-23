#ifndef VRP_H
#define VRP_H

#include "../actor/Customer.h"
#include <iostream>

class VRP {
private:
    Customer *customers;
    int numVertices;
    int vehicles;
    int capacity;
    int workTime;
public:
    VRP() {}
    VRP(Customer &cust, int n, int v, int c, int t);
    ~VRP();
};

#endif /* VRP_H */