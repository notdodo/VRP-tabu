#include "Customer.h"

/* constructor */
Customer::Customer() {}
Customer::Customer(std::string name, int x, int y, int r, int t) {
    this->name = name;
    this->x = x;
    this->y = y;
    this->request = r;
    this->serviceTime = t;
}

/* destructor */
Customer::~Customer() {}