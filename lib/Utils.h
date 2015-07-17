#ifndef Utils_H
#define Utils_H

#include "rapidjson/filereadstream.h"
#include "rapidjson/document.h"     // rapidjson's DOM-style API
#include "rapidjson/prettywriter.h" // for stringify JSON
#include <iostream>
#include <regex>

class Utils {
private:
    /* data */
public:
    Utils();
    bool ParseInt (std::string a);
    bool ParseInput (char **argv);
};

#endif /* Utils_H */