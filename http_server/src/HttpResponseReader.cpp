#include "HttpResponseReader.hpp"
#include <iostream>

HttpResponseReader::HttpResponseReader(const std::string &message) {
    return_code = GetReturnCode(message);
    headers = ExtractHeaders(message);
}

std::string HttpResponseReader::GetReturnCode(const std::string &message) const {
    size_t end = message.find('\r');
    size_t start = end;
    while (!std::isdigit(message[start])) {
        --start;
    }

    return message.substr(start - 2, end - start + 1);  // Full return code with additional text
}

std::string HttpResponseReader::GetReturnCode() const {
    return return_code;
}

std::map<std::string, std::string> HttpResponseReader::GetAllHeaders() const {
    return headers;
}

size_t HttpResponseReader::GetContentLength() const {
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

std::string HttpResponseReader::GetHeaderValue(const std::string &header_name) const {
    if (headers.contains(header_name)) {
        return headers.at(header_name);
    } else {
        return std::string();
    }    
}

std::map<std::string, std::string> HttpResponseReader::ExtractHeaders(const std::string &message) {
    std::map<std::string, std::string> result;

    size_t search = message.find('\r', 0) + 2;  // "\r\n"
    size_t pos;
    while ((pos = message.find(": ", search)) != std::string::npos) {
        std::string header_name = message.substr(search, pos - search);
        pos += 2;  // ": "
        search = message.find('\r', pos);

        std::string header_value = message.substr(pos, search - pos);
        result[header_name] = header_value;
        search += 2;  // \r\n 2 symbols
    }

    return result;
}
