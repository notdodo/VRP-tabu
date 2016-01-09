/*****************************************************************************
    This file is part of VRP.

    VRP is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    VRP is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with VRP.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/

#include "TabuList.h"

TabuList::TabuList(const float aspiration) {
    this->ASPIRATION_FACTOR = aspiration;
    this->FlushList();
}

/** @brief ###Add a tabu move to the list.
 *
 * This function insert a pair of Customer and Route as key of the map and the
 * evaluation value of the router as value.
 * @param p The pair Customer and Route
 * @param v The evaluation value of the router
 */
void TabuList::AddElement(TabuKey p, float v) {
    this->tabulist.insert({p, v});
}

/** @brief ###Aspiration criteria
 *
 * The aspiration criteria decrease the score of tabu moves until their are not
 * tabu anymore.
 */
void TabuList::Aspiration() {
    std::map<TabuKey, float>::iterator it = this->tabulist.begin();
    for (; it != this->tabulist.end();) {
        it->second -= this->ASPIRATION_FACTOR;
        if (it->second <= 0)
            it = this->tabulist.erase(it);
        else
            ++it;
    }
}

/** @brief ###Clear the list */
void TabuList::FlushList() {
    this->tabulist.clear();
}

/** @brief ###Find a move in the list
 *
 * This function find a move, as pair of Customer-Route, in the tabu list.
 * @param p The move to find
 * @return If the move is in list
 */
bool TabuList::Find(TabuKey p) const {
    if (this->tabulist.size() > 0)
        return this->tabulist.find(p) != this->tabulist.end();
    else
        return false;
}