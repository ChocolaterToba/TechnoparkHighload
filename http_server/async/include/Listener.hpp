#pragma once

#include <memory>
#include <mutex>
#include <queue>
#include <thread>

#include "socket.hpp"
#include "HTTPClient.hpp"

class Listener {
 private:
    Socket socket;
    std::queue<HTTPClient>& unprocessedClients;
    std::shared_ptr<std::mutex> unprocessedClientsMutex;

    bool stop;

    std::thread listenerThread;

    void Loop();
 
 public:
    explicit Listener(int port,
                      std::queue<HTTPClient>& unprocessedClients,
                      std::shared_ptr<std::mutex> unprocessedClientsMutex);

    Listener(const Listener& other) = delete;
    Listener(Listener&& other);

    Listener& operator=(const Listener& other) = delete;
    Listener& operator=(Listener&& other);

    ~Listener();
    
    void Start();
    void Stop();
};
