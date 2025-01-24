#include "Request.hpp"
#include <algorithm>
#include <stdexcept>

Request::Request(const std::string &request) {
    std::istringstream iss(request);
    std::string requestLine;
    std::getline(iss, requestLine);
    if (requestLine.empty() || requestLine[requestLine.size() - 1] != '\r')
        throw std::runtime_error("HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 42\r\n\r\nMalformed request line\r\n");
    std::istringstream oss(requestLine);
    oss >> method >> url >> version;
    if (method.empty() || url.empty() || version.empty()) {
        throw std::runtime_error("HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 48\r\n\r\nMalformed request line\r\n");
    }
    while (std::getline(iss, requestLine)) {
        if (requestLine == "\r" || requestLine.empty()) break;
        std::size_t pos = requestLine.find(':');
        std::cout << "request header : " << requestLine << std::endl;
        if (pos == std::string::npos)
            throw std::runtime_error("HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 39\r\n\r\nMalformed request header\r\n");
        std::string key = requestLine.substr(0, pos);
        std::string value = requestLine.substr(pos + 1);
        key.erase(key.find_last_not_of(" \t\r\n") + 1);
        value.erase(0, value.find_first_not_of(" \t\r\n"));        
        if (value.empty())
            throw std::runtime_error("HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 37\r\n\r\nEmpty header value\r\n");        
        headers[key] = value;
    }
    std::string body;
    while (std::getline(iss, requestLine))
        body += requestLine;
    this->body = body;
}


Request::Request() {}

Request::~Request() {}
