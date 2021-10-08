#pragma once

#include <unistd.h>    // close()
#include <memory>
#include <string>
#include <vector>
#include <functional>

std::string int2ipv4(uint32_t ip);

class Socket {
 public:
    Socket() : m_Sd(-1) {}
    explicit Socket(int sd, int port = 0) : m_Sd(sd), port(port), queue_size(0) {}

    Socket(const Socket& other) = delete;
    Socket(Socket&& other);

    Socket& operator=(const Socket& other) = delete;
    Socket& operator=(Socket&& other);
    ~Socket() {
        if (m_Sd > 0) {
           ::close(m_Sd);
        }
    }

 public:
    int sd() const noexcept { return m_Sd; }

    void connect(const std::string& host, int port);
    void connect(const std::string& host,
                 int port,
                 int timeout);

    void send(const std::string& s);
    void send(std::shared_ptr<std::vector<char>> s);

    bool hasData();

    int getPort() const { return port; }

    std::string recv();
    std::string recv(size_t bytes);
    std::string recvTimed(int timeout);
    std::string recvLoop();

    std::vector<char> recvVector();
    std::vector<char> recvVector(size_t bytes);
    std::vector<char> recvVectorMax(size_t bytesMax);
    std::vector<char> recvVectorTimed(int timeout);
    std::vector<char> recvVectorLoop();

    void setRcvTimeout(int sec, int microsec);
    void setNonBlocked(bool opt);
    void setReuseAddr(int sd);

    void createServerSocket(uint32_t port,
                            uint32_t queue_size);
    std::shared_ptr<Socket> accept();
    void close() {
       ::close(m_Sd);
       m_Sd = -1;}

    void httpQuery(const std::string& query,
                   std::function<void(const std::string& s)> cb);

   //  std::string httpHeader();  TODO: not implemented yet

 private:
    int m_Sd;
    uint32_t port;
    uint32_t queue_size;
};
