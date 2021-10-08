#pragma once

#include <memory>
#include <vector>
#include <map>

#include "concurrentqueue.hpp"

#include "socket.hpp"
#include "HTTPClient.hpp"
#include "EventLoop.hpp"
#include "TaskBuilder.hpp"
#include "TasksController.hpp"
#include "Listener.hpp"
#include "Worker.hpp"

class Master {
 protected:
    Socket socket;

    std::vector<Worker> workers;

    std::map<std::string, int> ports;
    std::vector<Listener> listeners;

    moodycamel::ConcurrentQueue<HTTPClient> unprocessedClients;

    std::map<int, Task> haveNoData;
    EventLoop<TasksController> haveNoDataEvents;
    std::shared_ptr<std::mutex> haveNoDataMutex;

    TaskBuilder builder;

    moodycamel::ConcurrentQueue<Task> haveData;

    TasksController controller;

    bool stop;

 public:
    explicit Master(std::map<std::string, int>& ports, size_t workersAmount = 1);
    virtual ~Master();

    //  static void SetSocket(std::unique_ptr<Socket> socket);
    virtual void Start();
    virtual void Stop();  // Processes all existing connections
};
