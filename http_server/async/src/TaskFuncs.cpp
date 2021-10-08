#include <vector>
#include <map>
#include <string>
#include <fstream>

#include "ports.hpp"

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

int ReadFile(const std::string& filename, std::shared_ptr<vector<char>> body, bool actuallyRead) {
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
    return fileSize;
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

    size_t queryStartPos = headers["url"].find('?');
    size_t finalDotPos = headers["url"].rfind('.', queryStartPos);
    size_t finalSlashPos = headers["url"].rfind('/', finalDotPos);
    std::string filename = headers["url"].substr(finalSlashPos + 1, queryStartPos - finalSlashPos - 1);
    std::string path = std::string(std::getenv("DOCUMENT_ROOT")) + "/" +
        headers["url"].substr(1, finalSlashPos);

    if (filename == "") {
        filename = "index.html";
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
