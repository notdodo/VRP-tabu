#ifndef Customer_H
#define Customer_H

#include <cstddef>
#include <iostream>
#include <limits>
#include <string>
#include <utility>

/** @brief Problem node containing coordinates, demand, and service time.
 *
 * Customers are value objects and are ordered by name so they can be used as
 * keys in graph and route containers. The depot is represented with the same
 * type, usually with zero demand and service time.
 */
class Customer {
  private:
    /** @brief Order customers by name for associative containers. */
    friend bool operator<(const Customer& c1, const Customer& c2) { return c1.name < c2.name; }

    /** @brief Print the customer name. */
    friend std::ostream& operator<<(std::ostream& out, const Customer& c) {
        out.flush();
        out << c.name;
        out.flush();
        return out;
    }

  public:
    static constexpr std::size_t invalidGraphIndex = std::numeric_limits<std::size_t>::max();

    /** @brief Assign all customer attributes from another customer. */
    Customer& operator=(const Customer&) = default;

    /** @brief Check identity by unique customer name. */
    bool operator==(const Customer& c) const { return name == c.name; }

    /** @brief Check whether two customer identities differ. */
    bool operator!=(const Customer& c) const { return !(*this == c); }

    std::string name;                           /**< Name of the customer */
    int x = 0;                                  /**< Coordinate X of the customer */
    int y = 0;                                  /**< Coordinate Y of the customer */
    int request = 0;                            /**< Demand requested by the customer */
    int serviceTime = 0;                        /**< Time for serving the customer */
    std::size_t graphIndex = invalidGraphIndex; /**< Dense graph matrix index, assigned by Graph */

    /** @brief Create an empty customer. */
    Customer() = default;

    /** @brief Create a customer with demand and service time. */
    Customer(std::string n, int x, int y, int r, int t) : name(std::move(n)), x(x), y(y), request(r), serviceTime(t) {}

    /** @brief Create a depot-like customer with no demand or service time. */
    Customer(std::string n, int x, int y) : name(std::move(n)), x(x), y(y) {}

    Customer(const Customer&) = default;
    ~Customer() = default;
};

#endif /* Vehicle_H */
