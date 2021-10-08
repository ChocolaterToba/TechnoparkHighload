#pragma once

#include <string>
#include <vector>
#include <queue>
#include <set>
#include <map>
#include <memory>
#include "socket.hpp"

class HTTPClient {
 private:
    std::string header;
    std::shared_ptr<std::vector<char>> body;

    std::shared_ptr<Socket> socket;

    std::vector<char>::iterator ParseBuffer(std::vector<char>& buf, std::string& result);

    bool receivedHeader;
    size_t contentLength;

 public:
    explicit HTTPClient(std::shared_ptr<Socket> socket, int timeout);  // "We" are server
    explicit HTTPClient(int port, int queueSize, int timeout = 120);  // "We" are server
    explicit HTTPClient(const std::string& host, int port);  // "We" are client
    HTTPClient(std::shared_ptr<Socket> socket = std::make_shared<Socket>());

    std::string GetHeader() const { return header; }
    std::shared_ptr<std::vector<char>> GetBody() const { return body; }
    std::queue<std::string> GetBodyQueue(const std::string& separator = "|") const;

    static std::queue<std::string> SplitVectorToQueue(const std::vector<char>& origin, const std::string& separator = "|");
    static std::vector<char> MergeQueueToVector(std::queue<std::string>& origin, const std::string& separator = "|");

    static std::set<std::string> SplitVectorToSet(const std::vector<char>& origin, const std::string& separator = "|");
    static std::vector<char> MergeSetToVector(const std::set<std::string>& origin, const std::string& separator = "|");

    static std::map<std::string, std::string> SplitVectorToMap(const std::vector<char>& origin,
                                                               const std::string& separator = "|",
                                                               const std::string& pairSeparator = ": ");
    static std::vector<char> MergeMapToVector(std::map<std::string, std::string>& origin,
                                              const std::string& separator = "|",
                                              const std::string& pairSeparator = ": ");

    void SetHeader(std::string header) { this->header = std::move(header); }
    void SetBody(std::vector<char> body) { this->body = std::make_shared<std::vector<char>>(body); }
    void SetBody(std::queue<std::string>& bodyQueue, const std::string& separator = "|");
    void SetContentLength(size_t contentLength) { this->contentLength = contentLength; }

    void RecvHeader();
    void RecvBody();

    void RecvHeaderAsync();
    bool RecvBodyAsync();

    void Send(bool close = false);
    void Send(std::shared_ptr<std::vector<char>> data, bool close = false);

    bool HasData() const { return socket->hasData(); }
    bool ReceivedHeader();

    int GetPort() const;
    int GetSd() const;

    void Clear();
    void Close();
};
