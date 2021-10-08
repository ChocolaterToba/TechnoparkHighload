#include <memory>
#include <thread>
#include <mutex>
#include <event2/event.h>

#include "msleep.hpp"
#include "blockingconcurrentqueue.hpp"

#include "EventLoop.hpp"
#include "Task.hpp"
#include "TasksController.hpp"
#include "TaskBuilder.hpp"

TaskBuilder::TaskBuilder(moodycamel::BlockingConcurrentQueue<HTTPClient>& unprocessedClients,
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
        HTTPClient newClient;
        if (unprocessedClients.wait_dequeue_timed(newClient, std::chrono::milliseconds(100)))  {  // timer only really matters when stopping builder
            haveNoDataMutex->lock();
            haveNoData.emplace(newClient.GetSd(), Task(newClient));
            haveNoDataMutex->unlock();
            haveNoDataEvents.AddEvent(newClient.GetSd());
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
