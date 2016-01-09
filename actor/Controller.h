#ifndef Controller_H
#define Controller_H

#include "Utils.h"
#include "VRP.h"

class Controller {
private:
    VRP *vrp;
    Controller() {};
    Controller(Controller const&) = delete;
    void operator=(Controller const&) = delete;
    void RunOpts(int);
    void RunVRP();
    void RunTabuSearch(int);
    void SaveResult();
protected:
    void PrintRoutes();
public:
    static Controller& Instance() {
        static Controller instance;
        return instance;
    }
    void Init(char **argv, float, float, int);
    Utils& GetUtils() const;
};

#endif /* Controller_H */