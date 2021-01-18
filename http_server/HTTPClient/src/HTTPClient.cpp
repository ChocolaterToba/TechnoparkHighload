#include <string>
#include <vector>
#include <queue>
#include <set>
#include <map>
#include <memory>
#include "socket.hpp"
#include "HTTPClient.hpp"

int BUFFER_SIZE = 256;


HTTPClient::HTTPClient(std::shared_ptr<Socket> socket, int timeout) : HTTPClient(socket) {
    socket->setRcvTimeout(/*sec*/ timeout, /*microsec*/ 0);
}

HTTPClient::HTTPClient(int port, int queueSize, int timeout) : HTTPClient() {
    socket->createServerSocket(port, queueSize);
    socket->setRcvTimeout(/*sec*/ timeout, /*microsec*/ 0);
}

HTTPClient::HTTPClient(const std::string& host, int port) : HTTPClient() {
    socket->connect(host, port);
}

HTTPClient::HTTPClient(std::shared_ptr<Socket> socket) :
            socket(socket),
            receivedHeader(false),
            receivedBodySize(0) {}

std::vector<char>::iterator HTTPClient::ParseBuffer(std::vector<char>& buffer, std::string& target) {
    // Returns true if '\0' was found, which means that binary body started.
    auto endlineIter = std::find(buffer.begin(), buffer.end(), '\0');
    target += std::string(buffer.begin(), endlineIter);
    
    return endlineIter;
}

void HTTPClient::SetBody(std::queue<std::string>& bodyQueue, const std::string& separator) {
    body.clear();
    for (size_t i = 0; i < bodyQueue.size(); ++i) {
        std::string& str = bodyQueue.front();
        std::copy(str.begin(), str.end(), std::back_inserter(body));
        bodyQueue.pop();
        bodyQueue.push(str);
        if (i < bodyQueue.size() - 1) {
            for (auto& character : separator) {
                body.push_back(character);
            }
        }
    }
}

std::queue<std::string> HTTPClient::GetBodyQueue(const std::string& separator) const {
    return SplitVectorToQueue(body, separator);
}

std::queue<std::string> HTTPClient::SplitVectorToQueue(const std::vector<char>& origin, const std::string& separator) {
    std::string bodyString(origin.begin(), origin.end());
    std::queue<std::string> result;

    size_t start = 0;
    size_t end = 0;
    while ((end = bodyString.find(separator, start)) != std::string::npos) {
        result.push(bodyString.substr(start, end - start));
        start = end + separator.length();
    }
    if (start < bodyString.size()) {  // Avoiding cases when origin ends in separator
        result.push(bodyString.substr(start));
    }

    return result;
}

std::vector<char> HTTPClient::MergeQueueToVector(std::queue<std::string>& origin, const std::string& separator) {
    std::vector<char> result;
    for (size_t i = 0; i < origin.size(); ++i) {
        result.insert(result.end(), origin.front().begin(), origin.front().end());
        origin.push(origin.front());
        origin.pop();
        result.insert(result.end(), separator.begin(), separator.end());
    }

    return result;
}

std::set<std::string> HTTPClient::SplitVectorToSet(const std::vector<char>& origin, const std::string& separator) {
    std::string bodyString(origin.begin(), origin.end());
    std::set<std::string> result;

    size_t start = 0;
    size_t end = 0;
    while ((end = bodyString.find(separator, start)) != std::string::npos) {
        result.insert(bodyString.substr(start, end - start));
        start = end + separator.length();
    }
    if (start < bodyString.size()) {  // Avoiding cases when origin ends in separator
        result.insert(bodyString.substr(start));
    }

    return result;
}

std::vector<char> HTTPClient::MergeSetToVector(const std::set<std::string>& origin, const std::string& separator) {
    std::vector<char> result;
    for (auto& param : origin) {
        result.insert(result.end(), param.begin(), param.end());
        result.insert(result.end(), separator.begin(), separator.end());
    }

    return result;
}

std::map<std::string, std::string> HTTPClient::SplitVectorToMap(const std::vector<char>& origin, const std::string& separator, const std::string& pairSeparator) {
    std::string bodyString(origin.begin(), origin.end());
    std::map<std::string, std::string> result;

    size_t start = 0;
    size_t end = 0;
    while ((end = bodyString.find(separator, start)) != std::string::npos) {
        std::string paramPair = std::move(bodyString.substr(start, end - start));
        std::size_t splitPos = paramPair.find(pairSeparator);
        result.insert(std::pair<std::string, std::string>(paramPair.substr(0, splitPos), paramPair.substr(splitPos + 2)));
        start = end + separator.length();
    }
    if (start < bodyString.size()) {  // Avoiding cases when origin ends in separator
        std::string paramPair = std::move(bodyString.substr(start));
        std::size_t splitPos = paramPair.find(": ");
        result.insert(std::pair<std::string, std::string>(paramPair.substr(0, splitPos), paramPair.substr(splitPos + 2)));
    }

    return result;
}

