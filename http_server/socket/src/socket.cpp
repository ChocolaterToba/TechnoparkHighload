#include <arpa/inet.h>    // inet_aton()
#include <fcntl.h>
#include <netdb.h>             // gethostbyname
#include <netinet/in.h>    // struct sockaddr_in
#include <cstring>
#include <sys/socket.h>    // socket(), AF_INET/PF_INET
#include <sys/ioctl.h>

#include <iostream>
#include <stdexcept>
#include <functional>

#include "socket.hpp"

// global
std::string int2ipv4(uint32_t ip) {
    char buf[128];
    snprintf(buf, sizeof(buf), "%u.%u.%u.%u", ip & 0xFF, (ip & 0xFF00) >> 8,
             (ip & 0xFF0000) >> 16, (ip & 0xFF000000) >> 24);
    return buf;
}

namespace {

struct sockaddr_in resolve(const char* host, int port) {
    struct hostent* hp = gethostbyname(host);
    if (NULL == hp) {
        throw std::runtime_error("resolve error: " +
                                 std::string(strerror(errno)));
    }

    char** pAddr = hp->h_addr_list;
    while (*pAddr) {
        unsigned char* ipf = reinterpret_cast<unsigned char*>(*pAddr);
        uint32_t cur_interface_ip = 0;
        uint8_t* rimap_local_ip_ptr = reinterpret_cast<uint8_t*>(&cur_interface_ip);
        rimap_local_ip_ptr[0] = ipf[0];
        rimap_local_ip_ptr[1] = ipf[1];
        rimap_local_ip_ptr[2] = ipf[2];
        rimap_local_ip_ptr[3] = ipf[3];
        // std::cerr << "resolved: " << int2ipv4(cur_interface_ip) << std::endl;
        ++pAddr;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = /*Address Family*/ AF_INET;    // only AF_INET !
    addr.sin_port = htons(port);
    memcpy(&addr.sin_addr, hp->h_addr, hp->h_length);

    return addr;
}

void set_non_blocked_impl(int sd, bool opt) {
    int flags = fcntl(sd, F_GETFL, 0);
    int new_flags = (opt) ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
    if (fcntl(sd, F_SETFL, new_flags) == -1) {
        throw std::runtime_error("make nonblocked: " + std::string(strerror(errno)));
    }
}

}    // namespace

// Move semantics
Socket::Socket(Socket&& other) :
        port(other.port),
        queue_size(other.queue_size) {
    other.close();
    createServerSocket(port, queue_size);
}

Socket& Socket::operator=(Socket&& other) {
    close();
    port = other.port;
    queue_size = other.queue_size;
    other.close();
    createServerSocket(port, queue_size);
    return *this;
}

void Socket::setNonBlocked(bool opt) {
    set_non_blocked_impl(m_Sd, opt);
}

void Socket::setRcvTimeout(int sec, int microsec) {
    struct timeval tv;
    tv.tv_sec = sec;
    tv.tv_usec = microsec;

    if (setsockopt(m_Sd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) != 0) {
        throw std::runtime_error("set rcvtimeout: " + std::string(strerror(errno)));
    }
}

void Socket::setReuseAddr(int sd) {
    int yes = 1;
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        ::close(sd);
        throw std::runtime_error("setopt: " + std::string(strerror(errno)));
    }
}

void Socket::connect(const std::string& host, int port) {
    struct sockaddr_in addr = resolve(host.data(), port);

    int sd = socket(/*Protocol Family*/ PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sd <= 0) {
        throw std::runtime_error("error to create socket: " + std::string(strerror(errno)));
    }

    int connected = ::connect(sd, (struct sockaddr*)&addr, sizeof(addr));
    if (connected == -1) {
        ::close(sd);
        throw std::runtime_error("connect error: " + std::string(strerror(errno)));
    }

    m_Sd = sd;
}

void Socket::connect(const std::string& host, int port, int timeout) {
    struct sockaddr_in addr = resolve(host.data(), port);

    int sd = socket(/*Protocol Family*/ PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sd <= 0) {
        throw std::runtime_error("error to create socket: " + std::string(strerror(errno)));
    }

    set_non_blocked_impl(sd, true);

    int connected = ::connect(sd, (struct sockaddr*)&addr, sizeof(addr));
    if (connected == -1 && errno != EINPROGRESS) {
        ::close(sd);
        throw std::runtime_error("connect error: " + std::string(strerror(errno)));
    }

    fd_set write_fds;
    FD_ZERO(&write_fds);
    FD_SET(sd, &write_fds);
    struct timeval tm;
    tm.tv_sec = timeout;
    tm.tv_usec = 0;

    int sel = select(sd + 1, /*read*/ NULL, /*write*/ &write_fds,
                                     /*exceptions*/ NULL, &tm);

    if (sel != 1) {
        ::close(sd);
        throw std::runtime_error("connect timeout");
    }

    m_Sd = sd;
}

