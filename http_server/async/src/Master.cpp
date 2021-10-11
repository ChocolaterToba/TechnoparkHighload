#include <memory>
#include <vector>
#include <map>
#include <iostream>

#include "msleep.hpp"
#include "blockingconcurrentqueue.hpp"

#include "socket.hpp"
#include "HTTPClient.hpp"
#include "EventLoop.hpp"
#include "Task.hpp"
#include "TaskBuilder.hpp"
#include "TasksController.hpp"
#include "Listener.hpp"
#include "Master.hpp"

const size_t EXPECTED_MAX_HANGING_TASKS = 10000;

Master::Master(std::map<std::string, int>& ports, size_t workersAmount):
        ports(ports),

        unprocessedClients(),

        haveNoData(),
        haveNoDataEvents(&MoveTaskWrapper),
        haveNoDataMutex(std::make_shared<std::mutex>()),

        builder(unprocessedClients, haveNoData, haveNoDataEvents, haveNoDataMutex),

        haveData(EXPECTED_MAX_HANGING_TASKS, 1, 0),  // Pre-allocationg 10000 elements in queue

        controller(haveNoData, haveNoDataEvents, haveNoDataMutex, haveData),

        stop(true) {
            if (ports.find("external") == ports.end()) {
                throw std::runtime_error(std::string(
                    "Master constructor: no \"external\" port given"
                ));
            }
            for (auto& keyVal : ports) {
                listeners.emplace_back(keyVal.second, unprocessedClients);
            }

            haveNoDataEvents.SetCallbackArgument(std::shared_ptr<TasksController>(&controller));

            if (!workersAmount) {
                throw std::runtime_error(std::string(
                    "Master constructor: cannot construct master with no workers"
                ));
            }

            std::cout << "Creating " << workersAmount << " workers" << std::endl;
            for (size_t i = 0; i < workersAmount; ++i) {
                workers.emplace_back(haveData);
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
            listener.Stop();  // TODO: stop external listener first
        }

        while (unprocessedClients.size_approx() != 0) {
            msleep(120);
        }
        builder.Stop();

        while (!haveNoData.empty()) {
            msleep(120);
        }
        controller.Stop();

        while (unprocessedClients.size_approx() != 0) {
            msleep(120);
        }
        for (Worker& worker : workers) {
            worker.Stop();
        }
    }
}
