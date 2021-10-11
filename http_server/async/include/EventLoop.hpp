#pragma once

#include <thread>
#include <mutex>
#include <memory>
#include <event2/event.h>
#include <event2/thread.h>
#include <pthread.h>

#include "CallbackPackage.hpp"

template <typename T>
class EventLoop {
 private:
    std::shared_ptr<struct event_base> base;

    event_callback_fn callback;
    std::shared_ptr<T> callbackArgument;
    short eventFlags;
    int timeout;

    std::thread loopThread;
    void RunLoop() {
        while (!stop) {
            eventLoopRunningMutex.lock();

            event_base_loop(base.get(), EVLOOP_NO_EXIT_ON_EMPTY);

            eventLoopRunningMutex.unlock();
            addEventRunningMutex.lock(); // This lock is needed so that eventloop does not lock before AddEvent is finished
            addEventRunningMutex.unlock();
        }
    }
    bool stop;

    std::mutex eventLoopRunningMutex;
    std::mutex addEventRunningMutex;

 public:
    EventLoop(event_callback_fn callback,
              std::shared_ptr<T> callbackArgument = std::shared_ptr<T>(),  // shared_ptr just for emptyness support
              short eventFlags = EV_READ | EV_TIMEOUT | EV_PERSIST,
              int timeout = 120) :
        callback(callback),
        callbackArgument(callbackArgument),
        eventFlags(eventFlags),
        timeout(timeout),
        stop(true) {
            evthread_use_pthreads();
            // base is initialised here because evthread_use_pthreads had to be called first, so that libevent is multithread-usable
            base = std::move(std::shared_ptr<event_base>(
                event_base_new(), [](event_base* base) { event_base_free(base); }
            ));
        }

    ~EventLoop() {
        // TODO: add package removal
        StopLoop();
    }

    EventLoop(EventLoop&& other) = delete;
    EventLoop& operator=(EventLoop&& other) = delete;
    EventLoop(EventLoop& other) = delete;
    EventLoop& operator=(EventLoop& other) = delete;

    void SetCallbackArgument(std::shared_ptr<T> callbackArgument){
        this->callbackArgument = callbackArgument;
    }

    void AddEvent(evutil_socket_t sd) {
        CallbackPackage<T>* package = new CallbackPackage<T>(callbackArgument);  // maybe replace with smart ptr?
        struct event* ev = event_new(base.get(), sd, eventFlags, callback, package);
        package->ev = ev;
        struct timeval timeout = {this->timeout, 0};

        addEventRunningMutex.lock();
        event_base_loopexit(base.get(), nullptr);
        eventLoopRunningMutex.lock();
        event_add(ev, &timeout);

        addEventRunningMutex.unlock();
        eventLoopRunningMutex.unlock();
    }
    void FreeEvent(event* ev) {
        // baseMutex.lock(); // Not needed - this function should only be used in callback, which is called by RunLoop
        event_free(ev);
        // baseMutex.unlock();
    }

    void StartLoop() {
        if (stop) {
            stop = false;
            loopThread = std::thread(&EventLoop::RunLoop, this);
        }
    }
    void StopLoop() {
        if (!stop) {
            stop = true;
            loopThread.join();
        }
    }
};
