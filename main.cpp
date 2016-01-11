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
#include <chrono>

using namespace std;
using namespace chrono;

#define TRAVEL_COST 0.3f
#define ALPHA 0.4f
#define ASPIRATION_FACTOR 3000
#define MAX_TIME_MIN 200

int main(int argc, char** argv) {
    high_resolution_clock::time_point t1 = high_resolution_clock::now();
    // create the controller
    Controller &c = Controller::Instance();
    Utils &u = Utils::Instance();
    // parse arguments
    if (argc == 2 || argc == 3) {
        try {
            c.Init(argv, TRAVEL_COST, ALPHA, ASPIRATION_FACTOR, MAX_TIME_MIN);
        }catch(const char *i) {
            u.logger(i, u.ERROR);
        }
    }else
        u.logger("Usage: ./VRP [-v] data.json", u.ERROR);

    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    int duration = duration_cast<milliseconds>(t2 - t1).count();
    // print duration
    u.logger(to_string(duration) + " milliseconds", u.INFO);
    return 0;
}


/*
 * Tabu Search is a meta-heuristic method designed for the solution of hard optimization problems.
 * This algorithm starts form an initial solution and jumps from one solution to another one in the
 * space but tries to avoid cycling by forbidding or penalizing moves which take the solution, in
 * the next iteration, to solutions previously visited.
 *
 * Problem.
 *          The optimization problem consisting of:
 *              - a set of instances I;
 *              - to a given x in I, there corresponds a set of  feasible solutions S(x)
 *              - a cost function f: S -> R associating a cost f(x) to solution s in S.
 *          The solution s* in S should respect some criteria (minimization or maximization).
 *
 * Solution.
 *          Tabu Search seeks for a feasible solution but the acceptability criteria for TS is to
 *          find a solution s' in S of cost f(s') as close as possible to the optimum cost f(s*).
 *
 * Neighborhood.
 *          Give a solution s, the set of all possible solutions that are reached from s in a single
 *          move, is referred to as the neighborhood of s, denoted by N(s).
 *          TS moves from s to s' in N(s) which is the best among all (or part of) possible solutions
 *          in N(s). The choice of next solution s' is crucial to the whole process. To carry it out
 *          recently visited solutions (marked as tabu) should be avoided.
 *
 * Move.
 *          A transition from a feasible solution to another feasible solution is called move.
 *          Moves can be given the tabu status if they lead to previously visited solutions, but
 *          TS establishes an aspiration criteria so that tabu moves can be accepted if the solution
 *          satisfy such a criteria.
 *
 * Tabu list and efficient use of memory.
 *          To prevent the search from cycling between the same solutions, TS uses a short term memory
 *          - the so-called tabu list- to the aim of representing the trajectory of solutions encountered.
 *          Based on certain restrictions the tabu list implicitly keeps track of moves by recording
 *          their attributes. The goal is to permit "good" moves in each iteration without re-visiting
 *          solutions already encountered. The key point to the TS procedure lies in the tabu list
 *          management.
 *
 * Intensification.
 *          TS incorporates an intensification procedure that consists in rewarding solutions having
 *          features in common with the current solution and penalizing solutions that are far from the
 *          current solution.
 *
 * Diversification.
 *          TS method launches the diversification procedure to spread out the search in another region.
 *          The diversification try to find out good solutions in another region (local minima).
 *
 */
