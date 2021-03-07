#pragma once

#include <memory>
#include <thread>
#include <queue>
#include <map>
#include <mutex>
#include <event2/event.h>

#include "EventLoop.hpp"
#include "Task.hpp"
#include "TasksController.hpp"

class TaskBuilder {
 protected:
    std::queue<HTTPClient>& unprocessedClients;
    std::shared_ptr<std::mutex> unprocessedClientsMutex;

    std::map<int, Task>& haveNoData;
    EventLoop<TasksController>& haveNoDataEvents;
    std::shared_ptr<std::mutex> haveNoDataMutex;

    virtual void CreateTasks();

    std::thread builderThread;
    bool stop;

 public:
    TaskBuilder(std::queue<HTTPClient>& unprocessedClients,
                std::shared_ptr<std::mutex> unprocessedClientsMutex,
                std::map<int, Task>& haveNoData,
                EventLoop<TasksController>& haveNoDataEvents,
                std::shared_ptr<std::mutex> haveNoDataMutex);
    virtual ~TaskBuilder();

    virtual void Start();
    virtual void Stop();
};
