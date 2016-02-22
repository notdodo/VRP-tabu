#ifndef OptimalMove_H
#define OptimalMove_H

#include "Route.h"
#include "Utils.h"
#include "../lib/ThreadPool.h"
#include <thread>
#include <mutex>

typedef std::list<Route> Routes;
// contains: the index of the two routes and the two routes
typedef std::pair<std::pair<int, int>, std::pair<Route, Route>> BestResult;

auto comp = [](const BestResult &l, const BestResult &r)->bool {
    return (l.second.first.GetTotalCost() + l.second.second.GetTotalCost()) <
        (r.second.first.GetTotalCost() + r.second.second.GetTotalCost());
};

class OptimalMove {
private:
	std::mutex mtx;
    const unsigned cores;
    Route Opt2Swap(Route, Customer, Customer);
    Route Opt3Swap(Route, Customer, Customer, Customer, Customer);
    float UpdateDistanceAverage(Routes);
	bool Move1FromTo(Route &, Route &, bool);
    bool SwapFromTo(Route &, Route &);
    bool AddRemoveFromTo(Route &, Route &, int, int);
public:
	OptimalMove(): cores(std::thread::hardware_concurrency() + 1) {};
	void CleanVoid(Routes &);
	int Opt10(Routes &, bool);
    int Opt01(Routes &, bool);
    int Opt11(Routes &, bool);
    int Opt21(Routes &, bool);
    int Opt12(Routes &, bool);
    int Opt22(Routes &, bool);
	void RouteBalancer(Routes &);
	bool Opt2(Routes &);
	bool Opt3(Routes &);
};

#endif /* OptimalMove_H */