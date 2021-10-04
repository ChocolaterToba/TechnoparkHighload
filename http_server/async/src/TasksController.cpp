#include <queue>
#include <mutex>
#include <thread>
#include <memory>
#include <event2/event.h>

#include "msleep.hpp"
#include "ports.hpp"

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
    ControllerPackage* package = static_cast<ControllerPackage*>(ctx);
    if (events & EV_TIMEOUT) {
        event_free(package->ev);  // also removes event from event loop,
        package->argument->TimeoutTaskRemove(fd);
        delete package;
    } else {
        try {
            if (package->argument->ReceiveInput(fd)) {
                event_free(package->ev);
                package->argument->MoveTask(fd);
                delete package;
            } 
        } catch (const std::exception &e) {
            event_free(package->ev);
            delete package;
            // std::cerr << e.what() << std::endl;
        }
    }
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

    haveDataMutex->lock();
    haveData.push(std::move(task));
    haveDataMutex->unlock();
}

void TasksController::TimeoutTaskRemove(int sd) {
    haveNoDataMutex->lock();
    haveNoData.erase(sd);  // automatically calls timeouted client's destructor
    haveNoDataMutex->unlock();
}
