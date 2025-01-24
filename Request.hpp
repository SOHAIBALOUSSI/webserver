#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <map>

class Request {
private:
    std::string method, url, version, body;
    std::map<std::string, std::string> headers;

public:
    Request();
    Request(const std::string &request);
    ~Request();

    std::string getMethod() const { return method; }
    std::string getUrl() const { return url; }
    std::string getVersion() const { return version; }
    std::string getBody() const { return body; }
    std::map<std::string, std::string> getHeaders() const { return headers; }
};
