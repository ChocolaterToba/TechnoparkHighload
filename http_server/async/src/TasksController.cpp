#include <mutex>
#include <thread>
#include <memory>
#include <event2/event.h>

#include "msleep.hpp"
#include "ports.hpp"
#include "blockingconcurrentqueue.hpp"

#include "CallbackPackage.hpp"
#include "HTTPClient.hpp"
#include "HttpRequest.hpp"
#include "HttpResponseReader.hpp"
#include "Task.hpp"
#include "TasksController.hpp"

typedef CallbackPackage<TasksController> ControllerPackage;  // rework?

TasksController::TasksController(std::map<int, Task>& haveNoData,
                                 EventLoop<TasksController>& haveNoDataEvents,
                                 std::shared_ptr<std::mutex> haveNoDataMutex,
                                 moodycamel::BlockingConcurrentQueue<Task>& haveData) :
    haveNoData(haveNoData),
    haveNoDataEvents(haveNoDataEvents),
    haveNoDataMutex(haveNoDataMutex),
    haveData(haveData),
    haveDataToken(haveData) {}

TasksController::~TasksController() {
    Stop();
}

void TasksController::Start() {
    haveNoDataEvents.StartLoop();
}

void TasksController::Stop() {
    haveNoDataEvents.StopLoop();
}

void MoveTaskWrapper(evutil_socket_t fd, short events, void* ctx) {
    ControllerPackage* package = static_cast<ControllerPackage*>(ctx);
    if (package->argument->MoveTask(fd, events, package->ev)) {
        delete package;
    }
}

bool TasksController::MoveTask(evutil_socket_t fd, short events, event* ev) {
    if (events & EV_TIMEOUT) {
        haveNoDataEvents.FreeEvent(ev);
        TimeoutTaskRemove(fd);
        return true;
    }

    try {
        if (ReceiveInput(fd)) {
            haveNoDataEvents.FreeEvent(ev);
            MoveTask(fd);
            return true;
        }
    } catch (const std::exception &e) {
        haveNoDataEvents.FreeEvent(ev);
        TimeoutTaskRemove(fd);
        return true;
    }

    return false;
}

bool TasksController::ReceiveInput(int sd) {
    haveNoDataMutex->lock();
    HTTPClient& input = haveNoData.at(sd).GetInput();
    haveNoDataMutex->unlock();

    if (!input.ReceivedHeader()) {
        input.RecvHeaderAsync();
        if (!input.ReceivedHeader()) {
            return false;
        }

        HttpRequest request(input.GetHeader());
        input.SetContentLength(request.GetContentLength());
    }

    return input.RecvBodyAsync();
}

void TasksController::MoveTask(int sd) {
    haveNoDataMutex->lock();
    Task task = std::move(haveNoData.at(sd));
    haveNoData.erase(sd);
    haveNoDataMutex->unlock();

    haveData.enqueue(haveDataToken, std::move(task));
}

void TasksController::TimeoutTaskRemove(int sd) {
    haveNoDataMutex->lock();
    haveNoData.erase(sd);  // automatically calls timeouted client's destructor
    haveNoDataMutex->unlock();
}
