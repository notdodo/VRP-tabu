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
    this->PrintRoutes();
    this->RunOpts(5);
    this->PrintRoutes();
    this->SaveResult();
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
    u.logger("Total cost: " + std::to_string(this->v->GetTotalCost()), u.INFO);
    std::cout << std::endl;
}

// devo fare opt-2 e opt-3

// appena migliora ricomincio dall'inizio
// devo filtrare i customer da scambiare
// distanza per ogni coppia di route
// la media e scambio solo route con distanza <= media

// i 2 devono essere consecutivi e provo tutte le combinazioni
void Controller::RunOpts(int times) {
    int i = 0;
    bool result;
    while (i < times) {
        result = this->v->Opt10();
        if (!result)
            result = this->v->Opt01();
        if (!result)
            result = this->v->Opt11();
        if (!result)
            result = this->v->Opt12();
        if (!result)
            result = this->v->Opt21();
        if (!result)
            result = this->v->Opt22();
        i++;
    }
}

void Controller::SaveResult() {
    Utils &u = this->GetUtils();
    std::list<Route> *e = this->v->GetRoutes();
    u.SaveResult(*e);
}