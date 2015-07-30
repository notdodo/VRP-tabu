#include "Customer.h"

/* constructor */
Customer::Customer(std::string name, int x, int y, int r, int t) {
    this->name = name;
    this->x = x;
    this->y = y;
    this->request = r;
    this->serviceTime = t;
}

Customer::Customer(std::string name, int x, int y) {
    this->name = name;
    this->x = x;
    this->y = y;
    this->request = 0;
    this->serviceTime = 0;
}

/* destructor */
Customer::~Customer() {}