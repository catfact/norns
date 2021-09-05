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
        void finishProcess() {
            auto t1 = std::chrono::high_resolution_clock::now();
            usProcess = std::chrono::duration_cast<std::chrono::microseconds>(t1-t0).count();
            t0 = t1;
        }

        Results getResults (double secs) const {
            double rus = 1.0 / (secs * 1000000.0);
            return Results{
                    static_cast<float>(static_cast<double>(usCommands) * rus),
                    static_cast<float>(static_cast<double>(usProcess) * rus)
            };
        }

    };
}

#endif //CRONE_CLIENTLOADMONITOR_H
