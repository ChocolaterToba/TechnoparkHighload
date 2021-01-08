#include <queue>
#include <mutex>
#include <thread>
#include <memory>
#include <event2/event.h>

#include "msleep.hpp"

#include "Task.hpp"
#include "TasksController.hpp"

TasksController::TasksController(std::map<int, Task>& haveNoData,
                                 EventLoop<TasksController>& haveNoDataEvents,
                                 std::shared_ptr<std::mutex> haveNoDataMutex,
                                 std::queue<Task>& haveData,
                                 std::shared_ptr<std::mutex> haveDataMutex) :
    haveNoData(haveNoData),
    haveNoDataEvents(haveNoDataEvents),
    haveNoDataMutex(haveNoDataMutex),
    haveData(haveData),
    haveDataMutex(haveDataMutex) {}

TasksController::~TasksController() {
    Stop();
}

void TasksController::Start() {
    haveNoDataEvents.StartLoop();
}

void TasksController::Stop() {
    haveNoDataEvents.StopLoop();
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
