#ifndef Controller_H
#define Controller_H

#include "Utils.h"
#include "VRP.h"

class Controller {
private:
    VRP v;
public:
    Controller();
    void Init(char **argv);
};

#endif /* Controller_H */