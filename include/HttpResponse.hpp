#pragma once

#include <bits/stdc++.h>
#include "HttpRequest.hpp"
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define READ_BUFFER_SIZE 1024000

class HttpResponse
{

private:
    Config     serverConfig;
    long        statusCode;
    std::string responseHeaders;
    std::string responseBody;
    std::string requestedContent;
    std::string contentType;
    long      contentLength;
    std::string Date;
    std::string servername;
    std::string Connection;
    std::string extension;
    void generateStatusCodes();
    void generateMimeTypes();

public:
    HttpResponse(Config& conf);

    void           generateAutoIndex(std::string& path, HttpRequest& request);
    std::string    generateErrorPage(size_t code);
    void           setErrorPage(std::map<int, std::string>& ErrPages);
    void           generateResponse(HttpRequest& request);
    void           prepareHeaders(std::string& path);
    void           setResponseStatusCode(unsigned code) { statusCode = code; }
    std::string    combineHeaders();

    //cgi
    bool        isCgiScript(HttpRequest& request);
    void        handleCgiScript(HttpRequest &request);
    
    void    GET(HttpRequest& request);
    void    POST(HttpRequest& request);
    void    DELETE(HttpRequest& request);

    void    reset();
    // getters
    
    long    getStatuscode() { return statusCode; }
    std::string& getResponseHeaders() { return responseHeaders; }
    std::string& getResponseBody() { return responseBody;}
    static std::map<int, std::string> statusCodesMap;
    static std::map<std::string, std::string> mimeTypes;
};

unsigned    checkFilePerms(std::string& path);