#include "lib/Utils.h"

using namespace std;

 int main(int argc, char** argv) {
    if (argc == 2) {
        Utils s;
        VRP *v = s.InitParameters(argv);
        if (v != NULL) v->log();
        s.SaveResult();
    }
    return 0;
}


/*
 * Tabu Search is a meta-heuristic method designed for the solution of hard optimization problems.
 * This algorithm starts form an initial solution and jumps from one solution to another one in the
 * space but tries to avoid cucling by forbidding or penalizing moves which take the solution, in
 * the next iteration, to solutions previously visited.
 *
 * Problem.
 *          The optimization problem consisting of:
 *              - a set of istances I;
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
 *          TS establishes an aspiration criteria so that tabu moves can be accepted if the satify
 *          such a criteria.
 *
 * Taby list and efficient use of memory.
 *          To prevent the search from cycling between the same solutions, TS uses a short term memory
 *          - the so-called tabu list- to the aim of representing the trajectory of solutions encountered.
 *          Based on certain restrictions the tabu list implicity keeps track of moves by recording
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