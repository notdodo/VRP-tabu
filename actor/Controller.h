#ifndef Controller_H
#define Controller_H

#include "../lib/Utils.h"
#include <iostream>

class Controller {
private:
    VRP v;
public:
    Controller();
    void Init(char **argv);
};

#endif /* Controller_H */