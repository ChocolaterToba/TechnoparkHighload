#pragma once

#include <map>
#include <string>
#include <vector>

#include "content_types.hpp"
#include "request_methods.hpp"

#define CRLF "\r\n"
#define BUF_SIZE 256



class HttpResponse {
 public:
    HttpResponse(const std::string& HTTPVersion,
                 RequestMethod reqType,
                 const std::string& url,
                 const std::string& returnCode,
                 bool keepAlive,
                 const std::string& contentLength,
                 const std::shared_ptr<std::vector<char>> body);

    HttpResponse() = delete;
    std::string GetHTTPVersion() const;
    std::string GetHeader() const;
    std::shared_ptr<std::vector<char>> GetData() const;

    static ContentType GetContentType(const std::string& url);

 private:
    std::string http_version;
    std::string url;
    std::string return_code;
    bool keep_alive;

    std::map<std::string, std::string> headers;
    std::string response_header;
    size_t contentLength;
    std::shared_ptr<std::vector<char>> response_body;
    std::shared_ptr<std::vector<char>> response;

    void SetContentType(ContentType type);
    void FormResponseHeader();
    void FormResponseData();
};
