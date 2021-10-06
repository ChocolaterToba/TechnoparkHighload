#include <memory>
#include <thread>
#include <queue>
#include <mutex>
#include <event2/event.h>

#include "msleep.hpp"

#include "EventLoop.hpp"
#include "Task.hpp"
#include "TasksController.hpp"
#include "TaskBuilder.hpp"

TaskBuilder::TaskBuilder(std::queue<HTTPClient>& unprocessedClients,
                         std::shared_ptr<std::mutex> unprocessedClientsMutex,
                         std::map<int, Task>& haveNoData,
                         EventLoop<TasksController>& haveNoDataEvents,
                         std::shared_ptr<std::mutex> haveNoDataMutex) :
    unprocessedClients(unprocessedClients),
    unprocessedClientsMutex(unprocessedClientsMutex),
    haveNoData(haveNoData),
    haveNoDataEvents(haveNoDataEvents),
    haveNoDataMutex(haveNoDataMutex),
    stop(true) {}

TaskBuilder::~TaskBuilder() {
    Stop();
}

void TaskBuilder::CreateTasks() {
    while (!stop) {
        if (!unprocessedClients.empty())  {
            //  TaskBuilder is singular, so there's no "queue shrinking" problem. - fix, it's not necessarily true
            unprocessedClientsMutex->lock();
            if (!unprocessedClients.empty())  {
                Task newTask(unprocessedClients.front());
                unprocessedClients.pop();
                unprocessedClientsMutex->unlock();

                haveNoDataMutex->lock();
                haveNoData.emplace(newTask.GetInput().GetSd(), newTask);
                haveNoDataEvents.AddEvent(newTask.GetInput().GetSd());
                haveNoDataMutex->unlock();
            } else {
                haveNoDataMutex->unlock();
                sleep(30);
            }
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
