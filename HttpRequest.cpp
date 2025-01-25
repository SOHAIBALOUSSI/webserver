#include "HttpRequest.hpp"
#include <algorithm>
#include <stdexcept>

HttpRequest::HttpRequest(const std::string &request) {
    std::istringstream iss(request);
    std::string requestLine;
    std::getline(iss, requestLine);
    if (requestLine.empty() || requestLine[requestLine.size() - 1] != '\r')
        throw std::runtime_error("HTTP/1.1 400 Bad HttpRequest\r\nContent-Type: text/plain\r\nContent-Length: 42\r\n\r\nMalformed request line");
    std::istringstream oss(requestLine);
    oss >> method >> url >> version;
    if ((method.empty() || url.empty() || version.empty()))
        throw std::runtime_error("HTTP/1.1 400 Bad HttpRequest\r\nContent-Type: text/plain\r\nContent-Length: 48\r\n\r\nMalformed request line");
    // std::cout << "\"" << method << "\" " <<  "\"" << url << "\" " <<  "\"" << version << "\" " << std::endl; 
    if ((method == "GET" || method == "POST" || method == "DELETE")
        || (version == "HTTP/1.0" || version == "HTTP/1.1"))
    {
        while (std::getline(iss, requestLine)) {
            if (requestLine == "\r" || requestLine.empty()) break;
            std::size_t pos = requestLine.find(':');
            // std::cout << requestLine << std::endl;
            if (pos == std::string::npos)
                throw std::runtime_error("HTTP/1.1 400 Bad HttpRequest\r\nContent-Type: text/plain\r\nContent-Length: 39\r\n\r\nMalformed request header");
            std::string key = requestLine.substr(0, pos);
            std::string value = requestLine.substr(pos + 1);
            key.erase(key.find_last_not_of(" \t\r\n") + 1);
            value.erase(0, value.find_first_not_of(" \t\r\n"));        
            if (value.empty())
                throw std::runtime_error("HTTP/1.1 400 Bad HttpRequest\r\nContent-Type: text/plain\r\nContent-Length: 37\r\n\r\nEmpty header value");        
            headers[toLowerCase(key)] = toLowerCase(value);
        }
        if (method == "POST")
        {
            std::string body;
            while (std::getline(iss, requestLine))
                body += requestLine;
            std::cout << "==>BODY : \n" << body << "\n_________________\n" << std::endl;
            this->body = body;
        }
    }
    else
        throw std::runtime_error("HTTP/1.1 400 Bad HttpRequest\r\nContent-Type: text/plain\r\nContent-Length: 48\r\n\r\nMalformed request line");
}


HttpRequest::HttpRequest() {}

HttpRequest::~HttpRequest() {}
