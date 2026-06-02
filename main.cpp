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

#include "Controller.h"

using namespace std;
using namespace chrono;

namespace {
constexpr float kTravelCost = 0.3f;
constexpr float kAlpha = 0.4f;
constexpr int kMaxTimeMin = 300;
} // namespace

int main(int argc, char** argv) {
    high_resolution_clock::time_point t1 = high_resolution_clock::now();
    int exitCode = 0;
    // create the controller
    Controller& c = Controller::Instance();
    Utils& u = Utils::Instance();
    try {
        c.Init(argc, argv, kTravelCost, kAlpha, kMaxTimeMin);
        c.PrintRoutes();
        c.SaveResult();
        c.RunVRP();
        c.PrintBestRoutes();
    } catch (const std::exception& e) {
        u.logger(e.what(), u.ERROR);
        exitCode = 1;
    }
    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    const auto duration = duration_cast<milliseconds>(t2 - t1).count();
    // print duration
    u.logger(to_string(duration) + " milliseconds", u.INFO);
    return exitCode;
}

/*
 * Tabu Search.
 *
 *          Tabu Search is a metaheuristic for hard optimization problems.
 *          Starting from a feasible solution, it moves through neighboring
 *          solutions while short-term memory discourages cycles by marking
 *          recently used moves as tabu.
 *
 * Problem.
 *          A VRP instance contains:
 *              - a depot and a set of customers;
 *              - vehicle capacity and optional work-time constraints;
 *              - travel costs between customers;
 *              - a feasible solution made of routes that visit each customer once.
 *          The objective is to minimize the total route cost.
 *
 * Solution.
 *          The solver starts from a feasible route set, improves it with local
 *          search, then uses Tabu Search to continue exploring customer
 *          relocations that may escape local minima.
 *
 * Neighborhood.
 *          Given a route set s, the neighborhood N(s) is composed of feasible
 *          customer-relocation moves:
 *              - remove one customer from its source route;
 *              - insert it into a destination route;
 *              - keep the route set feasible after the move.
 *          Candidate moves are evaluated on route copies, so rejected moves do
 *          not mutate the incumbent solution.
 *
 * Parallel evaluation.
 *          Candidate relocations are evaluated in parallel. Each candidate keeps
 *          its original scan-order sequence number; after all workers finish,
 *          results are sorted by that number and reduced deterministically.
 *
 * Move.
 *          A tabu move is represented by:
 *              - the customer being relocated;
 *              - the destination route index;
 *              - the source route index.
 *          The tabu list stores these move attributes, not complete route sets.
 *
 * Tabu list and memory.
 *          Recent moves receive a temporary penalty to discourage immediate
 *          reversal and short cycles. A tabu move can still be accepted by the
 *          aspiration rule when it does not worsen the destination route
 *          assessment.
 *
 * Diversification.
 *          If the search does not improve the global best solution, it can
 *          restart from one of the retained non-best candidates. This pushes the
 *          search into another region while keeping the process deterministic.
 */
