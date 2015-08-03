#ifndef Utils_H
#define Utils_H

#include <iostream>
#include <regex>
#include <vector>
#include <cstdlib>
#include "rapidjson/filereadstream.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/document.h"     // rapidjson's DOM-style API
#include "rapidjson/prettywriter.h" // for stringify JSON PrettyWriter
#include "rapidjson/error/en.h"
#include "VRP.h"

class Utils {
private:
    Utils() {}
    Utils(Utils const&) = delete;
    void operator=(Utils const&) = delete;
    rapidjson::Document d;
    const char* ANSI_RESET = "\u001B[0m";
    const char* ANSI_RED = "\u001B[31m";
    const char* ANSI_GREEN = "\u001B[32m";
    const char* ANSI_YELLOW = "\u001B[33m";
public:
    static Utils& Instance() {
        static Utils instance;
        return instance;
    }
    static const int ERROR = 1;
    static const int WARNING = 2;
    static const int SUCCESS = 0;
    VRP* InitParameters (char **);
    FILE* SaveResult(void);
    template <typename T>
    void logger(T s, int c) const {
        switch(c) {
            case SUCCESS:
                std::cout << ANSI_GREEN << s << ANSI_RESET << std::endl;
            break;
            case WARNING:
                std::cout << ANSI_YELLOW << s << ANSI_RESET << std::endl;
            break;
            case ERROR:
                std::cout << ANSI_RED << s << ANSI_RESET << std::endl;
            break;
        }
    }
};

#endif /* Utils_H */