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


MainFuncType PreProcess(map<string, string>& headers, vector<char>& body, HTTPClient& input) {
    HttpRequest request(input.GetHeader());
    headers = request.GetAllHeaders();
    headers["url"] = request.GetURL();
    headers["method"] = request.GetRequestMethodString();
    headers["http_version"] = request.GetHTTPVersion();

    body = input.GetBody();

    return MainProcessBasic;
}

bool ReadFile(const std::string& filename, vector<char>& body, bool actuallyRead) {
    std::ifstream file;
    file.open(filename, std::ios::in | std::ios :: binary);

    if (file.eof() || file.fail()) {
        return false;
    }

    if (!actuallyRead) {
        return true;
    }

    file.seekg(0, std::ios_base::end);
    std::streampos fileSize = file.tellg();
    body.resize(fileSize);

    file.seekg(0, std::ios_base::beg);
    file.read(&body[0], fileSize);
    return true;
}

void MainProcessBasic(map<string, string>& headers, vector<char>& body,
                      HTTPClient& input, HTTPClient& output) {
    if (headers["method"] != "GET" && headers["method"] != "HEAD") {
        headers["return_code"] = "405 Method Not Allowed";
        output = std::move(input);
        return;
    }

    int finalSlashPos = headers["url"].rfind('/');
    std::string filename(headers["url"].c_str() + finalSlashPos + 1);
    std::string path = std::string(std::getenv("DOCUMENT_ROOT")) +
        std::string(headers["url"].c_str() + 1, finalSlashPos);

    if (filename == "") {
        filename = "index.html";
    }

    bool fileRead = ReadFile(path + filename, body, headers["method"] == "GET");
    if (fileRead) {
        headers["return_code"] = "200 OK";
    } else {
        headers["return_code"] = "404 Not_Found";
    }
    
    output = std::move(input);
}

void PostProcess(map<string, string>& headers, vector<char>& body, HTTPClient& output) {
    HttpResponse response(headers["http_version"],
                          HttpRequest::StringToRequestMethod(headers["method"]),
                          headers["url"],
                          headers["return_code"],
                          (headers["Connection"] == "Keep-Alive"),
                          body);
    if ((headers["http_version"] == "1.1" && headers["Conection"] != "close") || headers["Connection"] == "Keep-Alive") {
        output.Send(response.GetData(), true);  // will fix later
    } else {
        output.Send(response.GetData(), true);
    }
}