void Socket::send(const std::string& str) {
    size_t left = str.size();
    ssize_t sent = 0;
    // int flags = MSG_DONTWAIT;
    int flags = 0;
    while (left > 0) {
        sent = ::send(m_Sd, str.data() + sent, str.size() - sent, flags);
        if (-1 == sent) {
            throw std::runtime_error("write failed: " + std::string(strerror(errno)));
        }

        left -= sent;
    }
}

void Socket::send(std::shared_ptr<std::vector<char>> data) {
    size_t left = data->size();
    ssize_t sent = 0;
    // int flags = MSG_DONTWAIT;
    int flags = MSG_NOSIGNAL;
    while (left > 0) {
        sent = ::send(m_Sd, data->data() + sent, data->size() - sent, flags);
        if (-1 == sent) {
            throw std::runtime_error("write failed: " + std::string(strerror(errno)));
        }

        left -= sent;
    }
}

std::string Socket::recv() {
    char buf[256];
#ifdef __APPLE__
    // mac os x don't defines MSG_NOSIGNAL
    int n = ::recv(m_Sd, buf, sizeof(buf), 0);
#else
    int n = ::recv(m_Sd, buf, sizeof(buf), MSG_NOSIGNAL);
#endif

    // std::cerr << "errno is " << errno << std::endl;

    if (-1 == n && errno != EAGAIN) {
        throw std::runtime_error("read failed: " + std::string(strerror(errno)));
    }
    if (0 == n) {
        throw std::runtime_error("client: " + std::to_string(m_Sd) + " disconnected");
    }
    if (-1 == n && errno == EAGAIN) {  // or non-blocking socket
        throw std::runtime_error("client: " + std::to_string(m_Sd) + " timeouted");
    }

    std::string ret(buf, buf + n);
    // std::cerr << "client: " << m_Sd << ", recv: " << std::endl
    //           << ret << " [" << n << " bytes]" << std::endl;
    return ret;
}

std::string Socket::recv(size_t bytes) {
    char* buf = new char[bytes];
    size_t r = 0;
    while (r != bytes) {
        ssize_t rc = ::recv(m_Sd, buf + r, bytes - r, 0);
        // std::cerr << "recv_ex: " << rc << " bytes\n";

        if (rc == -1 || rc == 0) {
            delete[] buf;
            throw std::runtime_error("read failed: " + std::string(strerror(errno)));
        }
        r += rc;
    }
    std::string ret(buf, buf + bytes);
    delete[] buf;
    return ret;
}

std::string Socket::recvLoop() {
    char buf[256];
    std::string ret;
    while (true) {
#ifdef __APPLE__
        // mac os x don't defines MSG_NOSIGNAL
        int n = ::recv(m_Sd, buf, sizeof(buf), 0);
#else
        int n = ::recv(m_Sd, buf, sizeof(buf), MSG_NOSIGNAL);
#endif
        if (-1 == n && errno != EAGAIN) {
            throw std::runtime_error("read failed: " + std::string(strerror(errno)));
        }

        if (0 == n || -1 == n) {    // -1 - timeout
            break;
        }
        ret.append(buf, n);
    }
    return ret;
}

std::string Socket::recvTimed(int timeout) {
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(m_Sd, &read_fds);
    struct timeval tm;
    tm.tv_sec = timeout;
    tm.tv_usec = 0;
    int sel = select(m_Sd + 1, /*read*/ &read_fds, /*write*/ NULL, /*exceptions*/ NULL, &tm);
    if (sel != 1) {
        throw std::runtime_error("read timeout");
    }

    return recv();
}

