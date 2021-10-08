#pragma once

#include <memory>
#include <thread>

#include "blockingconcurrentqueue.hpp"

#include "socket.hpp"
#include "HTTPClient.hpp"

class Listener {
 private:
    Socket socket;
    moodycamel::BlockingConcurrentQueue<HTTPClient>& unprocessedClients;
    moodycamel::ProducerToken unprocessedClientsToken;

    bool stop;

    std::thread listenerThread;

    void Loop();
 
 public:
    explicit Listener(int port, moodycamel::BlockingConcurrentQueue<HTTPClient>& unprocessedClients);

    Listener(const Listener& other) = delete;
    Listener(Listener&& other);

    Listener& operator=(const Listener& other) = delete;
    Listener& operator=(Listener&& other);

    ~Listener();
    
    void Start();
    void Stop();
};
