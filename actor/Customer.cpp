#include "Customer.h"

/** @brief Constructor of the Customer
 *
 * @param name Name of the customer
 * @param x Abscissa of the customer
 * @param y Ordinate of the customer
 * @param r Requested quantity for the customer
 * @param t Time for serving the customer
 */
Customer::Customer(std::string name, int x, int y, int r, int t) {
    this->name = name;
    this->x = x;
    this->y = y;
    this->request = r;
    this->serviceTime = t;
}

/** @brief Constructor of the depot
 *
 * @param name Name of the customer
 * @param x Abscissa of the customer
 * @param y Ordinate of the customer
 */
Customer::Customer(std::string name, int x, int y) {
    this->name = name;
    this->x = x;
    this->y = y;
    this->request = 0;
    this->serviceTime = 0;
}

/* destructor */
Customer::~Customer() {}