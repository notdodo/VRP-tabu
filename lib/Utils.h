#ifndef Utils_H
#define Utils_H

#include "rapidjson/filereadstream.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/document.h"     // rapidjson's DOM-style API
#include "rapidjson/prettywriter.h" // for stringify JSON PrettyWriter
#include "rapidjson/error/en.h"
#include <iostream>
#include <regex>
#include <vector>
#include "../actor/Customer.h"
#include "VRP.h"

class Utils {
private:
    rapidjson::Document d;
public:
    bool ParseInt (std::string a);
    VRP* InitParameters (char **argv);
    FILE* SaveResult(void);
};

#endif /* Utils_H */