#pragma once

#include <functional>
#include <vector>
#include <map>
#include <string>
#include "HTTPClient.hpp"

typedef std::function<void(std::map<std::string, std::string>&, std::shared_ptr<std::vector<char>>,
                           HTTPClient&, HTTPClient&)> MainFuncType;
typedef std::function<MainFuncType
                      (std::map<std::string, std::string>&,
                       std::shared_ptr<std::vector<char>>,
                       HTTPClient&)> PreFuncType;
typedef std::function<void(std::map<std::string, std::string>&,
                           std::shared_ptr<std::vector<char>>,
                           HTTPClient&)> PostFuncType;

class Task {
 protected:
    PreFuncType preFunc;
    MainFuncType mainFunc;
    PostFuncType postFunc;

    std::shared_ptr<HTTPClient> input;
    std::shared_ptr<HTTPClient> output;

 public:
    Task();
    explicit Task(HTTPClient& input);
    virtual ~Task() = default;

    PreFuncType GetPreFunc();
    MainFuncType GetMainFunc();
    PostFuncType GetPostFunc();

    void SetPreFunc(PreFuncType preFunc);
    void SetMainFunc(MainFuncType mainFunc);
    void SetPostFunc(PostFuncType postFunc);

    HTTPClient& GetInput();
    HTTPClient& GetOutput();
    void SetInput(HTTPClient& input);
    void SetOutput(HTTPClient& output);

    bool HasData();
};
