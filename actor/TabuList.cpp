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

/** @brief ###Add a tabu move to the list.
 *
 * This function insert a pair of Customer and Route and increment the counter
 * of 'how many times was used' the move.
 * @param[in] m    The move
 * @param[in] time Times the move is tabu
 */
void TabuList::AddTabu(Move m, int time) {
    float adj = 0.0f;
    // add the move to the solution moves for history searching and penalization
    auto findIterBest = std::find_if(this->bestMoves.begin(), this->bestMoves.end(),
            [m](TabuElement &e) {
                return m.first.first == e.first.first.first && m.first.second == e.first.second && m.second == e.second;
            });
    if (findIterBest != this->bestMoves.end()) {
        findIterBest->second += 1.0f;
        adj = findIterBest->second;
    }else
        this->bestMoves.emplace_back(std::make_pair(m, 1.0f));
    // add the move the tabulist
    auto findIter = std::find_if(this->tabulist.begin(), this->tabulist.end(),
            [m](TabuElement &e) {
                return m.first.first == e.first.first.first && m.first.second == e.first.second && m.second == e.second;
            });
    if (findIter != this->tabulist.end())
        findIter->second += 1;
    else
        this->tabulist.emplace_front(std::make_pair(m, (int)(time + (adj * time))));
}

/** @brief ###Remove a tabu move from the list
 *
 * Invalidate all moves which contain the route.
 * @param[in] p The move to remove
 */
void TabuList::RemoveTabu(Move p) {
    this->tabulist.remove_if([p](const TabuElement &e)->bool {
        return (p.first.first == e.first.first.first && p.first.second == e.first.second);
    });
}

void TabuList::IncrementSize() { if (this->size < 15) this->size++; }
void TabuList::DecrementSize() { if(this->size > 7) this->size--; }

/** @brief ###Aspiration criteria
 *
 * The aspiration criteria decrease the score of tabu moves until their are not
 * tabu anymore.
 */
void TabuList::Clean() {
    // 0.1 523/525
    for (auto &it : this->tabulist)
        it.second -= 1;
    for (auto &it : this->bestMoves)
        it.second -= 0.1f;
    this->tabulist.sort([]( const auto &a, const auto &b ) { return a.second > b.second; } );
    this->tabulist.remove_if([](auto l){ return l.second < 1; });
    std::sort(this->bestMoves.begin(), this->bestMoves.end(), [](const auto &lhs, const auto &rhs){ return lhs.second > rhs.second; });
    std::remove_if(this->bestMoves.begin(), this->bestMoves.end(), [](auto l){ return l.second <= 0.0f;});
    this->tabulist.resize(this->size);
}

/** @brief ###Clear the list */
void TabuList::FlushTabu() {
    this->tabulist.clear();
}

/** @brief ###Find a move in the list
 *
 * This function find is a move in the tabulist can be processed; plus, when customer
 * i was previously removed from route k, its reinsertion in that route is forbidden.
 * @param[in] m The move to find
 * @return If the move is in list
 */
bool TabuList::Find(Move m) const {
    auto findIter = std::find_if(this->tabulist.cbegin(), this->tabulist.cend(),
            [m](const TabuElement &e) {
                // if in the list there is a move which the customer is the same as 's', the second
                // element of TabuElement is the same as s.second or the move is the same
                return ((m.first.first == e.first.first.first && m.first.second == e.first.second) ||
                    (m.first.first == e.first.first.first && m.first.second == e.first.first.second && m.second == e.first.second));
            });
    return findIter != this->tabulist.cend();
}

/** @brief ###Number of times the move is added to a solution
 *
 * Finds and returns the number of times a move is added to a solution
 * @param[in] m The move to search for
 * @return      Times the move is added to a solution
 */
float TabuList::Check(Move m) const {
    auto findIter = std::find_if(this->bestMoves.cbegin(), this->bestMoves.cend(),
            [m](const TabuElement &e) {
                return (m.first.first == e.first.first.first && m.first.second == e.first.first.second);
            });
    return (findIter != this->bestMoves.cend())? findIter->second : 0;
}