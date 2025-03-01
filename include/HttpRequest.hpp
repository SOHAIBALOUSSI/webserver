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

enum parsingState
{
    REQUESTLINE,
    HEADERS,
    BODY,
    COMPLETE
};

class HttpRequest 
{
    private:
        const uint8_t* _buffer;
        size_t _pos, _bufferLen, bodyStart;
        Config  configs;
        std::string defaultIndex;
        bool        autoIndex;
        unsigned    statusCode;
        bool    fileCreated;
        std::string outfilename;
        int     _currentChunkSize;
        unsigned long long _totalBodysize;
        size_t _currentChunkBytesRead;  
        std::string method, uri, uriPath, version;
        std::vector<uint8_t> body;
        std::vector<uint8_t> request;
        std::string originalUri;
        std::map<std::string, std::string> uriQueryParams;
        std::map<std::string, std::string> headers;
        std::string  RequestrouteKey;
        bool    isChunked;
        Route   routeConf;
        //request-line parsing
        size_t    parseRequestLine();
        void    validateMethod();
        void    validateVersion();
        void    RouteURI();
        bool    keepAlive;
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
        
        std::vector<uint8_t> readLine();

    public:

        parsingState state;
        void    validateURI();
        HttpRequest();
        HttpRequest(const Config& _configs);
        HttpRequest(const std::string &request);
        ~HttpRequest();


        std::string getRequestrouteKey();
        void    setStatusCode(long code) { statusCode = code; }
        std::string getDefaultIndex() { return defaultIndex; }
        long  getStatusCode() { return statusCode; }
        std::string getOriginalUri() { return originalUri; }

        std::string     getHeaderValue(std::string key);
        std::string& getMethod() { return method; }
        std::string& getURI() { return uri; }
        std::string& getVersion() { return version; }
        std::vector<uint8_t>& getBody() { return body; }
        std::map<std::string, std::string>& getHeaders() { return headers; }
        std::string& getUriPath() { return uriPath; }
        std::map<std::string, std::string>& getUriQueryParams() { return uriQueryParams; }
        parsingState& getState() { return state; }
        Config& getConfig() { return configs; }
        bool     getautoIndex () {return autoIndex; }
        Route&  getRouteConf() {return routeConf; }
        void    setURI(const std::string& _uri) { uri = _uri; }
        void    setURIpath(const std::string& _uripath) { uriPath = _uripath; }
        void  setBodyStartPos(size_t value) { bodyStart = value; }
        std::vector<uint8_t>& getRequestBuffer() { return request; }
        
        //main parsing
        size_t    parse(const uint8_t *buffer, size_t bufferLen);
        std::string getLineAsString(const std::vector<uint8_t>& line);      
        std::string getUploadDir();
        bool isImplemented(std::string str);
        bool isFileCreated() {return fileCreated;}
        void    reset();
        std::string getoutfilename() {return outfilename;}
};
