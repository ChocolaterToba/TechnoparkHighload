#include <vector>
#include <map>
#include <string>
#include <fstream>
#include <algorithm>
#include <iterator>

#include "ports.hpp"
#include "cache.hpp"
#include "lfu_cache_policy.hpp"

#include "HTTPClient.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "HttpRequestCreator.hpp"
#include "HttpResponseReader.hpp"
#include "TaskFuncs.hpp"

using std::string;
using std::map;
using std::vector;


MainFuncType PreProcess(map<string, string>& headers, std::shared_ptr<vector<char>> body, HTTPClient& input) {
    HttpRequest request(input.GetHeader());
    headers = request.GetAllHeaders();
    headers["url"] = request.GetURL();
    headers["method"] = request.GetRequestMethodString();
    headers["http_version"] = request.GetHTTPVersion();

    body = input.GetBody();

    return MainProcessBasic;
}

template <typename Key, typename Value>
using lfu_cache_t = typename caches::fixed_sized_cache<Key, Value, caches::LFUCachePolicy>;
lfu_cache_t<std::string, std::vector<char>> fileCache(32);

int ReadFile(const std::string& filename, std::shared_ptr<vector<char>> body, bool actuallyRead) {
    try {
        if (!actuallyRead) {
            return fileCache.Get(filename).size();
        }

        *body = fileCache.Get(filename);
        return body->size();

    } catch (const std::range_error &e) {
        std::ifstream file;
        file.open(filename, std::ios::in | std::ios :: binary);

        if (file.eof() || file.fail()) {
            return -1;
        }

        file.seekg(0, std::ios_base::end);
        std::streampos fileSize = file.tellg();

        if (!actuallyRead) {
            return fileSize;
        }

        body->resize(fileSize);

        file.seekg(0, std::ios_base::beg);
        file.read(body->data(), fileSize);
        fileCache.Put(filename, *body);
        return fileSize;
    }
}

void MainProcessBasic(map<string, string>& headers, std::shared_ptr<vector<char>> body,
                      HTTPClient& input, HTTPClient& output) {
    if (headers["method"] != "GET" && headers["method"] != "HEAD") {
        headers["return_code"] = "405 Method Not Allowed";
        output = std::move(input);
        return;
    }

    // TODO: decoding %

    if (headers["url"].find("/../") != std::string::npos) {
        headers["return_code"] = "403 Forbidden";
        headers["Content-Length"] = "0";
        output = std::move(input);
        return;
    }

    std::string path;
    std::string filename;

    size_t queryStartPos = headers["url"].find('?');
    size_t finalDotPos = headers["url"].rfind('.', queryStartPos);
    if (finalDotPos != std::string::npos) {
        size_t finalSlashPos = headers["url"].rfind('/', finalDotPos);
        filename = headers["url"].substr(finalSlashPos + 1, queryStartPos - finalSlashPos - 1);
        path = std::string(std::getenv("DOCUMENT_ROOT")) + "/" +
            headers["url"].substr(1, finalSlashPos);

        if (filename == "") {
            filename = "index.html";
        }
    } else {
        filename = "index.html";
        path = headers["url"];
    }

    int readBytesAmount = ReadFile(path + filename, body, headers["method"] == "GET");
    if (readBytesAmount != -1) {
        headers["return_code"] = "200 OK";
        headers["Content-Length"] = std::to_string(readBytesAmount);
    } else {
        if (filename == "index.html") {
            headers["return_code"] = "403 Forbidden";
        } else {
            headers["return_code"] = "404 Not_Found";
        }
        headers["Content-Length"] = "0";
    }
    
    output = std::move(input);
}

void PostProcess(map<string, string>& headers, std::shared_ptr<vector<char>> body, HTTPClient& output) {
    HttpResponse response(headers["http_version"],
                          HttpRequest::StringToRequestMethod(headers["method"]),
                          headers["url"],
                          headers["return_code"],
                          (headers["Connection"] == "Keep-Alive"),
                          headers["Content-Length"],
                          body);
    if ((headers["http_version"] == "1.1" && headers["Conection"] != "close") || headers["Connection"] == "Keep-Alive") {
        output.Send(response.GetData(), true);  // will fix later
    } else {
        output.Send(response.GetData(), true);
    }
}
