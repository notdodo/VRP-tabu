#ifndef TabuList_H
#define TabuList_H

#include "Route.h"
#include "Customer.h"
#include <forward_list>

/** @brief Move descriptor: customer, destination route index, and source route index. */
using Move = std::pair<std::pair<Customer, int>, int>;

/** @brief Stored tabu move with its remaining tenure/score. */
using TabuElement = std::pair<Move, float>;

/** @brief Short-term memory for tabu search moves.
 *
 * The list tracks recently applied customer relocations so the search avoids
 * immediately undoing them. It also stores non-best moves used by the
 * diversification phase.
 */
class TabuList {
  private:
    std::forward_list<TabuElement> tabulist; /**< List of all tabu moves */
    std::vector<TabuElement> nonBestMoves;
    unsigned size = 7;

    /** @brief Remove all tabu entries from the active list. */
    void FlushTabu();

  public:
    /** @brief Create a tabu list with the default tenure. */
    TabuList() = default;

    /** @brief Create a tabu list with an explicit tenure. */
    TabuList(int n) : size(n) {};

    /** @brief Increase tabu tenure. */
    void IncrementSize();

    /** @brief Decrease tabu tenure without going below the implementation limit. */
    void DecrementSize();

    /** @brief Add a move to tabu memory. */
    void AddTabu(const Move&, float);

    /** @brief Remove a move from tabu memory. */
    void RemoveTabu(const Move&);

    /** @brief Age tabu entries and discard expired moves. */
    void Clean();

    /** @brief Check whether a move is currently tabu. */
    [[nodiscard]] bool Find(const Move&) const;

    /** @brief Return the tabu penalty associated with a move. */
    [[nodiscard]] float Check(const Move&) const;
};

#endif /* TabuList_H */
