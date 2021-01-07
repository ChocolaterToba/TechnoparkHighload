#pragma once

#include <queue>
#include <map>
#include <mutex>
#include <thread>
#include <memory>
#include <event2/event.h>

#include "Task.hpp"

class TasksController {
 protected:
    std::map<int, Task>& haveNoData;
    std::shared_ptr<struct event_base> haveNoDataEvents;
    std::shared_ptr<std::mutex> haveNoDataMutex;

    std::queue<Task>& haveData;
    std::shared_ptr<std::mutex> haveDataMutex;

    std::thread tasksControllerThread;
    virtual void Loop();
    bool stop;

    void MoveTask(int sd);

 public:
    TasksController(std::map<int, Task>& haveNoData,
                    std::shared_ptr<struct event_base> haveNoDataEvents,
                    std::shared_ptr<std::mutex> haveNoDataMutex,
                    std::queue<Task>& haveData,
                    std::shared_ptr<std::mutex> haveDataMutex);

    ~TasksController();

    void Start();
    void Stop();

    static void MoveTaskWrapper(evutil_socket_t fd, short events, void* ctx);
};