std::vector<char> Socket::recvVector() {
    char buf[256];
#ifdef __APPLE__
    // mac os x don't defines MSG_NOSIGNAL
    int n = ::recv(m_Sd, buf, sizeof(buf), 0);
#else
    int n = ::recv(m_Sd, buf, sizeof(buf), MSG_NOSIGNAL);
#endif

    // std::cerr << "errno is " << errno << std::endl;

    if (-1 == n && errno != EAGAIN) {
        throw std::runtime_error("read failed: " + std::string(strerror(errno)));
    }
    if (0 == n) {
        throw std::runtime_error("client: " + std::to_string(m_Sd) + " disconnected");
    }
    if (-1 == n && errno == EAGAIN) {  // or non-blocking socket
        throw std::runtime_error("client: " + std::to_string(m_Sd) + " timeouted");
    }

    std::vector<char> ret(buf, buf + n);
    // std::cerr << "client: " << m_Sd << ", recv: " << std::endl
    //           << "[may be binary file, cannot be displayed]"
    //           << " [" << n << " bytes]" << std::endl;
    return ret;
}

std::vector<char> Socket::recvVector(size_t bytes) {
    char* buf = new char[bytes];
    size_t r = 0;
    while (r != bytes) {
        ssize_t rc = ::recv(m_Sd, buf + r, bytes - r, 0);
        // std::cerr << "recv_ex: " << rc << " bytes\n";

        if (rc == -1 || rc == 0) {
            delete[] buf;
            throw std::runtime_error("read failed: " + std::string(strerror(errno)));
        }
        r += rc;
    }
    std::vector<char> ret(buf, buf + bytes);
    delete[] buf;
    return ret;
}

std::vector<char> Socket::recvVectorMax(size_t bytesMax) {
    int count;
    ioctl(m_Sd, FIONREAD, &count);
    if (count < 0) {
        return std::vector<char>();
    }

    if (static_cast<size_t>(count) >= bytesMax) {
        count = bytesMax;
    }

    return recvVector(count);
}

std::vector<char> Socket::recvVectorTimed(int timeout) {
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(m_Sd, &read_fds);
    struct timeval tm;
    tm.tv_sec = timeout;
    tm.tv_usec = 0;
    int sel = select(m_Sd + 1, /*read*/ &read_fds, /*write*/ NULL, /*exceptions*/ NULL, &tm);
    if (sel != 1) {
        throw std::runtime_error("read timeout");
    }

    return recvVector();
}

std::vector<char> Socket::recvVectorLoop() {
    char buf[256];
    std::vector<char> ret;
    while (true) {
#ifdef __APPLE__
        // mac os x don't defines MSG_NOSIGNAL
        int n = ::recv(m_Sd, buf, sizeof(buf), 0);
#else
        int n = ::recv(m_Sd, buf, sizeof(buf), MSG_NOSIGNAL);
#endif
        if (-1 == n && errno != EAGAIN) {
            throw std::runtime_error("read failed: " + std::string(strerror(errno)));
        }

        if (0 == n || -1 == n) {    // -1 - timeout
            break;
        }
        ret.insert(ret.end(), buf, buf + n);
    }
    return ret;
}

bool Socket::hasData() {
    int count;
    ioctl(m_Sd, FIONREAD, &count);
    return (count > 0);
}

void Socket::createServerSocket(uint32_t port,
                                uint32_t listen_queue_size) {
    int sd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sd <= 0) {
        throw std::runtime_error("socket: " + std::string(strerror(errno)));
    }

    setReuseAddr(sd);

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);    // inet_addr("0.0.0.0");
    // serv_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    serv_addr.sin_port = htons(port);

    if (bind(sd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        ::close(sd);
        throw std::runtime_error("bind: " + std::string(strerror(errno)));
    }

    listen(sd, listen_queue_size);
    m_Sd = sd;
    this->port = port;
    this->queue_size = listen_queue_size;
}

std::shared_ptr<Socket> Socket::accept() {
    struct sockaddr_in client;
    memset(&client, 0, sizeof(client));
    socklen_t cli_len = sizeof(client);

    // std::cerr << "ready to accept new clients, port: " << this->port << std::endl;
    int cli_sd = ::accept(m_Sd, (struct sockaddr*)&client, &cli_len);
    if (-1 == cli_sd) {
        return std::shared_ptr<Socket>();
    }

    // std::cerr << "new client: " << cli_sd
    //           << ", from: " << int2ipv4(client.sin_addr.s_addr) << std::endl;

    return std::make_shared<Socket>(cli_sd, this->port);
}

void Socket::httpQuery(const std::string& query,
                       std::function<void(const std::string& s)> cb) {
    send(query);
    std::string res = recvLoop();
    // std::cerr << "client: " << m_Sd << ", recv: \n" << res << std::endl;
    cb(res);
}
