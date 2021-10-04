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
