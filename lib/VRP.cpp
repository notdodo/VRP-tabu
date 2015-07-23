#include "VRP.h"

/* constructor */
VRP::VRP(Customer &cust, int n, int v, int c, int t) {
    this->customers = &cust;
    this->numVertices = n;
    this->vehicles = v;
    this->capacity = c;
    this->workTime = t;
}

/* destructor */
VRP::~VRP() {
}