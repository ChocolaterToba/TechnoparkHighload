#pragma once

#include <memory>
#include <thread>
#include <map>
#include <mutex>
#include <event2/event.h>

#include "blockingconcurrentqueue.hpp"

#include "EventLoop.hpp"
#include "Task.hpp"

class TasksController {
 protected:
    std::map<int, Task>& haveNoData;
    EventLoop<TasksController>& haveNoDataEvents;
    std::shared_ptr<std::mutex> haveNoDataMutex;

    moodycamel::BlockingConcurrentQueue<Task>& haveData;

    bool ReceiveInput(int sd);
    void MoveTask(int sd);
    void TimeoutTaskRemove(int sd);

 public:
    TasksController(std::map<int, Task>& haveNoData,
                    EventLoop<TasksController>& haveNoDataEvents,
                    std::shared_ptr<std::mutex> haveNoDataMutex,
                    moodycamel::BlockingConcurrentQueue<Task>& haveData);

    ~TasksController();

    void Start();
    void Stop();

    bool MoveTask(evutil_socket_t fd, short events, event* ev);  // returns true if event is freed
};

void MoveTaskWrapper(evutil_socket_t fd, short events, void* ctx);
