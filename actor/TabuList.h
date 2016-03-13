#ifndef TabuList_H
#define TabuList_H

#include "Route.h"
#include "Customer.h"
#include <forward_list>

// insert the customer in the i-th route
typedef std::pair<std::pair<Customer, int>, int> Move;
// list of tabu moves: the customer inserted in i-th route from the k-th route
typedef std::pair<Move, float> TabuElement;

class TabuList {
private:
	std::forward_list<TabuElement> tabulist;			/**< List of all tabu moves */
	std::vector<TabuElement> nonBestMoves;
	unsigned size = 7;
	void FlushTabu();
public:
	TabuList() {};
	TabuList(int n) : size(n) {};
	void IncrementSize();
	void DecrementSize();
	void AddTabu(Move, int);
	void RemoveTabu(Move);
	void Clean();
	bool Find(Move) const;
	float Check(Move) const;
};

#endif /* TabuList_H */
