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
        out.flush();
        out << c.name;
        out.flush();
        return out;
    }
public:
    bool operator==(const Customer &c) const {
        return name == c.name;
    }
    bool operator!=(const Customer&c) const {
        return name != c.name;
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

#endif /* Vehicle_H */
