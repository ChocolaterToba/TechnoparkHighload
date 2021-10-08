#pragma once

#include <thread>
#include <functional>
#include <memory>
#include <vector>

#include "concurrentqueue.hpp"

#include "HTTPClient.hpp"
#include "Task.hpp"

typedef enum {
    NoTask,
    TaskReceived,
    PreFuncRunning,
    PreFuncRan,
    MainFuncRunning,
    MainFuncRan,
    PostFuncRunning
} WorkerStates;

class Worker {
 private:
    moodycamel::ConcurrentQueue<Task>& tasks;
    Task currentTask;
    WorkerStates state;

    std::map<std::string, std::string> headers;
    std::shared_ptr<std::vector<char>> body;

    std::thread workerThread;
    bool stop;

    virtual void TakeNewTask();

    virtual void RunPreFunc();
    virtual void RunMainFunc();
    virtual void RunPostFunc();

    virtual void Loop();

 public:
    Worker(moodycamel::ConcurrentQueue<Task>& tasks);

    Worker(const Worker& other) = delete;
    Worker(Worker&& other);

    Worker& operator=(const Worker& other) = delete;
    Worker& operator=(Worker&& other);

    virtual ~Worker();

    

    virtual void Start();
    virtual void Stop();
};
