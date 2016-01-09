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
 */
void Controller::Init(char **argv, float costTravel, float alphaParam, int aspiration) {
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
 */
void Controller::RunVRP() {
    int customers = this->vrp->GetNumberOfCustomers();
    for (int i = 0; i < customers; i++) {
        this->RunTabuSearch(customers);
        Utils::Instance().logger("Starting opt", Utils::VERBOSE);
        this->RunOpts(customers);
    }
}
/** @brief ###Runs the tabu search function.
 *
 * @param[in] times Number of iteration.
 */
void Controller::RunTabuSearch(int times) {
    int initCost = this->vrp->GetTotalCost();
    Utils::Instance().logger("Starting Tabu Search", Utils::VERBOSE);
    for (int k = 0; k < times; ++k) {
        this->vrp->TabuSearch();
    }
    Utils::Instance().logger("Tabu Search improved: " + std::to_string(initCost - this->vrp->GetTotalCost()), Utils::VERBOSE);
}

/** @brief ###Run optimal functions.
 *
 * Runs all the optimal functions to achieve a better optimization of the routes.
 * When the routine do not improve the routes, stops and try to balance the routes.
 * @param[in] times Number of iteration
 */
void Controller::RunOpts(int times) {
    this->vrp->RouteBalancer();
    int i = 0;
    bool result, optxx = true;
    while (i < times) {
        Utils::Instance().logger("Round " + std::to_string(i), Utils::VERBOSE);
        if (optxx) {
            result = this->vrp->Opt10();
            if (!result)
                result = this->vrp->Opt01();
            if (!result)
                result = this->vrp->Opt11();
            if (!result)
                result = this->vrp->Opt12();
            if (!result)
                result = this->vrp->Opt21();
            if (!result)
                result = this->vrp->Opt22();
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
    }
    this->vrp->RouteBalancer();
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
