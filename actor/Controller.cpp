#include "Controller.h"

void Controller::Init(char **argv) {
    Utils &u = this->GetUtils();
    u.logger("Initializing...", u.WARNING);
    this->v = Utils::Instance().InitParameters(argv);
    int res = this->v->InitSolutions();
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
    int i = 0;
    while(i < 100) {
        this->RunOpts(10);
        i++;
    }
    this->PrintRoutes();
}

Utils& Controller::GetUtils() const {
    return Utils::Instance();
}

void Controller::PrintRoutes() {
    Utils &u = this->GetUtils();
    std::list<Route> *e = this->v->GetRoutes();
    std::cout << std::endl;
    for (auto i = e->begin(); i != e->cend(); i++) {
        u.logger(*i);
        std::advance(i, 1);
        if (i == e->cend()) break;
        u.logger(*i, u.SUCCESS);
    }
    std::cout << std::endl;
}

void Controller::RunOpts(int time) {
    this->v->Opt10(time);
    this->v->Opt11();
    this->v->Opt12();
    this->v->Opt22();
    this->v->Opt21();
}