#ifndef Utils_H
#define Utils_H

#include "rapidjson/filereadstream.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/document.h"     // rapidjson's DOM-style API
#include "rapidjson/prettywriter.h" // for stringify JSON PrettyWriter
#include "rapidjson/error/en.h"
#include "VRP.h"

class VRP;
class Utils {
private:
    Utils() {}
    Utils(Utils const&) = delete;
    void operator=(Utils const&) = delete;
    rapidjson::Document d;                      /**< JSON Document */
    const char* ANSI_RESET = "\u001B[0m";
    const char* ANSI_RED = "\u001B[1;31m";
    const char* ANSI_GREEN = "\u001B[1;32m";
    const char* ANSI_YELLOW = "\u001B[33m";
    const char* ANSI_BLUE = "\u001B[1;34m";
    const char* ANSI_LIGHTGREEN = "\u001B[32m";
    const char* ANSI_IBLUE = "\e[0;94m";
    public:
    static Utils& Instance() {
        static Utils instance;
        return instance;
    }
    static const int SUCCESS = 0;               /**< Success code */
    static const int ERROR = 1;                 /**< Error code */
    static const int WARNING = 2;               /**< Warning code */
    static const int INFO = 3;                  /**< Simple logging code */
    static const int VERBOSE = 4;               /**< Verbose code */
    bool verbose = false;
    VRP* InitParameters (char **);
    void SaveResult(std::list<Route>);

    /** @brief Print a log string
     *
     * @param s The string to print
     * @param c The code for log level
     */
    template <typename T>
    void logger(T s, int c = 5) const {
        switch(c) {
            case SUCCESS:
                std::cout << ANSI_LIGHTGREEN << s << ANSI_RESET << std::endl;
            break;
            case WARNING:
                std::cout << ANSI_YELLOW << "[w] " << s << ANSI_RESET << std::endl;
            break;
            case ERROR:
                std::cout << ANSI_RED << s << ANSI_RESET << std::endl;
            break;
            case INFO:
                std::cout << ANSI_BLUE << s << ANSI_RESET << std::endl;
                break;
            case VERBOSE:
                if (this->verbose)
                    std::cout << ANSI_IBLUE << "[-]\t" << s << ANSI_RESET << std::endl;
                break;
            default:
                std::cout << s << std::endl;
            break;
        }
    }
};

#endif /* Utils_H */