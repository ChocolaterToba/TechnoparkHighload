#pragma once

#include <memory>
#include <thread>
#include <queue>
#include <map>
#include <mutex>
#include <event2/event.h>

#include "Task.hpp"
#include "TasksController.hpp"

class TaskBuilder {
 protected:
    std::queue<HTTPClient>& unprocessedClients;
    std::shared_ptr<std::mutex> unprocessedClientsMutex;

    std::map<int, Task>& haveNoData;
    std::shared_ptr<struct event_base> haveNoDataEvents;
    std::shared_ptr<std::mutex> haveNoDataMutex;

    TasksController& tasksController;

    virtual void CreateTasks();

    std::thread builderThread;
    bool stop;

 public:
    TaskBuilder(std::queue<HTTPClient>& unprocessedClients,
                std::shared_ptr<std::mutex> unprocessedClientsMutex,
                std::map<int, Task>& haveNoData,
                std::shared_ptr<struct event_base> haveNoDataEvents,
                std::shared_ptr<std::mutex> haveNoDataMutex,
                TasksController& tasksController);
    virtual ~TaskBuilder();

    virtual void Start();
    virtual void Stop();
};
