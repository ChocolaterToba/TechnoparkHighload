#include <string>
#include <fstream>
#include <vector>
#include <sys/stat.h>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <set>
#include <random>
#include <chrono>

#include "date.h"


#include "HttpResponse.hpp"
#include "HttpRequest.hpp"
#include "exceptions.hpp"


std::string HttpResponse::GetHTTPVersion() const {
    return http_version;
}

std::string HttpResponse::GetHeader() const {
    return response_header;
}
std::vector<char> HttpResponse::GetData() const {
    return response;
}

HttpResponse::HttpResponse(const std::string& HTTPVersion,
                           RequestMethod reqType,
                           const std::string& url,
                           const std::string& returnCode,
                           bool keepAlive,
                           const std::string& contentLength,
                           const std::vector<char>& body) :
              http_version(HTTPVersion),
              url(url),
              return_code(returnCode),
              keep_alive(keepAlive),
              contentLength(std::stoi(contentLength)) {
    if (HTTPVersion.empty()) {
        http_version = "0.9";
        if (reqType == GET) {
            SetResponseBody(body);
            response.insert(response.begin(), response_body.begin(), response_body.end());
            return;
        } else {
            throw OldVersionException();
        }
    }

    FormResponseHeader();
    SetResponseBody(body);
    FormResponseData();
}

constexpr unsigned int hash(const char *s, int off = 0) {  // Used just for switch()
    return !s[off] ? 5381 : (hash(s, off+1)*33) ^ s[off];
}

ContentType HttpResponse::GetContentType(const std::string& url) {
    const char* ext = url.c_str() + url.rfind('.') + 1;
    switch (hash(ext)) {
        case hash("/"):
            return TXT_HTML;
        case hash("jpg"):
        case hash("JPG"):
        case hash("jpeg"):
        case hash("JPEG"):
            return IMG_JPEG;
        case hash("png"):
        case hash("PNG"):
            return IMG_PNG;
        case hash("gif"):
            return IMG_GIF;
        case hash("swf"):
            return ADOBE_SWF;
        case hash("txt"):
        case hash("TXT"):
            return TXT_PLAIN;
        case hash("html"):
            return TXT_HTML;
        case hash("css"):
            return TXT_CSS;
        case hash("js"):
            return TXT_JS;
        default:
            return UNDEF;
    }
}

void HttpResponse::SetResponseBody(const std::vector<char>& body) {
    response_body.clear();
    response_body.insert(response_body.end(), body.begin(), body.end());
}

void HttpResponse::SetContentType(ContentType type) {
    std::pair<std::string, std::string> c_t_header;
    c_t_header.first = "Content-type";
    switch (type) {
        case TXT_HTML:
            c_t_header.second = "text/html";
            break;
        case TXT_CSS:
            c_t_header.second = "text/css";
            break;
        case TXT_JS:
            c_t_header.second = "text/javascript";
            break;
        case TXT_PLAIN:
            c_t_header.second = "text/plain";
            break;
        case IMG_JPEG:
            c_t_header.second = "image/jpeg";
            break;
        case IMG_PNG:
            c_t_header.second = "image/png";
            break;
        case IMG_GIF:
            c_t_header.second = "image/gif";
            break;
        case ADOBE_SWF:
            c_t_header.second = "application/x-shockwave-flash";
            break;
        default:
            c_t_header.second = "undefined";
            break;
    }

    headers.insert(c_t_header);
}

void HttpResponse::FormResponseHeader() {
    response_header.clear();
    response_header.append("HTTP/").append(http_version).append(" ");
    response_header.append(return_code).append(CRLF);
    
    if (http_version == "1.0" && keep_alive) {
        headers.insert(std::pair<std::string, std::string>("Connection", "Keep-Alive"));
    } else {
        headers.insert(std::pair<std::string, std::string>("Connection", "close"));
    }

    if (!response_body.empty()) {
        headers.insert(std::pair<std::string, std::string>("Content-Length", std::to_string(response_body.size())));
    }

    headers.insert(std::pair<std::string, std::string>("Server", "My cool async server lol"));
    headers.insert(std::pair<std::string, std::string>(
        "Date", date::format("%F %T", std::chrono::system_clock::now())
    ));
    headers.insert(std::pair<std::string, std::string>(
        "Content-Length", std::to_string(contentLength)
    ));

    SetContentType(GetContentType(url));

    for (auto& header_pair : headers) {
        if (!header_pair.second.empty()) {
            response_header.append(header_pair.first).append(": ").append(header_pair.second).append(CRLF);
        }
    }

    response_header.append(CRLF);
}

void HttpResponse::FormResponseData() {
    response.assign(response_header.begin(), response_header.end());
    response.insert(response.end(), response_body.begin(), response_body.end());
}
