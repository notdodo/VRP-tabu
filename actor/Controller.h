#ifndef Controller_H
#define Controller_H

#include "VRP.h"
#include <chrono>

/** @brief Application-level coordinator for loading, solving, and reporting a VRP instance.
 *
 * Controller is a singleton wrapper around one VRP run. It owns timing,
 * initialization/final cost bookkeeping, and the final result persistence.
 */
class Controller {
  public:
    Controller(Controller const&) = delete;
    Controller& operator=(Controller const&) = delete;

  private:
    VRP* vrp = nullptr;

    /** @brief Hide construction behind the singleton accessor. */
    Controller() = default;

    /** @brief Run tabu search phases until the configured time budget expires. */
    int RunTabuSearch(int);

    int MAX_TIME_MIN = 0;
    int initCost = 0;
    int finalCost = 0;
    std::chrono::high_resolution_clock::time_point startTime;

  public:
    /** @brief Return the singleton controller instance. */
    static Controller& Instance() {
        static Controller instance;
        return instance;
    }

    /** @brief Parse command-line input and create the VRP model. */
    void Init(int, char** argv, float, float, int);

    /** @brief Execute the full VRP workflow from initial solution to local search. */
    void RunVRP();

    /** @brief Persist the best solution found during the run. */
    void SaveResult();

    /** @brief Print the current route set. */
    void PrintRoutes();

    /** @brief Print the best route set found so far. */
    void PrintBestRoutes();

    /** @brief Return the shared utility singleton. */
    [[nodiscard]] Utils& GetUtils() const;
};

#endif /* Controller_H */
