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

/** @brief ###Configure variable and routes
 *
 * The controller start the program settings all the variable and calling
 * the functions to configure the routes.
 * @param[in] argv The arguments passed through command line.
 * @param[in] costTravel The cost of travelling.
 * @param[in] alphaParam Alpha parameter for route evaluation.
 * @param[in] aspiration Aspiration factor.
 * @param[in] max_time Maximum execution time in minutes.
 */
void Controller::Init(char **argv, float costTravel, float alphaParam, int aspiration, int max_time) {
    this->MAX_TIME_MIN = max_time;
    Utils &u = this->GetUtils();
    u.logger("Initializing...", u.INFO);
    this->vrp = Utils::Instance().InitParameters(argv, costTravel, alphaParam, aspiration);
    int res = this->vrp->InitSolutions();
    switch (res) {
        case -1:
            u.logger("You need less vehicles.", u.WARNING);
        break;
        case 0:
            u.logger("Done!", u.SUCCESS);
        break;
        case 1:
            throw "You need more vehicles";
        break;
    }
    int initCost = this->vrp->GetTotalCost();
    this->PrintRoutes();
    this->RunVRP();
    int finalCost = this->vrp->GetTotalCost();
    int percCost = (float)(finalCost-initCost)/initCost * 100;
    this->PrintRoutes();
    u.logger("Total improvement: " + std::to_string(initCost - finalCost) + " " + std::to_string(percCost) + "%", u.INFO);
    this->SaveResult();
}

/** @brief ###Runs all the main functions
 *
 * This function sets and call the tabu search and optimal functions.
 * If the routines do not improves the solutions set stop.
 */
void Controller::RunVRP() {
    // if number of customers is high, reduce (for execution time)
    int customers = this->vrp->GetNumberOfCustomers();
    // number of opt functions executions
    int timeOpts = customers / 7;
    bool ts, opt;
    int noimprov = 0, duration = 0;
    // start time
    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
    // run the routines for 'customers' times or stop if no improvement or duration is more than ~3h
    for (int i = 0; i < customers && noimprov < 5 && duration <= 200; i++) {
        ts = this->RunTabuSearch(customers * 0.9);
        Utils::Instance().logger("Starting opt", Utils::VERBOSE);
        opt = this->RunOpts(timeOpts);
        if (!ts && !opt)
            noimprov++;
        // partial time
        std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::minutes>(t2 - t1).count();
    }
}
/** @brief ###Runs the tabu search function.
 *
 * @param[in] times Number of iteration.
 * @return          If the routine made some improvements.
 */
bool Controller::RunTabuSearch(int times) {
    int initCost = this->vrp->GetTotalCost();
    Utils::Instance().logger("Starting Tabu Search", Utils::VERBOSE);
    for (int k = 0; k < times; ++k) {
        this->vrp->TabuSearch();
    }
    int diffCost = initCost - this->vrp->GetTotalCost();
    if (diffCost > 0) {
        Utils::Instance().logger("Tabu Search improved: " + std::to_string(diffCost), Utils::VERBOSE);
        return true;
    }else {
        Utils::Instance().logger("Tabu Search no improvement", Utils::VERBOSE);
        return false;
    }
}

/** @brief ###Run optimal functions.
 *
 * Runs all the optimal functions to achieve a better optimization of the routes.
 * When the routine do not improve the routes, stops and try to balance the routes.
 * @param[in] times Number of iteration
 * @return          If the routine made some improvements.
 */
bool Controller::RunOpts(int times) {
    this->vrp->RouteBalancer();
    int i = 0;
    bool result, optxx = true;
    // against starvation
    int duration = 0;
    // start time
    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
    // do at least 4 iteration in less than 30 minutes
    while (i < times && !(i > 3 && duration >= 30)) {
        Utils::Instance().logger("Round " + std::to_string(i), Utils::VERBOSE);
        if (optxx) {
            result = this->vrp->Opt10();
            if (!result)
                result = this->vrp->Opt01();
            if (!result)
                result = this->vrp->Opt11();
            if (!result)
                result = this->vrp->Opt12();
            /*if (!result)
                result = this->vrp->Opt21();
            if (!result)
                result = this->vrp->Opt22();*/
        }
        // if no more improvements run only 2-opt and 3-opt
        if (!result)
            optxx = false;
        if (this->vrp->Opt2())
            optxx = true;
        if (this->vrp->Opt3())
            optxx = true;
        if (!result && !optxx)
            break;
        i++;
        // partial time
        std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::minutes>(t2 - t1).count();
    }
    this->vrp->RouteBalancer();
    // if no improvements return false;
    return !(!result && !optxx);
}

Utils& Controller::GetUtils() const {
    return Utils::Instance();
}

/** @brief ###Print all routes.
 *
 * Prints all routes with costs and in a more readable way.
 */
void Controller::PrintRoutes() {
    Utils &u = this->GetUtils();
    std::list<Route> *e = this->vrp->GetRoutes();
	std::cout << std::endl;
    for (auto i = e->cbegin(); i != e->cend(); i++) {
        u.logger(*i);
        std::advance(i, 1);
        if (i == e->cend()) break;
        u.logger(*i, u.SUCCESS);
    }
    u.logger("Total cost: " + std::to_string(this->vrp->GetTotalCost()), u.INFO);
	std::cout << std::endl;
}

/** @brief ###Save results.
 *
 * The results from the program are saved into a JSON file called 'output.json'
 * in the same folder of the executable file.
 */
void Controller::SaveResult() {
    Utils &u = this->GetUtils();
    u.logger("Saving to output.json", u.VERBOSE);
    std::list<Route> *e = this->vrp->GetRoutes();
    u.SaveResult(*e);
}
