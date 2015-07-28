#ifndef Customer_H
#define Customer_H

#include <iostream>

class Customer {
private:
    /* overloading < */
    friend bool operator< (const Customer &c1, const Customer &c2) {
        return c1.name < c2.name;
    }
    friend std::ostream &operator<<(std::ostream &out, Customer c) {
        return out << c.name;
    }
public:
    bool operator==(const Customer &c) const {
        return name == c.name;
    }
    std::string name;
    int x;
    int y;
    int request;
    int serviceTime;
    Customer() {}
    Customer(std::string name, int x, int y, int r, int t);
    Customer(std::string v0, int x, int y);
    ~Customer();
};
namespace std {
    template <>
    struct hash<Customer> {
        std::size_t operator()(const Customer& c) const {
            using std::hash;
            using std::string;

      // Compute individual hash values for first,
      // second and third and combine them using XOR
      // and bit shifting:

            return hash<string>()(c.name);
        }
    };
}


#endif /* Vehicle_H */
