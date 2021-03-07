#pragma once

#include <thread>
#include <mutex>
#include <memory>
#include <event2/event.h>

#include "CallbackPackage.hpp"

template <typename T>
class EventLoop {
 private:
    std::shared_ptr<struct event_base> base;
    std::mutex baseMutex;

    event_callback_fn callback;
    std::shared_ptr<T> callbackArgument;
    short eventFlags;
    int timeout;

    std::thread loopThread;
    void RunLoop() {
        while (!stop) {
            event_base_loop(base.get(), EVLOOP_ONCE);
        }
    }
    bool stop;

 public:
    EventLoop(event_callback_fn callback,
              std::shared_ptr<T> callbackArgument = std::shared_ptr<T>(),  // shared_ptr just for emptyness support
              short eventFlags = EV_READ | EV_TIMEOUT | EV_PERSIST,
              int timeout = 120) :
        base(event_base_new(), [](event_base* base) { event_base_free(base); }),
        baseMutex(),
        callback(callback),
        callbackArgument(callbackArgument),
        eventFlags(eventFlags),
        timeout(timeout),
        stop(true) {}

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
        struct event* ev = event_new(base.get(), sd, 0, nullptr, nullptr);
        CallbackPackage<T>* package = new CallbackPackage<T>(callbackArgument, ev);  // maybe replace with smart ptr?
        event_assign(ev, base.get(), sd, eventFlags, callback, package);
        struct timeval timeout = {this->timeout, 0};
        baseMutex.lock();
        event_add(ev, &timeout);
        baseMutex.unlock();
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
