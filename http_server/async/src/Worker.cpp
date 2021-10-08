#include <thread>
#include <functional>
#include <memory>
#include <vector>

#include "msleep.hpp"
#include "concurrentqueue.hpp"

#include "Worker.hpp"
#include "Task.hpp"

Worker::Worker(moodycamel::ConcurrentQueue<Task>& tasks) :
        tasks(tasks),
        state(NoTask),
        body(std::make_shared<std::vector<char>>()),
        stop(true) {}

Worker::Worker(Worker&& other) :
        Worker(other.tasks) {
    other.Stop();
}

Worker& Worker::operator=(Worker&& other) {
    Stop();
    tasks = std::move(other.tasks);
    currentTask = Task();
    state = NoTask;
    other.Stop();
    return *this;
}

Worker::~Worker() {
    Stop();
}

void Worker::TakeNewTask() {
    if (state == NoTask) {
        while (!stop) {
            if (tasks.try_dequeue(currentTask)) {
                state = TaskReceived;
                break;
            }

            msleep(1);
        }
    } else {
        throw std::runtime_error(std::string(
            "Worker: TakeNewTask: state is not NoTask!"));
    }
}

void Worker::RunPreFunc() {
    if (state == TaskReceived) {
        state = PreFuncRunning;
        currentTask.SetMainFunc(currentTask.GetPreFunc()(headers, body, currentTask.GetInput()));
        state = PreFuncRan;
    } else {
        throw std::runtime_error(std::string(
            "Worker: RunPreFunc: state is not TaskReceived!"));
    }
}
void Worker::RunMainFunc() {
    if (state == PreFuncRan) {
        state = MainFuncRunning;
        currentTask.GetMainFunc()(headers, body,
                                  currentTask.GetInput(),
                                  currentTask.GetOutput());
        state = MainFuncRan;
    } else {
        throw std::runtime_error(std::string(
            "Worker: RunMainFunc: state is not PreFuncRan!"));
    }
}
void Worker::RunPostFunc() {
    if (state == MainFuncRan) {
        state = PostFuncRunning;
        currentTask.GetPostFunc()(headers, body, currentTask.GetOutput());
        state = NoTask;
    } else {
        throw std::runtime_error(std::string(
            "Worker: RunPostFunc: state is not MainFuncRan!"));
    }
}

void Worker::Start() {
    if (stop) {
        stop = false;
        state = NoTask;
        workerThread = std::thread(&Worker::Loop, this);
    }
}

void Worker::Loop() {
    while (!stop) {
        TakeNewTask();
        RunPreFunc();
        RunMainFunc();
        RunPostFunc();
    }
}

void Worker::Stop() {
    if (!stop) {
        stop = true;
        workerThread.join();
        state = NoTask;

        headers.clear();
        body->clear();
    }
}
