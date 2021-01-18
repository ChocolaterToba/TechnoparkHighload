#include <string>
#include <map>

#include "exceptions.hpp"
#include "HttpRequest.hpp"

HttpRequest::HttpRequest(const std::string& message) {
    // find method
    size_t pos = 0;
    size_t search = message.find(' ');
    std::string method = message.substr(pos, search);
    SetRequestMethod(method);
    if (request_method == UNKNOWN) {
        throw BadFormatException();
    }

    // find URL
    ++search;
    pos = search;
    while ((message[search] != ' ') && (message[search] != '\r')) {
        ++search;
    }
    if (search == pos) {
        throw BadFormatException();
    }
    url = message.substr(pos, search - pos);

    // find HTTP version
    ++search;
    pos = 0;
    if ((pos = message.find("HTTP/", search - 1, 5)) != std::string::npos) {
        pos += 5;
        search = pos;
        while (message[search] != '\r') {
            ++search;
        }
        http_version = message.substr(pos, search - pos);
    } else {
        http_version = std::string();   // check
    }
    search += 2;   // \r\n 2 symbols

    // get headers
    while ((pos = message.find(": ", search)) != std::string::npos) {
        std::string key = message.substr(search, pos - search);
        pos += 2;
        search = pos;
        while (message[search] != '\r') {
            ++search;
        }
        std::string value = message.substr(pos, search - pos);
        headers.insert(std::pair<std::string, std::string>(key, value));
        search += 2;  // \r\n 2 symbols
    }
}

std::string HttpRequest::GetHeader(const std::basic_string<char>& headerName) const {
    auto iter = headers.find(headerName);
    if (iter == headers.end()) {
        return std::string();
    } else {
        return iter->second;
    }
}

void HttpRequest::SetRequestMethod(const std::string& method_name) {
    request_method = StringToRequestMethod(method_name);
}

std::string HttpRequest::GetURL() const {
    return url;
}

std::string HttpRequest::GetHTTPVersion() const {
    return http_version;
}

RequestMethod HttpRequest::GetRequestMethod() const {
    return request_method;
}

std::string HttpRequest::GetRequestMethodString() const {
    return RequestMethodToString(request_method);
}

std::map<std::string, std::string> HttpRequest::GetAllHeaders() const {
    return headers;
}

size_t HttpRequest::GetContentLength() const {
    int result = 0;
    try {
        result = std::stoi(headers.at("Content-Length"));
        if (result < 0) {
            result = 0;
        }
    }
    catch (std::invalid_argument &e) {
        result = 0;
    }
    catch (std::out_of_range& e) {
        result = 0;
    }
    return static_cast<size_t>(result);
}

std::string HttpRequest::RequestMethodToString(RequestMethod method) {
    switch (method) {
        case GET:
            return "GET";
        case POST:
            return "POST";
        case OPTIONS:
            return "OPTIONS";
        case HEAD:
            return "HEAD";
        case PUT:
            return "PUT";
        case PATCH:
            return "PATCH";
        case DELETE:
            return "DELETE";
        case CONNECT:
            return "CONNECT";
        default:
            return "UNKNOWN";
    }
}

RequestMethod HttpRequest::StringToRequestMethod(const std::string& methodString) {
    RequestMethod method;
    if (methodString == "GET") {
        method = GET;
    } else if (methodString == "POST") {
        method = POST;
    } else if (methodString == "OPTIONS") {
        method = OPTIONS;
    } else if (methodString == "HEAD") {
        method = HEAD;
    } else if (methodString == "PUT") {
        method = PUT;
    } else if (methodString == "PATCH") {
        method = PATCH;
    } else if (methodString == "DELETE") {
        method = DELETE;
    } else if (methodString == "CONNECT") {
        method = CONNECT;
    } else {
        method = UNKNOWN;
    }
    return method;
}
