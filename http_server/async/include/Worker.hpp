#pragma once

#include <thread>
#include <mutex>
#include <functional>
#include <memory>
#include <vector>
#include <queue>
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
    std::queue<Task>& tasks;
    std::shared_ptr<std::mutex> tasksMutex;
    Task currentTask;
    WorkerStates state;

    std::map<std::string, std::string> headers;
    std::vector<char> data;

    std::thread workerThread;
    bool stop;

    virtual void TakeNewTask();

    virtual void RunPreFunc();
    virtual void RunMainFunc();
    virtual void RunPostFunc();

    virtual void Loop();

 public:
    Worker(std::queue<Task>& tasks,
           std::shared_ptr<std::mutex> tasksMutex);

    Worker(const Worker& other) = delete;
    Worker(Worker&& other);

    Worker& operator=(const Worker& other) = delete;
    Worker& operator=(Worker&& other);

    virtual ~Worker();

    

    virtual void Start();
    virtual void Stop();
};
