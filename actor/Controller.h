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
    bool RunTabuSearch(int);
    int MAX_TIME_MIN;
    int initCost;
    int finalCost;
protected:
public:
    static Controller& Instance() {
        static Controller instance;
        return instance;
    }
    void Init(int, char **argv, float, float, int);
    void RunVRP();
    void SaveResult();
    void PrintRoutes();
	void PrintBestRoutes();
    Utils& GetUtils() const;
};

#endif /* Controller_H */
