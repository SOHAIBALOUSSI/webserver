#pragma once

#include "common.h"

class HttpRequest {
private:
    std::string method, url, version, body;
    std::map<std::string, std::string> headers;

public:
    HttpRequest();
    HttpRequest(const std::string &request);
    ~HttpRequest();

    std::string getMethod() const { return method; }
    std::string getUrl() const { return url; }
    std::string getVersion() const { return version; }
    std::string getBody() const { return body; }
    std::map<std::string, std::string> getHeaders() const { return headers; }
};
