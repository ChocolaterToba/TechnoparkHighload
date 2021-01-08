#include <thread>
#include <mutex>
#include <memory>
#include <event2/event.h>

#include "EventLoop.hpp"

template <typename T>
EventLoop<T>::EventLoop(event_callback_fn callback, std::shared_ptr<T> callbackArgument, 
                        short eventFlags, int timeout) :
              base(event_base_new(), [](event_base* base) { event_base_free(base); }),
              baseMutex(),
              callback(callback),
              callbackArgument(callbackArgument),
              eventFlags(eventFlags),
              timeout(timeout),
              stop(true) {}

template <typename T>
EventLoop<T>::~EventLoop() {
    StopLoop();
}

template <typename T>
void EventLoop<T>::SetCallbackArgument(std::shared_ptr<T> callbackArgument) {
    this->callbackArgument = callbackArgument;
}

template <typename T>
void EventLoop<T>::AddEvent(evutil_socket_t sd) {
    struct timeval timeout = {this->timeout, 0};
    baseMutex.lock();
    event_base_once(base.get(), sd, EV_TIMEOUT | EV_READ,
                    callback, &callbackArgument, &timeout);  // TODO: async recv
    baseMutex.unlock();
}

template <typename T>
void EventLoop<T>::StartLoop() {
    if (stop) {
        stop = false;
        loopThread = std::thread(&EventLoop::RunLoop, this);
    }
}

template <typename T>
void EventLoop<T>::RunLoop() {
    while (!stop) {
        event_base_loop(base.get(), EVLOOP_ONCE);
    }
}

template <typename T>
void EventLoop<T>::StopLoop() {
    if (!stop) {
        stop = true;
        loopThread.join();
    }
}
