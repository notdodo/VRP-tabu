/*****************************************************************************
    This file is part of VRP.

    VRP is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    VRP is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with VRP.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/

#include "Customer.h"

/** @brief ###Constructor of the Customer
 *
 * @param n Name of the customer
 * @param x Abscissa of the customer
 * @param y Ordinate of the customer
 * @param r Requested quantity for the customer
 * @param t Time for serving the customer
 */
Customer::Customer(std::string n, int x, int y, int r, int t) {
    this->name = n;
    this->x = x;
    this->y = y;
    this->request = r;
    this->serviceTime = t;
}

/** @brief ###Constructor of the depot
 *
 * @param n Name of the customer
 * @param x Abscissa of the customer
 * @param y Ordinate of the customer
 */
Customer::Customer(std::string n, int x, int y) {
    this->name = n;
    this->x = x;
    this->y = y;
    this->request = 0;
    this->serviceTime = 0;
}

/* destructor */
Customer::~Customer() {}