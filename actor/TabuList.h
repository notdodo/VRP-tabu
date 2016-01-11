#ifndef TabuList_H
#define TabuList_H

#include "Route.h"
#include "Customer.h"

typedef std::pair<Customer, Route> TabuKey;

class TabuList {
private:
	/** @brief overriding of the "<<" operator to print the list, for debugging */
    friend std::ostream& operator<<(std::ostream& out, const TabuList &l) {
        if (l.tabulist.size() > 0) {
        	for (auto i : l.tabulist) {
        		out << "{ " << std::setw(3) << i.first.first << ", " << i.first.second << " }, " << i.second << std::endl;
        	};
        };
        return out;
    }
	/** @brief Function for ordering the elements inside the std::map */
	struct tabucomp {
  		bool operator() (const TabuKey& lhs, const TabuKey& rhs) const {
  			if (lhs.first == rhs.first) {
  				return lhs.second.GetTotalCost() < rhs.second.GetTotalCost();
  			} else
  				return lhs.first < rhs.first;
  		};
	};
	std::map<TabuKey, float, tabucomp> tabulist; 	/**< List of all tabu moves*/
	int ASPIRATION_FACTOR;					              /**< Aspiration factor of moves */
public:
  TabuList() {};
	TabuList(const float);   			                //! constructor
  int size() const;
	void AddElement(TabuKey, float);
	void Clean();
	void FlushList();
	bool Find(TabuKey) const;
};

#endif /* TabuList_H */