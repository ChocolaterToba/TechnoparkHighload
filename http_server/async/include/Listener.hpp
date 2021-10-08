#pragma once

#include <memory>
#include <thread>

#include "concurrentqueue.hpp"

#include "socket.hpp"
#include "HTTPClient.hpp"

class Listener {
 private:
    Socket socket;
    moodycamel::ConcurrentQueue<HTTPClient>& unprocessedClients;

    bool stop;

    std::thread listenerThread;

    void Loop();
 
 public:
    explicit Listener(int port, moodycamel::ConcurrentQueue<HTTPClient>& unprocessedClients);

    Listener(const Listener& other) = delete;
    Listener(Listener&& other);

    Listener& operator=(const Listener& other) = delete;
    Listener& operator=(Listener&& other);

    ~Listener();
    
    void Start();
    void Stop();
};