std::vector<char> HTTPClient::MergeMapToVector(std::map<std::string, std::string>& origin, const std::string& separator, const std::string& pairSeparator) {
    std::vector<char> result;
    for (auto& paramPair : origin) {
        result.insert(result.end(), paramPair.first.begin(), paramPair.first.end());
        result.insert(result.end(), pairSeparator.begin(), pairSeparator.end());
        result.insert(result.end(), paramPair.second.begin(), paramPair.second.end());
        result.insert(result.end(), separator.begin(), separator.end());
    }

    return result;
}

void HTTPClient::RecvHeader() {
    header.clear();
    body.clear();
    std::string result;
    std::vector<char> buffer;

    bool binaryBodyStarted = false;
    while (result.find("\r\n\r\n") == std::string::npos &&
           result.find("\n\n") == std::string::npos) {
        buffer = std::move(socket->recvVector());

        auto endlineIter = ParseBuffer(buffer, result);
        if (endlineIter != buffer.end()) {  // if '\0' was in buffer
            binaryBodyStarted = true;
            body.insert(body.begin(), endlineIter, buffer.end());
            break;
        }
    }

    size_t bodyStartIndex = result.find("\r\n\r\n");
    size_t shift = 4;
    if (bodyStartIndex == std::string::npos) {
        bodyStartIndex = result.find("\n\n");
        shift = 2;
        if (bodyStartIndex == std::string::npos) {
            throw std::runtime_error(std::string("recvHeader: Met \\0 character in header!"));
        }
    }

    header = result.substr(0, bodyStartIndex + shift / 2);

    std::cerr <<"Received header, size: " << header.size() << " bytes:" << std::endl
              << header << std::endl;

    std::vector<char> temp(result.begin() + bodyStartIndex + shift, result.end());
    if (binaryBodyStarted) {
        temp.insert(temp.end(), body.begin(), body.end());
    }
    body = std::move(temp);
}

void HTTPClient::RecvBody(size_t contentLength) {
    contentLength -= body.size();
    std::vector<char> receivedBody = std::move(socket->recvVector(contentLength));
    body.insert(body.end(), receivedBody.begin(), receivedBody.end());

    std::cerr << "Received body, size: " << body.size() << " bytes" << std::endl;
}

void HTTPClient::RecvHeaderAsync() {
    std::string result;
    std::vector<char> buffer;

    bool binaryBodyStarted = false;
    buffer = std::move(socket->recvVector());

    auto endlineIter = ParseBuffer(buffer, result);
    if (endlineIter != buffer.end()) {  // if '\0' was in buffer
        binaryBodyStarted = true;
        body.insert(body.begin(), endlineIter, buffer.end());
    }

    size_t bodyStartIndex = result.find("\r\n\r\n");
    size_t shift = 0;
    if (bodyStartIndex != std::string::npos) {
        shift = 4;
        receivedHeader = true;
    } else {
        bodyStartIndex = result.find("\n\n");
        if (bodyStartIndex != std::string::npos) {
            shift = 2;
            receivedHeader = true;
        }
    }

    header.append(result.substr(0, bodyStartIndex + shift / 2));

    std::cerr <<"Received header's part, size: " << header.size() << " bytes:" << std::endl;
    if (receivedHeader) {
        std::cout << header << std::endl;
    }

    std::vector<char> temp(result.begin() + bodyStartIndex + shift, result.end());
    if (binaryBodyStarted) {
        temp.insert(temp.end(), body.begin(), body.end());
    }
    body = std::move(temp);
}

bool HTTPClient::RecvBodyAsync(size_t contentLength) {
    std::vector<char> receivedBody = std::move(socket->recvVectorMax(contentLength - body.size()));
    body.insert(body.end(), receivedBody.begin(), receivedBody.end());

    std::cerr << "Received body, size: " << body.size() << " bytes" << std::endl;
    return body.size() == contentLength;
}

bool HTTPClient::ReceivedHeader() {
    return receivedHeader;
}

void HTTPClient::Send(bool close) {
    socket->send(header + "\r\n\r\n");
    socket->send(body);
    if (close) {
        socket->close();
    }
}

void HTTPClient::Send(std::vector<char> data, bool close) {
    socket->send(std::move(data));
    if (close) {
        socket.reset();
    }
}

int HTTPClient::GetPort() const {
    if (socket != nullptr) {
        return socket->getPort();
    }
    return -1;
}

int HTTPClient::GetSd() const {
    if (socket != nullptr) {
        return socket->sd();
    }
    return -1;
}

void HTTPClient::Clear() {
    header.clear();
    body.clear();
}

void HTTPClient::Close() {
    socket.reset();
    Clear();
}
