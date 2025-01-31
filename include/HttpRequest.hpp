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

        std::string method, uri, uriPath, version, body;
        std::map<std::string, std::string> uriQueryParams;
        std::unordered_map<std::string, std::string> headers;

        //main parsing
        size_t    parse(const char* buffer, size_t bufferLen);

        //request-line parsing
        size_t    parseRequestLine();
        void    validateMethod();
        void    validateURI();
        void    validateVersion();

        //URI parsing
        std::pair<std::string, std::string> splitKeyValue(const std::string& uri, char delim);
        bool    isAbsoluteURI();
        bool    isURIchar(char c);
        std::string decodeAndNormalize();
        std::string decode(std::string& encoded);
        std::string normalize(std::string& decoded);
        std::map<std::string, std::string> decodeAndParseQuery(std::string& query);
        
        
        //headers parsing
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
        std::string getURI() const { return uri; }
        std::string getVersion() const { return version; }
        std::string getBody() const { return body; }
        std::unordered_map<std::string, std::string> getHeaders() const { return headers; }
        std::string getUriPath() const { return uriPath; }
        std::map<std::string, std::string> getUriQueryParams() const { return uriQueryParams; }
        parsingState getState() const { return state; }
};
