#include <memory>
#include <vector>
#include <queue>
#include <map>

#include "msleep.hpp"

#include "socket.hpp"
#include "HTTPClient.hpp"
#include "Task.hpp"
#include "TaskBuilder.hpp"
#include "TasksController.hpp"
#include "Listener.hpp"
#include "Master.hpp"

Master::Master(std::map<std::string, int>& ports, size_t workersAmount):
        ports(ports),
        unprocessedClients(),
        unprocessedClientsMutex(std::make_shared<std::mutex>()),
        haveNoData(),
        haveNoDataMutex(std::make_shared<std::mutex>()),
        builder(unprocessedClients, unprocessedClientsMutex,
                haveNoData, haveNoDataMutex),
        haveData(),
        haveDataMutex(std::make_shared<std::mutex>()),
        controller(haveNoData, haveNoDataMutex, haveData, haveDataMutex),
        pendingDBResponseMutex(std::make_shared<std::mutex>()),
        stop(true) {
            if (!workersAmount) {
                throw std::runtime_error(std::string(
                    "Master constructor: cannot construct master with no workers"
                ));
            }
            for (size_t i = 0; i < workersAmount; ++i) {
                workers.emplace_back(haveData, haveDataMutex,
                                     pendingDBResponse, pendingDBResponseMutex);
            }

            for (auto& keyVal : ports) {
                listeners.emplace_back(keyVal.second, unprocessedClients, unprocessedClientsMutex);
            }
        }

Master::~Master() {
    Stop();
}

void Master::Start() {
    if (stop) {
        stop = false;
        for (Worker& worker : workers) {
            worker.Start();
        }

        controller.Start();
        builder.Start();

        for (Listener& listener : listeners) {
            listener.Start();
            msleep(1);  // listener takes some time to start.
        }
    }
}
void Master::Stop() {  // Processes all existing connections
    if (!stop) {
        stop = true;

        for (Listener& listener : listeners) {
            listener.Stop();
        }

        while (!unprocessedClients.empty()) {
            msleep(120);
        }
        builder.Stop();
        while (!haveNoData.empty()) {
            msleep(120);
        }
        controller.Stop();

        while (!haveData.empty()) {
            msleep(120);
        }
        for (Worker& worker : workers) {
            worker.Stop();
        }
    }
}
