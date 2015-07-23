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
#include <cstdlib>
#include "../actor/Customer.h"
#include "VRP.h"

#define ANSI_RESET "\u001B[0m"
#define ANSI_RED "\u001B[31m"
#define ANSI_GREEN "\u001B[32m"
#define ANSI_YELLOW "\u001B[33m"

class Utils {
private:
    Utils() {}
    Utils(Utils const&) = delete;
    void operator=(Utils const&) = delete;
    rapidjson::Document d;
public:
    static Utils& Instance() {
        static Utils instance;
        return instance;
    }
    static const int ERROR = 1;
    static const int WARNING = 2;
    static const int SUCCESS = 0;
    bool ParseInt (std::string a);
    VRP InitParameters (char **argv);
    FILE* SaveResult(void);
    void logger(std::string s, int code);
};

#endif /* Utils_H */