//
// Created by emb on 9/4/21.
//

#ifndef CRONE_CLIENTLOADMONITOR_H
#define CRONE_CLIENTLOADMONITOR_H

#include <chrono>

namespace crone {
    class ClientLoadMonitor {
    private:
        struct Results {
            float ratioCommands;
            float ratioProcess;
        };
        Results results;
        std::chrono::time_point<std::chrono::high_resolution_clock> t0;
        long long usCommands;
        long long usProcess;
    public:
        void startBlock() {
            t0 = std::chrono::high_resolution_clock::now();
        }

        void finishCommands() {
            auto t1 = std::chrono::high_resolution_clock::now();
            usCommands = std::chrono::duration_cast<std::chrono::microseconds>(t1-t0).count();
            t0 = t1;
        }

        void finishProcess(double seconds) {
            auto t1 = std::chrono::high_resolution_clock::now();
            usProcess = std::chrono::duration_cast<std::chrono::microseconds>(t1-t0).count();
            t0 = t1;
            // reciprocal of microseconds
            double rus = 1.0 / (seconds * 1000000.0);
            results.ratioCommands = static_cast<float>(static_cast<double>(usCommands) * rus);
            results.ratioProcess = static_cast<float>(static_cast<double>(usProcess) * rus);
        }

        const Results &getResults () const {
            return results;
        }

    };
}

#endif //CRONE_CLIENTLOADMONITOR_H
