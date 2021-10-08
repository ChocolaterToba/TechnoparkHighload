#include <memory>
#include <thread>
#include <mutex>
#include <event2/event.h>

#include "msleep.hpp"
#include "concurrentqueue.hpp"

#include "EventLoop.hpp"
#include "Task.hpp"
#include "TasksController.hpp"
#include "TaskBuilder.hpp"

TaskBuilder::TaskBuilder(moodycamel::ConcurrentQueue<HTTPClient>& unprocessedClients,
                         std::map<int, Task>& haveNoData,
                         EventLoop<TasksController>& haveNoDataEvents,
                         std::shared_ptr<std::mutex> haveNoDataMutex) :
    unprocessedClients(unprocessedClients),
    haveNoData(haveNoData),
    haveNoDataEvents(haveNoDataEvents),
    haveNoDataMutex(haveNoDataMutex),
    stop(true) {}

TaskBuilder::~TaskBuilder() {
    Stop();
}

void TaskBuilder::CreateTasks() {
    while (!stop) {
        Task newTask();
        if (unprocessedClients.try_dequeue(newTask))  {
            haveNoDataMutex->lock();
            haveNoData.emplace(newTask.GetInput().GetSd(), newTask);
            haveNoDataMutex->unlock();
            haveNoDataEvents.AddEvent(newTask.GetInput().GetSd());
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
