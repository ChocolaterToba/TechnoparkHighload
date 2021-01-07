#include <queue>
#include <mutex>
#include <thread>
#include <memory>
#include <event2/event.h>

#include "msleep.hpp"

#include "Task.hpp"
#include "TasksController.hpp"

TasksController::TasksController(std::map<int, Task>& haveNoData,
                                 std::shared_ptr<struct event_base> haveNoDataEvents,
                                 std::shared_ptr<std::mutex> haveNoDataMutex,
                                 std::queue<Task>& haveData,
                                 std::shared_ptr<std::mutex> haveDataMutex) :
    haveNoData(haveNoData),
    haveNoDataEvents(haveNoDataEvents),
    haveNoDataMutex(haveNoDataMutex),
    haveData(haveData),
    haveDataMutex(haveDataMutex),
    stop(true) {}

TasksController::~TasksController() {
    Stop();
}


void TasksController::Loop() {
    while (!stop) {
        event_base_loop(haveNoDataEvents.get(), EVLOOP_ONCE);
    }
}

void TasksController::Start() {
    if (stop) {
        stop = false;
        tasksControllerThread = std::thread(&TasksController::Loop, this);
    }
}

void TasksController::Stop() {
    if (!stop) {
        stop = true;
        tasksControllerThread.join();
    }
}

void TasksController::MoveTaskWrapper(evutil_socket_t fd, short events, void* ctx) {
    if (events & EV_TIMEOUT) {
        (static_cast<TasksController*>(ctx))->TimeoutTaskRemove(fd);
    } else {
        (static_cast<TasksController*>(ctx))->MoveTask(fd);
    }
}

void TasksController::MoveTask(int sd) {
    haveNoDataMutex->lock();
    Task task = std::move(haveNoData.at(sd));
    haveNoData.erase(sd);
    haveNoDataMutex->unlock();

    haveDataMutex->lock();
    haveData.push(std::move(task));
    haveDataMutex->unlock();
}

void TasksController::TimeoutTaskRemove(int sd) {
    haveNoDataMutex->lock();
    haveNoData.erase(sd);  // automatically calls timeouted client's destructor
    haveNoDataMutex->unlock();
}
