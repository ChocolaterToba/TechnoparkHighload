#include <memory>
#include <thread>
#include <event2/event.h>

#include "blockingconcurrentqueue.hpp"

#include "socket.hpp"
#include "HTTPClient.hpp"
#include "Listener.hpp"

Listener::Listener(int port,
                   moodycamel::BlockingConcurrentQueue<HTTPClient>& unprocessedClients) :
          socket(), 
          unprocessedClients(unprocessedClients),
          unprocessedClientsToken(unprocessedClients),
          stop(true) {
    socket.createServerSocket(port, 5);
}

Listener::Listener(Listener&& other) :
    unprocessedClients(other.unprocessedClients),
    unprocessedClientsToken(std::move(other.unprocessedClientsToken)),
    stop(true) {
    other.Stop();
    socket = std::move(other.socket);
}

Listener& Listener::operator=(Listener&& other) {
    Stop();
    other.Stop();
    socket = std::move(other.socket);
    unprocessedClients = std::move(other.unprocessedClients);
    unprocessedClientsToken = std::move(other.unprocessedClientsToken);
    return *this;
}

Listener::~Listener() {
    Stop();
}

void Listener::Start() {
    if (stop) {
        stop = false;
        listenerThread = std::thread(&Listener::Loop, this);
    }
}

void Listener::Loop() {
    while (!stop) {
        HTTPClient client(socket.accept());
        unprocessedClients.enqueue(unprocessedClientsToken, std::move(client));
    }
}

void Listener::Stop() {
    if (!stop) {
        stop = true;
        listenerThread.join();
    }
}
