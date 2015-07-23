#ifndef Customer_H
#define Customer_H

#include <string>

class Customer {
private:

public:
    std::string name;
    int x;
    int y;
    int request;
    int serviceTime;
    Customer() {}
    Customer(std::string name, int x, int y, int r, int t);
    ~Customer();
};

#endif /* Vehicle_H */