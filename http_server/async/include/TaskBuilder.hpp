#pragma once

#include <memory>
#include <thread>
#include <map>
#include <mutex>
#include <event2/event.h>

#include "blockingconcurrentqueue.hpp"

#include "EventLoop.hpp"
#include "Task.hpp"
#include "TasksController.hpp"

class TaskBuilder {
 protected:
    moodycamel::BlockingConcurrentQueue<HTTPClient>& unprocessedClients;
    moodycamel::ConsumerToken unprocessedClientsToken;

    std::map<int, Task>& haveNoData;
    EventLoop<TasksController>& haveNoDataEvents;
    std::shared_ptr<std::mutex> haveNoDataMutex;

    virtual void CreateTasks();

    std::thread builderThread;
    bool stop;

 public:
    TaskBuilder(moodycamel::BlockingConcurrentQueue<HTTPClient>& unprocessedClients,
                std::map<int, Task>& haveNoData,
                EventLoop<TasksController>& haveNoDataEvents,
                std::shared_ptr<std::mutex> haveNoDataMutex);
    virtual ~TaskBuilder();

    virtual void Start();
    virtual void Stop();
};
