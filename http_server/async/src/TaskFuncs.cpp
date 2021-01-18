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
#include "TemplateManager.hpp"
#include "TaskFuncs.hpp"

using std::string;
using std::map;
using std::vector;


MainFuncType PreProcess(map<string, string>& headers, vector<char>& body, HTTPClient& input) {
    while (!input.ReceivedHeader()) {
        input.RecvHeaderAsync();
    }

    int bodySize = 0;
    headers.clear();
    if (input.GetPort() == FROM_DB_PORT) {  // port FROM_DB_PORT is reserved for db's responses
        HttpResponseReader response(input.GetHeader());
        headers = response.GetAllHeaders();
        headers["return_code"] = response.GetReturnCode();
        bodySize = response.GetContentLength();

    } else {
        HttpRequest request(input.GetHeader());
        headers = request.GetAllHeaders();
        headers["url"] = request.GetURL();
        headers["method"] = request.GetRequestMethodString();
        headers["http_version"] = request.GetHTTPVersion();
        bodySize = request.GetContentLength();
    }

    body.clear();
    if (bodySize > 0) {
        while (!input.RecvBodyAsync(bodySize));
        body = input.GetBody();
    }

    if (input.GetPort() == FROM_DB_PORT)
        return MainProcessDBReceived;
    else if (input.GetPort() == TO_DB_PORT)
        return MainProcessDBServer;
    else
        return MainProcessBasic;
}

void MainProcessBasic(map<string, string>& headers, vector<char>& body,
                      map<int, HTTPClient>& pendingDBResponse,
                      std::shared_ptr<std::mutex> pendingDBResponseMutex,
                      HTTPClient& input, HTTPClient& output) {
    ContentType type = HttpResponse::GetContentType(headers["url"]);
    if (type == TXT_HTML) {
        TemplateManager templateManager(headers["url"]);
        std::set<string> params = templateManager.GetParameterNames();
        if (params.empty()) {
            body = templateManager.GetHtmlFinal(map<string, string>());

            output = std::move(input);

        } else {
            pendingDBResponseMutex->lock();
            pendingDBResponse.insert(std::pair<int, HTTPClient&>(input.GetSd(), input));
            pendingDBResponseMutex->unlock();

            headers["Connection"] = "close";  // maybe make headers from scratch???

            body.clear();
            string sdString = std::to_string(input.GetSd());
            body.insert(body.end(), sdString.begin(), sdString.end());
            body.push_back('|');
            vector<char> paramsPart = HTTPClient::MergeSetToVector(params);
            body.insert(body.end(), paramsPart.begin(), paramsPart.end());

            output = HTTPClient("localhost", TO_DB_PORT);
            headers["proxy"] = "true";
        }

    } else {
        std::ifstream source("../static" + headers["url"], std::ios::binary);
        if (source) {  // file was opened successfully
            char buffer[BUF_SIZE] = {0};
            while (source.read(buffer, BUF_SIZE))
                body.insert(body.end(), buffer, buffer + BUF_SIZE);

            body.insert(body.end(), buffer, buffer + source.gcount());
            headers["return_code"] = "200 OK";
        } else {
            headers["return_code"] = "404 Not Found";
        }
        source.close();

        output = std::move(input);
    }
}

static map<string, string> ProcessTemplatesInDB(const std::set<string>& params, size_t ID);
static map<string, string> ProcessTemplatesInDBIndex(const std::set<string> &params);

void MainProcessDBServer(map<string, string>& headers, vector<char>& body,
                         map<int, HTTPClient>& pendingDBResponse,
                         std::shared_ptr<std::mutex> pendingDBResponseMutex,
                         HTTPClient& input, HTTPClient& output) {
    auto firstSepPos = std::find(body.begin(), body.end(), '|');
    if (firstSepPos > body.end()) {
        firstSepPos = body.end();
    }

    TemplateManager templateManager(headers["url"]);
    size_t id = 0;
    size_t find = headers["url"].find("watch?");
    if (find != string::npos) {
        try {
            id = std::stoi(headers["url"].substr(find+6));
        }
        catch (std::exception&) {
            id = 0;
        }
    }

    map<string, string> params;
    if (headers["url"] == "/")
        params = ProcessTemplatesInDBIndex(templateManager.GetParameterNames());
    else if (headers["url"].find("watch?") != string::npos)
        params = ProcessTemplatesInDB(templateManager.GetParameterNames(), id);
    
    params["sd"] = string(body.begin(), firstSepPos);
    body = HTTPClient::MergeMapToVector(params);

    headers["return_code"] = "200 OK";

    output = HTTPClient("localhost", FROM_DB_PORT);
}

void MainProcessDBReceived(map<string, string>& headers, vector<char>& body,
                           map<int, HTTPClient>& pendingDBResponse,
                           std::shared_ptr<std::mutex> pendingDBResponseMutex,
                           HTTPClient& input, HTTPClient& output) {
    map<string, string> bodyParams = HTTPClient::SplitVectorToMap(body);

    int sd = std::stoi(bodyParams.at("sd"));
    bodyParams.erase("sd");

    input.Close();

    pendingDBResponseMutex->lock();
    output = pendingDBResponse.at(sd);
    pendingDBResponse.erase(sd);
    pendingDBResponseMutex->unlock();

    HttpRequest initialRequest(output.GetHeader());
    map<string, string> newHeaders = initialRequest.GetAllHeaders();
    newHeaders["url"] = initialRequest.GetURL();
    newHeaders["method"] = initialRequest.GetRequestMethodString();
    newHeaders["http_version"] = initialRequest.GetHTTPVersion();
    newHeaders["return_code"] = headers["return_code"];

    headers = std::move(newHeaders);

    TemplateManager templateManager(headers["url"]);
    body = templateManager.GetHtmlFinal(bodyParams);
}

