#ifndef Controller_H
#define Controller_H

#include "VRP.h"
#include <chrono>

class Controller {
private:
    VRP *vrp;
    Controller() {};
    Controller(Controller const&) = delete;
    void operator=(Controller const&) = delete;
    int RunTabuSearch(int);
    int MAX_TIME_MIN;
    int initCost;
    int finalCost;
    std::chrono::high_resolution_clock::time_point startTime;
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
