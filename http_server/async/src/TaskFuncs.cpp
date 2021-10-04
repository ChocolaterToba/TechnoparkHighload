#include <vector>
#include <map>
#include <string>
#include <fstream>
#include <pqxx/pqxx>

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

void MainProcessBasic(map<string, string>& headers, vector<char>& body,
                      HTTPClient& input, HTTPClient& output) {
    // ADD MAIN LOGIC HERE



    // ContentType type = HttpResponse::GetContentType(headers["url"]);
    // if (type == TXT_HTML) {
    //     TemplateManager templateManager(headers["url"]);
    //     std::set<string> params = templateManager.GetParameterNames();
    //     if (params.empty()) {
    //         body = templateManager.GetHtmlFinal(map<string, string>());

    //         output = std::move(input);

    //     } else {
    //         pendingDBResponseMutex->lock();
    //         pendingDBResponse.insert(std::pair<int, HTTPClient&>(input.GetSd(), input));
    //         pendingDBResponseMutex->unlock();

    //         headers["Connection"] = "close";  // maybe make headers from scratch???

    //         body.clear();
    //         string sdString = std::to_string(input.GetSd());
    //         body.insert(body.end(), sdString.begin(), sdString.end());
    //         body.push_back('|');
    //         vector<char> paramsPart = HTTPClient::MergeSetToVector(params);
    //         body.insert(body.end(), paramsPart.begin(), paramsPart.end());

    //         output = HTTPClient("localhost", TO_DB_PORT);
    //         headers["proxy"] = "true";
    //     }

    // } else {
    //     std::ifstream source("../static" + headers["url"], std::ios::binary);
    //     if (source) {  // file was opened successfully
    //         char buffer[BUF_SIZE] = {0};
    //         while (source.read(buffer, BUF_SIZE))
    //             body.insert(body.end(), buffer, buffer + BUF_SIZE);

    //         body.insert(body.end(), buffer, buffer + source.gcount());
    //         headers["return_code"] = "200 OK";
    //     } else {
    //         headers["return_code"] = "404 Not Found";
    //     }
    //     source.close();

    //     output = std::move(input);
    // }
    headers["return_code"] = "200 OK";
    output = std::move(input);  // just return what was sent
}

void PostProcess(map<string, string>& headers, vector<char>& body, HTTPClient& output) {
    HttpResponse response(headers["http_version"],
                         HttpRequest::StringToRequestMethod(headers["method"]),
                         headers["return_code"],
                         (headers["Connection"] == "Keep-Alive"),
                         body);
    if ((headers["http_version"] == "1.1" && headers["Conection"] != "close") || headers["Connection"] == "Keep-Alive")
        output.Send(response.GetData(), true);  // will fix later
    else
        output.Send(response.GetData(), true);
}
