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

void Controller::Init(char **argv) {
    Utils &u = this->GetUtils();
    u.logger("Initializing...", u.INFO);
    this->vrp = Utils::Instance().InitParameters(argv);
    int res = this->vrp->InitSolutions();
    switch (res) {
        case -1:
            u.logger("You need less vehicles.", u.WARNING);
        break;
        case 0:
            u.logger("Done!", u.SUCCESS);
        break;
        case 1:
            throw std::string("You need more vehicles");
        break;
    }
    this->PrintRoutes();
    u.logger("Starting opt", u.VERBOSE);
    this->RunOpts(10);
    this->PrintRoutes();
    this->SaveResult();
}

Utils& Controller::GetUtils() const {
    return Utils::Instance();
}

void Controller::PrintRoutes() {
    Utils &u = this->GetUtils();
    std::list<Route> *e = this->vrp->GetRoutes();
    std::cout << std::endl;
    for (auto i = e->begin(); i != e->cend(); i++) {
        u.logger(*i);
        std::advance(i, 1);
        if (i == e->cend()) break;
        u.logger(*i, u.SUCCESS);
    }
    u.logger("Total cost: " + std::to_string(this->vrp->GetTotalCost()), u.INFO);
    std::cout << std::endl;
}

void Controller::RunOpts(int times) {
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
        this->vrp->Opt2();
        this->vrp->Opt3();
        i++;
    }
    this->vrp->RouteBalancer();
}

void Controller::SaveResult() {
    Utils &u = this->GetUtils();
    u.logger("Saving to output.json", u.VERBOSE);
    std::list<Route> *e = this->vrp->GetRoutes();
    u.SaveResult(*e);
}