void PostProcess(map<string, string>& headers, vector<char>& body, HTTPClient& output) {
    if (headers["proxy"] == "true") {  // Meaning, we need to call another server
        HttpRequestCreator request(headers["http_version"],
                                   HttpRequestCreator::StringToRequestMethod(headers["method"]),
                                   headers["url"],
                                   (headers["Connection"] == "Keep-Alive"),
                                   body);

        if ((headers["http_version"] == "1.1" && headers["Conection"] != "close") || headers["Connection"] == "Keep-Alive")
            output.Send(request.GetRequest(), true);  // will fix later
        else
            output.Send(request.GetRequest(), true);

    } else {
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
}

//  {[movietittle,moviedescription,starphoto,starname,movielogo,moviename,videolink,recommended,tittles]}
static map<string, string> ProcessTemplatesInDB(const std::set<string>& params, size_t ID) {
    map<string, string> result_map;

    string connection_string("host=localhost port=5432 dbname=db_woosh user=basic password=123");
    pqxx::connection con(connection_string.c_str());

    pqxx::work wrk(con);
    //pqxx::result res = wrk.exec("SELECT m_id,title,description,rating FROM MOVIES WHERE (m_id between "+std::to_string(ID)+" and "+std::to_string(ID+1)+")");

    pqxx::result number = wrk.exec("SELECT COUNT(m_id) FROM MOVIES");
    if (ID >= std::stoul(number[0][0].as<string>()))
        ID = 0;

    // SELECT for movies-data
    pqxx::result res = wrk.exec("SELECT m_id,title,description,rating,poster,video_link FROM MOVIES WHERE "
                                "(m_id = " + std::to_string(ID) + ")");
    result_map["movietittle"] = res[0][1].as<string>();
    result_map["moviename"] = res[0][1].as<string>();   // redundant
    result_map["moviedescription"] = res[0][2].as<string>();
    result_map["movierating"] = res[0][3].as<string>();
    result_map["movielogo"] = res[0][4].as<string>();
    result_map["videolink"] = res[0][5].as<string>();


    // SELECT for actors
    // Need to figure many to many request for finding actors with m_id == ID from table actors-movies.
    res = wrk.exec("SELECT a_id,name,photo FROM ACTORS WHERE (a_id = "+std::to_string(ID)+")");
    result_map["recommended"] = "6";  // change it in for cycle too
    result_map["starname"] = res[0][1].as<string>();
    result_map["starphoto"] = res[0][2].as<string>();  // replace


    // SELECT for actors (many to many)
    res = wrk.exec("SELECT a_id,name FROM actors WHERE a_id in (Select a_id From actors_movies Where m_id =" + std::to_string(ID)+")");
    size_t n = 0;
    for (const auto &row: res) {
        result_map["stars"+std::to_string(n)] = std::to_string(n);
        result_map["actor"+std::to_string(n)] = row[0].as<string>();
        result_map["actorname"+std::to_string(n)] = row[1].as<string>();
        ++n;
    }
    result_map["stars"] = std::to_string(n);


    // SELECT for recommended      // crutch (see ID+6) and for +3
    res = wrk.exec("SELECT m_id,title,description,rating FROM MOVIES WHERE (m_id != "+std::to_string(ID)+") ORDER BY RANDOM() LIMIT 10");
    for (size_t i = 0; i < 9; ++i)
        result_map["recommended" + std::to_string(i)] =  res[i][0].as<string>();

    res = wrk.exec("SELECT title FROM MOVIES");
    for (size_t i = 0; i < 10; ++i)
        result_map["title" + std::to_string(i)] =  res[i][0].as<string>();

    return result_map;
}

static map<string, string> ProcessTemplatesInDBIndex(const std::set<string> &params) {
    map<string, string> result_map;

    string connection_string("host=localhost port=5432 dbname=db_woosh user=basic password=123");
    pqxx::connection con(connection_string.c_str());

    pqxx::work wrk(con);

    pqxx::result number = wrk.exec("SELECT COUNT(m_id) FROM MOVIES");
    size_t number_of_movies = std::stoul(number[0][0].as<string>());
    pqxx::result res;

    result_map["best"] = "10";
    res = wrk.exec("SELECT title FROM MOVIES");
    for (size_t i = 0; i < number_of_movies; ++i)
        result_map["title" + std::to_string(i)] =  res[i][0].as<string>();

    res = wrk.exec("SELECT m_id,title,description,rating FROM MOVIES ORDER BY RANDOM() LIMIT 10");
    for (size_t i = 0; i < number_of_movies; ++i)
        result_map["best" + std::to_string(i)] =  res[i][0].as<string>();

    return result_map;
}
