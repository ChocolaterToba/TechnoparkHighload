#include <memory>
#include <thread>
#include <queue>
#include <mutex>
#include <event2/event.h>

#include "msleep.hpp"

#include "Task.hpp"
#include "TasksController.hpp"
#include "TaskBuilder.hpp"

TaskBuilder::TaskBuilder(std::queue<HTTPClient>& unprocessedClients,
                         std::shared_ptr<std::mutex> unprocessedClientsMutex,
                         std::map<int, Task>& haveNoData,
                         std::shared_ptr<struct event_base> haveNoDataEvents,
                         std::shared_ptr<std::mutex> haveNoDataMutex,
                         TasksController& tasksController) :
    unprocessedClients(unprocessedClients),
    unprocessedClientsMutex(unprocessedClientsMutex),
    haveNoData(haveNoData),
    haveNoDataEvents(haveNoDataEvents),
    haveNoDataMutex(haveNoDataMutex),
    tasksController(tasksController),
    stop(true) {}

TaskBuilder::~TaskBuilder() {
    Stop();
}

void TaskBuilder::CreateTasks() {
    while (!stop) {
        if (!unprocessedClients.empty())  {
            //  TaskBuilder is singular, so there's no "queue shrinking" problem.
            unprocessedClientsMutex->lock();
            Task newTask(unprocessedClients.front());
            unprocessedClients.pop();
            unprocessedClientsMutex->unlock();

            haveNoDataMutex->lock();
            haveNoData.emplace(newTask.GetInput().getSd(), newTask);
            event_base_once(haveNoDataEvents.get(), newTask.GetInput().getSd(), EV_READ,
                            &TasksController::MoveTaskWrapper, &tasksController, nullptr);
            haveNoDataMutex->unlock();
        } else {
            msleep(30);
        }
    }
}

void TaskBuilder::Start() {
    if (stop) {
        stop = false;
        builderThread = std::thread(&TaskBuilder::CreateTasks, this);
    }
}

void TaskBuilder::Stop() {
    if (!stop) {
        stop = true;
        builderThread.join();
    }
}
