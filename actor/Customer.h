#ifndef Customer_H
#define Customer_H

#include <iostream>

class Customer {
private:
    /** @brief ###Overriding '<' operator to evaluate two customers */
    friend bool operator<(const Customer &c1, const Customer &c2) {
        return c1.name < c2.name;
    }
    /** @brief ###Overriding '<<' operator for printing the customer */
    friend std::ostream &operator<<(std::ostream &out, Customer c) {
        out.flush();
        out << c.name;
        out.flush();
        return out;
    }
public:
    /** @brief ###Overriding of the "=" operator to assign a customer to another */
    Customer& operator=(const Customer &c) {
        this->name = c.name;
        this->x = c.x;
        this->y = c.y;
        this->request = c.request;
        this->serviceTime = c.serviceTime;
        return *this;
    }
    /** @brief ###Overriding '==' operator to evaluate two customers */
    bool operator==(const Customer &c) const {
        return name == c.name && x == c.x && y == c.y;
    }
    /** @brief ###Overriding '!=' operator to evaluate two customers */
    bool operator!=(const Customer&c) const {
        return x != c.x || y != c.y;
    }
    std::string name;       /**< Name of the customer */
    int x;                  /**< Coordinate X of the customer */
    int y;                  /**< Coordinate Y of the customer */
    int request;            /**< Quantity request from the customer */
    int serviceTime;        /**< Time for serving the customer */
    Customer() {};          //!< Constructor
    Customer(std::string n, int x , int y , int r, int t): name(n), x(x), y(y), request(r), serviceTime(t) {}    //!< Constructor
    Customer(std::string n, int x, int y): name(n), x(x), y(y), request(0), serviceTime(0) {}                    //!< Constructor
    Customer(const Customer&); //!< Constructor
    ~Customer();            //!< Destructor
};

#endif /* Vehicle_H */
