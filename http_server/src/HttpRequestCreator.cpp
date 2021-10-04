#include <string>
#include <fstream>
#include <vector>
#include <sys/stat.h>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <set>
#include <random>


#include "HttpRequestCreator.hpp"
#include "exceptions.hpp"

HttpRequestCreator::HttpRequestCreator(const string& HTTPVersion,
                                       RequestMethod reqType,
                                       const string& url,
                                       bool keepAlive,
                                       const std::vector<char>& body) :
                   http_version(HTTPVersion),
                   reqType(reqType),
                   url(url),
                   keep_alive(keepAlive) {
    if (HTTPVersion.empty()) {
        http_version = "0.9";
        if (reqType == GET) {
            requestHeader = "GET " + url + CRLF;
            request.insert(request.begin(), requestHeader.begin(), requestHeader.end());
            return;
        } else {
            throw OldVersionException();
        }
    }

    SetRequestBody(body);
    FormRequestHeader();
    FormRequestData();
}

string HttpRequestCreator::GetHTTPVersion() const {
    return http_version;
}

string HttpRequestCreator::GetHeader() const {
    return requestHeader;
}
std::vector<char> HttpRequestCreator::GetRequest() const {
    return request;
}

void HttpRequestCreator::FormRequestHeader() {
    requestHeader.clear();
    requestHeader.append(RequestMethodToString(reqType)).append(" ").append(url).append(" ");
    requestHeader.append("HTTP/").append(http_version).append(" ").append(CRLF);
    
    if (http_version == "1.0" && keep_alive) {
        headers.insert(std::pair<string, string>("Connection", "Keep-Alive"));
    }
    if (!requestBody.empty()) {
        headers.insert(std::pair<string, string>("Content-Length", std::to_string(requestBody.size())));
    }
    for (auto& header_pair : headers) {
        if (!header_pair.second.empty()) {
            requestHeader.append(header_pair.first).append(": ").append(header_pair.second).append(CRLF);
        }
    }
}

void HttpRequestCreator::SetRequestBody(const std::vector<char>& body) {
    requestBody.clear();
    requestBody.insert(requestBody.end(), body.begin(), body.end());
}

void HttpRequestCreator::FormRequestData() {
    request.assign(requestHeader.begin(), requestHeader.end());
    request.insert(request.end(), CRLF, CRLF + sizeof(CRLF) - 1);  // -1 because CRLF ends in '\0'
    request.insert(request.end(), requestBody.begin(), requestBody.end());
}

string HttpRequestCreator::RequestMethodToString(RequestMethod method) {
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

RequestMethod HttpRequestCreator::StringToRequestMethod(const string& methodString) {
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


string HttpRequestCreator::GetQueryParameter(const string &parameter_name) const {
    string value;
    try {
        value = query.at(parameter_name);
    }
    catch (std::exception &) {
        value = "";
    }

    return value;
}

void HttpRequestCreator::ProcessQueryString() {
    string key;
    string value;
    size_t pos = 0;
    while ((pos = url.find('?', pos)) != string::npos) {
        size_t end_key = url.find('=', pos);
        key = url.substr(pos + 1, end_key - pos - 1);
        while (url[pos] != '?' && url[pos] != '\0')
            ++pos;
        value = url.substr(end_key + 1, pos - end_key - 1);

        query[key] = value;
    }
}

std::map<string, string> HttpRequestCreator::GetQueryString() const {
    return query;
}
