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
 * @param[in] argc The number of arguments passed through command line.
 * @param[in] argv The arguments passed through command line.
 * @param[in] costTravel The cost of travelling.
 * @param[in] alphaParam Alpha parameter for route evaluation.
 * @param[in] max_time Maximum execution time in minutes.
 */
void Controller::Init(int argc, char **argv, float costTravel, float alphaParam, int max_time) {
    this->MAX_TIME_MIN = max_time;
    Utils &u = this->GetUtils();
    u.logger("Initializing...", u.INFO);
    this->vrp = Utils::Instance().InitParameters(argc, argv, costTravel, alphaParam);
    int res = this->vrp->InitSolutionsNeigh();
    switch (res) {
        case -1:
            u.logger("You need less vehicles.", u.WARNING);
        break;
        case 0:
            u.logger("Done!", u.SUCCESS);
        break;
        case 1:
            throw std::runtime_error("You need more vehicles");
        break;
    }
    this->initCost = this->vrp->GetTotalCost();
}

/** @brief ###Runs all the main functions
 *
 * This function sets and call the tabu search and optimal functions.
 * If the routines do not improves the solutions set stop.
 */
void Controller::RunVRP() {
    int customers = this->vrp->GetNumberOfCustomers();
	int timeOpts = customers, iteration = customers;
    // number of opt functions executions
	if (customers > 60){
		timeOpts = customers / 4;
    }else
		timeOpts *= 0.50;
    int noimprov = 0, duration = 0, last = 0, prelast = 0;
    // start time
    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
    // run the routines for 'customers' times or stop if no improvement or duration is more than MAX_TIME
    for (int i = 0; i < iteration && noimprov < 5 && duration <= this->MAX_TIME_MIN; i++) {
        bool opt, optflag = false;
        int ts = this->RunTabuSearch(10);
        if (ts < 0)
            optflag = true;
        if (ts == -last || ts == prelast || ts == 0) {
            optflag = true;
            noimprov++;
        }else {
            prelast = last;
            last = ts;
        }
        Utils::Instance().logger("Starting opt", Utils::VERBOSE);
        opt = this->vrp->RunOpts(timeOpts, optflag);
        optflag = false;
		this->vrp->UpdateBest();
        if (ts == 0 && !opt)
            noimprov++;
        else
            noimprov = 0;
        // partial time
        std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::minutes>(t2 - t1).count();
		Utils::Instance().logger("PARTIAL: " + std::to_string(this->vrp->GetTotalCost()) +
				" " +std::to_string(i+1) + "/" + std::to_string(iteration), Utils::INFO);
    }
    this->finalCost = this->vrp->GetTotalCost();
    int percCost = (float)(this->finalCost - this->initCost) / this->initCost * 100;
    Utils::Instance().logger("Total improvement: " + std::to_string(this->initCost - this->finalCost) + " "
        + std::to_string(percCost) + "%", Utils::INFO);
}
/** @brief ###Runs the tabu search function.
 *
 * @param[in] times Number of iteration.
 * @return          If the routine made some improvements.
 */
int Controller::RunTabuSearch(int times) {
    int initCost = this->vrp->GetTotalCost();
    Utils::Instance().logger("Starting Tabu Search", Utils::VERBOSE);
    this->vrp->RunTabuSearch(times);
    int diffCost = initCost - this->vrp->GetTotalCost();
    if (diffCost != 0) {
        Utils::Instance().logger("Tabu Search improved: " + std::to_string(diffCost), Utils::VERBOSE);
        return diffCost;
    }else {
        Utils::Instance().logger("Tabu Search no improvement", Utils::VERBOSE);
        return 0;
    }
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

/** @brief ###Print the best solution.
 *
 * Prints all routes with costs and in a more readable way.
 */
void Controller::PrintBestRoutes() {
    Utils &u = this->GetUtils();
    std::list<Route> *e = this->vrp->GetBestRoutes();
	int totCost = 0;
	std::cout << std::endl;
    for (auto i = e->cbegin(); i != e->cend(); i++) {
        u.logger(*i);
		totCost += i->GetTotalCost();
        std::advance(i, 1);
        if (i == e->cend()) break;
        u.logger(*i, u.SUCCESS);
		totCost += i->GetTotalCost();
    }
    u.logger("Total cost: " + std::to_string(totCost), u.INFO);
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
    std::list<Route> *e = this->vrp->GetBestRoutes();
    u.SaveResult(*e);
}
