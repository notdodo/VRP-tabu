#ifndef Customer_H
#define Customer_H

#include <iostream>

class Customer {
private:
    /** @brief Overriding '<' operator to evaluate two customers */
    friend bool operator< (const Customer &c1, const Customer &c2) {
        return c1.name < c2.name;
    }
    /** @brief Overriding '<<' operator for printing the customer */
    friend std::ostream &operator<<(std::ostream &out, Customer c) {
        out.flush();
        out << c.name;
        out.flush();
        return out;
    }
public:
    /** @brief Overriding '==' operator to evaluate two customers */
    bool operator==(const Customer &c) const {
        return name == c.name;
    }
    /** @brief Overriding '!=' operator to evaluate two customers */
    bool operator!=(const Customer&c) const {
        return name != c.name;
    }
    std::string name;       /**< Name of the customer */
    int x;                  /**< Coordinate X of the customer */
    int y;                  /**< Coordinate Y of the customer */
    int request;            /**< Quantity request from the customer */
    int serviceTime;        /**< Time for serving the customer */
    Customer() {}                                  //!< Constructor
    Customer(std::string, int, int, int, int);     //!< Constructor
    Customer(std::string, int, int);               //!< Constructor
    ~Customer();                                   //!< Destructor
};

#endif /* Vehicle_H */
