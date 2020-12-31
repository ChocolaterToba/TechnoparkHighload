#include <queue>
#include <mutex>
#include <thread>
#include <memory>

#include "msleep.hpp"

#include "Task.hpp"
#include "TasksController.hpp"

TasksController::TasksController(std::vector<Task>& haveNoData,
                                 std::shared_ptr<std::mutex> haveNoDataMutex,
                                 std::queue<Task>& haveData,
                                 std::shared_ptr<std::mutex> haveDataMutex) :
    haveNoData(haveNoData),
    haveNoDataMutex(haveNoDataMutex),
    haveData(haveData),
    haveDataMutex(haveDataMutex),
    stop(true) {}

TasksController::~TasksController() {
    Stop();
}


void TasksController::Loop() {
    while (!stop) {
        if (!haveNoData.empty()) {
            std::queue<Task> buffer;
            int i = haveNoData.size() - 1;
            haveNoDataMutex->lock();
            while (i >= 0) {
                if (haveNoData[i].HasData()) {
                    buffer.push(std::move(haveNoData[i]));
                    haveNoData.erase(haveNoData.begin() + i);
                }
                --i;
            }
            haveNoDataMutex->unlock();

            haveDataMutex->lock();
            while (!buffer.empty()) {
                haveData.push(std::move(buffer.front()));
                buffer.pop();
            }
            haveDataMutex->unlock();
        } else {
            msleep(100);
        }

        msleep(100);
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
