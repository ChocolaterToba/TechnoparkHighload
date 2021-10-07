#pragma once

#include <queue>
#include <map>
#include <mutex>
#include <thread>
#include <memory>
#include <event2/event.h>

#include "EventLoop.hpp"
#include "Task.hpp"

class TasksController {
 protected:
    std::map<int, Task>& haveNoData;
    EventLoop<TasksController>& haveNoDataEvents;
    std::shared_ptr<std::mutex> haveNoDataMutex;

    std::queue<Task>& haveData;
    std::shared_ptr<std::mutex> haveDataMutex;

    bool ReceiveInput(int sd);
    void MoveTask(int sd);
    void TimeoutTaskRemove(int sd);

 public:
    TasksController(std::map<int, Task>& haveNoData,
                    EventLoop<TasksController>& haveNoDataEvents,
                    std::shared_ptr<std::mutex> haveNoDataMutex,
                    std::queue<Task>& haveData,
                    std::shared_ptr<std::mutex> haveDataMutex);

    ~TasksController();

    void Start();
    void Stop();

    void MoveTask(evutil_socket_t fd, short events, event* ev);
};

void MoveTaskWrapper(evutil_socket_t fd, short events, void* ctx);
