#pragma once

#include <map>
#include <string>

#include "request_methods.hpp"

// string default value ""

/* 
0 2 4      13 15   20 22     
GET /index.php HTTP/1.1
Host: example.com
User-Agent: Mozilla/5.0 (X11; U; Linux i686; ru; rv:1.9b5) Gecko/2008050509 Firefox/3.0b5
Accept: text/html
Connection: close
*/

class HttpRequest {
 public:
    explicit HttpRequest(const std::string &message);

    std::string GetHeader(const std::basic_string<char>& headerName) const;
    ~HttpRequest() = default;
    std::string GetURL() const;
    std::string GetHTTPVersion() const;
    RequestMethod GetRequestMethod() const;
    std::string GetRequestMethodString() const;
    std::map<std::string, std::string> GetAllHeaders() const;
    size_t GetContentLength() const;

    static std::string RequestMethodToString(RequestMethod method);
    static RequestMethod StringToRequestMethod(const std::string& methodString);

 private:
    RequestMethod request_method;
    std::string url;
    std::string http_version;
    std::map<std::string, std::string> headers;

    void SetRequestMethod(const std::string &method_name);
};
