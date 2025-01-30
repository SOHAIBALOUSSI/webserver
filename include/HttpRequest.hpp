#pragma once

#include "Common.h"
#include "Config.hpp"

class HttpIncompleteRequest : public std::exception
{
    virtual const char * what() const throw() {return "Need more data to complete request";}
};

class HttpRequestError : public std::exception 
{
    private:
        std::string _msg;
    public:
        HttpRequestError(const std::string& msg) : _msg(msg) {}
        virtual const char* what() const throw() { return _msg.c_str(); }
};

class HttpRequest 
{
    private:
        const char* _buffer;
        size_t _pos, _bufferLen;
        Config  configs;
        enum parsingState
        {
            REQUESTLINE,
            HEADERS,
            BODY,
            COMPLETE
        };
        parsingState state;

        std::string method, uri, version, body;
        std::unordered_map<std::string, std::string> headers;

        size_t    parse(const char* buffer, size_t bufferLen);
        
        size_t    parseRequestLine();
        void    validateMethod();

        void    validateURI();
        std::pair<std::string, std::string> splitToPathAndQuery(const std::string& uri);
        bool    isAbsoluteURI();
        bool    isURIchar(char c);
        std::string decodeAndNormalize(std::string& path);
        std::string decode(std::string& encoded);
        std::string normalize(std::string& decoded);
        
        void    validateVersion();

        size_t    parseHeaders();
        size_t    parseBody();
        size_t    parseChunkedBody();
        
        std::string readLine();

    public:
        HttpRequest();
        HttpRequest(const Config& _configs);
        HttpRequest(const std::string &request);
        ~HttpRequest();

        std::string getMethod() const { return method; }
        std::string getUrl() const { return uri; }
        std::string getVersion() const { return version; }
        std::string getBody() const { return body; }
        std::unordered_map<std::string, std::string> getHeaders() const { return headers; }
};
