#ifndef Controller_H
#define Controller_H

#include "Utils.h"
#include "VRP.h"
#include <chrono>

class Controller {
private:
    VRP *vrp;
    Controller() {};
    Controller(Controller const&) = delete;
    void operator=(Controller const&) = delete;
    bool RunOpts(int);
    void RunVRP();
    bool RunTabuSearch(int);
    void SaveResult();
    int MAX_TIME_MIN;
protected:
    void PrintRoutes();
public:
    static Controller& Instance() {
        static Controller instance;
        return instance;
    }
    void Init(char **argv, float, float, int, int);
    Utils& GetUtils() const;
};

#endif /* Controller_H */