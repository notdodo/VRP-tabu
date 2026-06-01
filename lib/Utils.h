#ifndef Utils_H
#define Utils_H

#include "VRP.h"
#include "Route.h"
#include <list>
#include <nlohmann/json.hpp>
#include <string>

class VRP;

/** @brief Utility singleton for input parsing, output writing, and logging.
 *
 * Utils owns the JSON document used during initialization and provides process-wide
 * helpers for reading instances, saving route results, and emitting colored
 * console messages.
 */
class Utils {
  public:
    Utils(Utils const&) = delete;
    Utils& operator=(Utils const&) = delete;

  private:
    /** @brief Hide construction behind the singleton accessor. */
    Utils() = default;
    nlohmann::json d; /**< Parsed input and output JSON document */
    const char* ANSI_RESET = "\u001B[0m";
    const char* ANSI_RED = "\u001B[1;31m";
    const char* ANSI_YELLOW = "\u001B[33m";
    const char* ANSI_BLUE = "\u001B[1;34m";
    const char* ANSI_LIGHTGREEN = "\u001B[32m";
    const char* ANSI_IBLUE = "\x1b[0;94m";

  public:
    /** @brief Return the shared utility instance. */
    static Utils& Instance() {
        static Utils instance;
        return instance;
    }
    static const int SUCCESS = 0; /**< Success code */
    static const int ERROR = 1;   /**< Error code */
    static const int WARNING = 2; /**< Warning code */
    static const int INFO = 3;    /**< Simple logging code */
    static const int VERBOSE = 4; /**< Verbose code */
    bool verbose = false;
    std::string filename = "";

    /** @brief Parse CLI arguments and JSON input into a VRP instance. */
    VRP* InitParameters(int, char**, const float, const float);

    /** @brief Save the supplied routes as the result for a run timestamp/index. */
    void SaveResult(const Routes&, long long);

    /** @brief Print a log string
     *
     * @param[in] s The string to print
     * @param[in] c The code for log level
     */
    template <typename T> void logger(const T& s, int c = 5) const {
        switch (c) {
        case SUCCESS:
            std::cout << ANSI_LIGHTGREEN << s << ANSI_RESET << std::endl;
            break;
        case WARNING:
            std::cout << ANSI_YELLOW << "[w]\t" << s << ANSI_RESET << std::endl;
            break;
        case ERROR:
            std::cout << ANSI_RED << "\r\n" << s << ANSI_RESET << std::endl;
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
