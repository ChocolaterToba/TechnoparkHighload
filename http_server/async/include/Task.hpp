#pragma once

#include <functional>
#include <map>
#include <vector>
#include "HTTPClient.hpp"

typedef std::function<void(std::string&, std::vector<char>&, HTTPClient&)> MainFuncType;
typedef std::function<MainFuncType
                      (std::string&, std::vector<char>&, HTTPClient&)> PreFuncType;
typedef std::function<void(std::string&, std::vector<char>&, HTTPClient&)> PostFuncType;

class Task {
 protected:
    PreFuncType preFunc;
    MainFuncType mainFunc;
    PostFuncType postFunc;

    // If we use something other that HTTP
    // as a protocol (like for DB connection)
    // then HTTPClient needs to be replaced with Client
    // which will be client abstraction.
    std::shared_ptr<HTTPClient> input;
    std::shared_ptr<HTTPClient> output;

 public:
    Task();
    explicit Task(HTTPClient& input);
    virtual ~Task() = default;

    virtual PreFuncType GetPreFunc();
    virtual MainFuncType GetMainFunc();
    virtual PostFuncType GetPostFunc();

    virtual void SetPreFunc(PreFuncType preFunc);
    virtual void SetMainFunc(MainFuncType mainFunc);
    virtual void SetPostFunc(PostFuncType postFunc);

    virtual HTTPClient& GetInput();
    virtual HTTPClient& GetOutput();
    virtual void SetInput(HTTPClient& input);
    virtual void SetOutput(HTTPClient& output);

    virtual bool HasData();
};
