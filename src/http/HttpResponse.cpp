#include "../../include/HttpResponse.hpp"
#include "../../include/HttpRequest.hpp"

#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>

#define CRLF "\r\n"


std::map<int, std::string> HttpResponse::statusCodesMap = {
    {200, "OK"},
    {201, "Created"},
    {202, "Accepted"},
    {204, "No Content"},
    {301, "Moved Permanently"},
    {302, "Found"},
    {304, "Not Modified"},
    {400, "Bad Request"},
    {401, "Unauthorized"},
    {403, "Forbidden"},
    {404, "Not Found"},
    {405, "Method Not Allowed"},
    {413, "Request Entity Too Large"},
    {414, "URI Too Long"},
    {500, "Internal Server Error"},
    {501, "Not Implemented"},
    {503, "Service Unavailable"},
    {505, "HTTP Version Not Supported"}
};

std::map<std::string, std::string> HttpResponse::mimeTypes = {
    {".html", "text/html"},
    {".htm", "text/html"},
    {".css", "text/css"},
    {".js", "application/javascript"},
    {".json", "application/json"},
    {".xml", "application/xml"},
    {".txt", "text/plain"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".png", "image/png"},
    {".gif", "image/gif"},
    {".bmp", "image/bmp"},
    {".ico", "image/x-icon"},
    {".svg", "image/svg+xml"},
    {".pdf", "application/pdf"},
    {".zip", "application/zip"},
    {".tar", "application/x-tar"},
    {".gz", "application/gzip"},
    {".mp3", "audio/mpeg"},
    {".mp4", "video/mp4"},
    {".mpeg", "video/mpeg"},
    {".avi", "video/x-msvideo"},
    {".csv", "text/csv"},
    {".doc", "application/msword"},
    {".docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
    {".xls", "application/vnd.ms-excel"},
    {".xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
    {".ppt", "application/vnd.ms-powerpoint"},
    {".pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation"}
};

HttpResponse::HttpResponse(Config& conf) : serverConfig(conf) , contentType("text/html"), contentLength(0) {}

std::string getContentType(std::string path) {
    size_t pos = path.find_last_of('.');
    if (pos == std::string::npos) {    
        return "application/octet-stream";
    }
    std::string ext = path.substr(pos);
    std::map<std::string, std::string>::iterator mimeIt; 
    if ((mimeIt = HttpResponse::mimeTypes.find(ext)) != HttpResponse::mimeTypes.end()) {
        return mimeIt->second;
    }
    return "application/octet-stream";
}

unsigned    checkFilePerms(std::string& path) {
    if (access(path.c_str(), F_OK) == 0) {
        if (access(path.c_str(), R_OK) != 0)
            return 403;
        return 200;
    }
    return 404;
}

long getFileContentLength(std::string& path) {
    struct stat fileStat;
    if (stat(path.c_str(), &fileStat) != 0) {
        std::cerr << "ERROR: Cannot access file at " << path << std::endl;
        return -1;
    }
    return fileStat.st_size;
}


std::string getCurrentDateHeader() {
    std::time_t now = std::time(0);
    std::tm* gmt = std::gmtime(&now);
    char buffer[100];
    std::strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", gmt);
    return std::string(buffer);
}

std::string HttpResponse::combineHeaders() {
    std::stringstream ss;
    ss << "HTTP/1.1 " << statusCode << " " << statusCodesMap[statusCode] << CRLF
       << "Date: " << getCurrentDateHeader() << CRLF
       << "Content-Type: " << contentType << CRLF
       << "Content-Length: " << std::to_string(contentLength) << CRLF
       << "Server: EnginX" << CRLF
       << "Connection: " << Connection << CRLF << CRLF;
    return ss.str();
}

std::string getConnetionType(std::map<std::string, std::string>& headers) {
    std::map<std::string, std::string>::iterator it = headers.find("Connection");
    if (it == headers.end()) { return "close"; }
    if (it != headers.end()) {
        return it->second.empty() ? "close" : it->second;
    }
    return "close";
}

void    HttpResponse::prepareHeaders(std::string& path) {
    contentType = getContentType(path);
    contentLength = getFileContentLength(path);
    if (contentLength == -1) { statusCode = 500; }
    responseHeaders = combineHeaders();
}

bool isDirectory(const std::string& path) {
    struct stat statbuf;
    if (stat(path.c_str(), &statbuf) != 0)
        return false;
    return S_ISDIR(statbuf.st_mode);
}


void HttpResponse::generateAutoIndex(std::string& path, HttpRequest& request) {
    std::string autoIndexContent = "<html><head><title>Index of " + path + "</title></head><body>";
    autoIndexContent += "<h1>Index of " + request.getOriginalUri() + "</h1><hr><pre>";
    
    DIR *dir = opendir(path.c_str());
    if (dir) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            std::string name = ent->d_name;
            if (name == ".") continue;
            std::string fullPath = path + name;
            struct stat statbuf;
            if (stat(fullPath.c_str(), &statbuf) == 0) {
                autoIndexContent += "<a href=\"" + name + (S_ISDIR(statbuf.st_mode) ? "/" : "") + "\">" + name + "</a>\n";
            }
        }
        if (-1 == closedir(dir))
            std::cout << "ERROR: Can't close\n";
    }
    
    autoIndexContent += "</pre><hr></body></html>";
    
    statusCode = 200;
    contentType = "text/html";
    contentLength = autoIndexContent.size();
    
    std::stringstream ss;
    ss << "HTTP/1.1 200 OK\r\n"
       << "Date: " << getCurrentDateHeader() << "\r\n"
       << "Content-Type: text/html\r\n"
       << "Content-Length: " << contentLength << "\r\n"
       << "Connection: " << Connection << "\r\n\r\n"
       << autoIndexContent;
    
    responseHeaders = ss.str();
}

void HttpResponse::POST(HttpRequest& request) {
    if (request.getHeaders().count("content-type") && !request.isImplemented(request.getHeaders()["content-type"])){
        statusCode = 501;
        setErrorPage(request.getConfig().getErrorPages());
        return ;
    }
    if (statusCode == 200) {
        statusCode = 201;
        std::stringstream ss;
        ss << "HTTP/1.1 " << statusCode << " " << statusCodesMap[statusCode] << CRLF
            << "Server: EnginX" << CRLF
            << "Connection: " << Connection << CRLF << CRLF;
        responseHeaders = ss.str();

    }
    else {
        setErrorPage(request.getConfig().getErrorPages());
    }
}


void HttpResponse::GET(HttpRequest& request) {
    std::string &path = request.getUriPath();
    unsigned code = checkFilePerms(path);

    if (isDirectory(path)) {
        if (path.back() != '/') {
            statusCode = 301;
            responseHeaders = "HTTP/1.1 301 Moved Permanently\r\nLocation: " + request.getOriginalUri() + "/\r\n\r";
            responseBody.clear();
            return ;
        }
        
        std::set<std::string>& methods = request.getRouteConf().getAllowedMethods();

        if (methods.count(request.getMethod()) == 0) {
            statusCode = 405;
            setErrorPage(request.getConfig().getErrorPages());
            return ;
        }
        std::string defaultFile = request.getDefaultIndex();
        std::string pathToIndex = path + defaultFile;
        if (!defaultFile.empty() && checkFilePerms(pathToIndex) == 200)
        {
            request.setURIpath(pathToIndex);
            prepareHeaders(request.getUriPath());
        }
        else {
            if (request.getautoIndex() == true) {
                generateAutoIndex(path, request);
            }
            else {
                statusCode = 403;
                setErrorPage(request.getConfig().getErrorPages());
            }   
        }
        return ;
    }

    if (code == 200) {
        prepareHeaders(path);
    }
    else {
        setErrorPage(request.getConfig().getErrorPages());
    }    
}


std::string    HttpResponse::generateErrorPage(size_t code) {
    std::stringstream ss;
    ss << "<h1> <center>" << code << " " <<  statusCodesMap[code] << " <center></h1>";
    return ss.str();
}

void    HttpResponse::setErrorPage(std::map<int, std::string>& ErrPages) {

    char buffer[4096];
    std::map<int, std::string>::iterator ErrPage = ErrPages.find(statusCode);
    
    if (ErrPage != ErrPages.end()) {
        std::string errPagePath = ErrPage->second;
        contentType = getContentType(errPagePath);
        contentLength = getFileContentLength(errPagePath);
        if (contentLength == -1) { statusCode = 500; }
        responseHeaders = combineHeaders();
        unsigned code = checkFilePerms(errPagePath);
        if (code == 200) {
            std::ifstream errfile(errPagePath, std::ios::binary);
            errfile.read(buffer, 4096);
            std::streamsize bytesRead = errfile.gcount();
            if (bytesRead > 0) {
                responseBody.append(buffer, bytesRead);
            }
            else if (errfile.eof() && responseBody.empty()) {
                errfile.close();
            }
        }
        else {
            responseBody = generateErrorPage(statusCode);
            contentLength = responseBody.size();
            responseHeaders = combineHeaders();
        }
    }
    else {
        responseBody = generateErrorPage(statusCode);
        contentLength = responseBody.size();    
        responseHeaders = combineHeaders();
    }
    return ;
}


void    HttpResponse::DELETE(HttpRequest& request) {
    if (statusCode == 200) {
        if (isDirectory(requestedContent) || requestedContent.find(request.getUploadDir()) != 0) {
            statusCode = 403;
            setErrorPage(request.getConfig().getErrorPages());
            return ;
        }
        if (remove(requestedContent.c_str()) != 0) {
            statusCode = 500;
            setErrorPage(request.getConfig().getErrorPages());
        }
        else {
            statusCode = 204;
            std::stringstream ss;
            ss << "HTTP/1.1 " << statusCode << " " << statusCodesMap[statusCode] << CRLF
                << "Server: EnginX" << CRLF
                << "Connection: " << Connection << CRLF << CRLF;
            responseHeaders = ss.str();
        }
    }
    else {
        setErrorPage(request.getConfig().getErrorPages());
    }
}

bool HttpResponse::isCgiScript(HttpRequest& request)
{
    const std::string& uriPath = request.getUriPath();
    
    size_t extPos = uriPath.rfind('.');
    if (extPos == std::string::npos || extPos == uriPath.length() - 1)
        return false;
    
    extension = uriPath.substr(extPos);

    for (size_t i = 0; i < extension.size(); ++i) {
        extension[i] = tolower(extension[i]);
    }
    const std::map<std::string, Route>& routes = request.getConfig().getRoutes();
    const std::string& routeKey = request.getRequestrouteKey();
    
    std::map<std::string, Route>::const_iterator routeIt = routes.find(routeKey);
    if (routeIt == routes.end())
        return false;

    const std::vector<std::string>& cgiExtensions = routeIt->second.cgi_extensions;
    for (std::vector<std::string>::const_iterator it = cgiExtensions.begin(); 
         it != cgiExtensions.end(); 
         ++it)
    {
        std::string allowedLower = *it;
        for (size_t i = 0; i < allowedLower.size(); ++i) {
            allowedLower[i] = tolower(allowedLower[i]);
        }
        
        if (extension == allowedLower) {
            return true;
        }
    }
    return false;
}

void HttpResponse::handleCgiScript(HttpRequest &request)
{
    const std::string &uriPath = request.getUriPath();
    const std::string &routeKey = request.getRequestrouteKey();
    const std::map<std::string, Route> &routes = request.getConfig().getRoutes();
    std::map<std::string, Route>::const_iterator routeIt = routes.find(routeKey);

    if (routeIt == routes.end()) {
        statusCode = 404;
        setErrorPage(request.getConfig().getErrorPages());
        return;
    }

    if (access(uriPath.c_str(), F_OK) != 0) {
        statusCode = 404;
        setErrorPage(request.getConfig().getErrorPages());
        return;
    }

    std::string queryString;
    if (!request.getUriQueryParams().empty()) {
        std::map<std::string, std::string>::const_iterator paramIt;
        for (paramIt = request.getUriQueryParams().begin(); paramIt != request.getUriQueryParams().end(); ++paramIt) {
            if (!queryString.empty())
                queryString += "&";
            queryString += paramIt->first + "=" + paramIt->second;
        }
    }

    std::string hostHeader = request.getHeaderValue("host");
    size_t colonPos = hostHeader.find(':');
    std::string portPart = hostHeader.substr(colonPos + 1);

    std::string contentLengthStr = request.getHeaders().count("content-length") 
        ? request.getHeaders()["content-length"] : "0";
    std::string contentTypeStr = request.getHeaders().count("content-type") 
        ? request.getHeaders()["content-type"] : "text/HTML";
    std::string cookieStr = request.getHeaders().count("cookie") 
        ? request.getHeaders()["cookie"] : "";

    std::vector<std::string> envVars = {
        "REQUEST_METHOD=" + request.getMethod(),
        "QUERY_STRING=" + queryString,
        "CONTENT_LENGTH=" + contentLengthStr,
        "CONTENT_TYPE=" + contentTypeStr,
        "SCRIPT_NAME=" + request.getOriginalUri(),
        "SERVER_NAME=" + (!request.getConfig().server_names.empty() 
            ? request.getConfig().server_names[0] : "localhost"),
        "SERVER_PROTOCOL=HTTP/1.1",
        "REMOTE_ADDR=127.0.0.1",
        "GATEWAY_INTERFACE=CGI/1.1",
        "REDIRECT_STATUS=200",
        "SERVER_PORT=" + portPart,
        "SCRIPT_FILENAME=" + uriPath,
        "PATH_INFO=" + request.getOriginalUri(),
        "PATH_TRANSLATED=" + uriPath,
        "HTTP_HOST=" + request.getHeaderValue("host"),
        "HTTP_COOKIE=" + cookieStr
    };

    std::vector<char*> envp;
    for (std::vector<std::string>::const_iterator envIt = envVars.begin(); envIt != envVars.end(); ++envIt) {
        envp.push_back(strdup(envIt->c_str()));
    }
    envp.push_back(nullptr);
    std::string interpreter;
    if (extension == ".php") {
        interpreter = "/usr/bin/php-cgi";
    } else if (extension == ".py") {
        interpreter = "/usr/bin/python3";
    } else if (extension == ".sh") {
        interpreter = "/bin/sh";
    } else {
        statusCode = 500;
        setErrorPage(request.getConfig().getErrorPages());
        for (char* env : envp) free(env);
        return;
    }

    int pipe_in[2];
    int pipe_out[2];
    if (pipe(pipe_in) == -1 || pipe(pipe_out) == -1) {
        statusCode = 500;
        setErrorPage(request.getConfig().getErrorPages());
        for (std::vector<char*>::iterator envIt = envp.begin(); envIt != envp.end(); ++envIt) {
            free(*envIt);
        }
        return;
    }

    pid_t pid = fork();
    if (pid == -1) {
        std::cerr << "Fork failed\n";
        statusCode = 500;
        setErrorPage(request.getConfig().getErrorPages());
        for (char* env : envp) free(env);
        return;
    }

    if (pid == 0) {
        close(pipe_in[1]);
        dup2(pipe_in[0], STDIN_FILENO);
        close(pipe_in[0]);
        close(pipe_out[0]);
        dup2(pipe_out[1], STDOUT_FILENO);
        close(pipe_out[1]);
        char* argv[] = {const_cast<char*>(interpreter.c_str()), 
                        const_cast<char*>(uriPath.c_str()), nullptr};
        execve(interpreter.c_str(), argv, envp.data());
        std::cerr << "execve failed\n";
        exit(1);
    }

    close(pipe_in[0]);
    close(pipe_out[1]);

    if (!request.getBody().empty()) {
        std::string bodyStr(request.getBody().begin(), request.getBody().end());
        ssize_t bytesWritten = write(pipe_in[1], request.getBody().data(), request.getBody().size());
        if (bytesWritten < 0) {
            std::cerr << "Failed to write body\n";
        }
    }
    close(pipe_in[1]);

    char buffer[READ_BUFFER_SIZE];
    std::string cgiOutput;
    ssize_t bytesRead;
    while ((bytesRead = read(pipe_out[0], buffer, READ_BUFFER_SIZE - 1)) > 0) {
        buffer[bytesRead] = '\0';
        cgiOutput += buffer;
    }
    if (bytesRead < 0) {
        std::cerr << "Read from CGI failed \n";
    }
    close(pipe_out[0]);
    int status;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        size_t headerEnd = cgiOutput.find("\r\n\r\n");
        if (headerEnd == std::string::npos) {
            headerEnd = cgiOutput.find("\n\n");
        }
        if (headerEnd == std::string::npos) {
            std::cerr << "Malformed CGI output: no header-body separator\n";
            statusCode = 500;
            setErrorPage(request.getConfig().getErrorPages());
            return;
        }

        std::string cgiHeaders = cgiOutput.substr(0, headerEnd);
        std::string cgiBody = cgiOutput.substr(headerEnd + (cgiOutput[headerEnd] == '\r' ? 4 : 2));

        statusCode = 200;
        contentType = "text/html";
        contentLength = cgiBody.size();
        Date = getCurrentDateHeader();
        Connection = getConnetionType(request.getHeaders());

        std::vector<std::string> setCookieHeaders;
        std::istringstream headerStream(cgiHeaders);
        std::string line;
        while (std::getline(headerStream, line) && !line.empty() && line != "\r") {
            size_t colonPos = line.find(':');
            if (colonPos != std::string::npos) {
                std::string key = line.substr(0, colonPos);
                std::string tempValue = line.substr(colonPos + 1);
                std::string value = strTrim(tempValue);
                if (key == "Status") {
                    statusCode = std::stoi(value.substr(0, 3));
                } else if (key == "content-type") {
                    contentType = value;
                } else if (key == "Set-Cookie") {
                    setCookieHeaders.push_back("Set-Cookie: " + value + "\r\n");
                }
            }
        }
        responseHeaders = "HTTP/1.1 " + std::to_string(statusCode) + " " + statusCodesMap[statusCode] + "\r\n" +
                          "Date: " + Date + "\r\n" +
                          "Content-Type: " + contentType + "\r\n" +
                          "Content-Length: " + std::to_string(contentLength) + "\r\n" +
                          "Server: EnginX\r\n" +
                          "Connection: " + Connection + "\r\n";
        std::vector<std::string>::const_iterator cookieIt = setCookieHeaders.begin();
        for (; cookieIt != setCookieHeaders.end(); ++cookieIt) {
            responseHeaders += *cookieIt;
        }
        responseHeaders += "\r\n";
        responseBody = cgiBody;
    } else {
        std::cerr << "CGI execution failed with status: " << WEXITSTATUS(status) << "\n";
        statusCode = 500;
        setErrorPage(request.getConfig().getErrorPages());
    }
}

void  HttpResponse::generateResponse(HttpRequest& request) {
    statusCode = request.getStatusCode();
    
    Config& conf = request.getConfig();
    requestedContent = request.getUriPath();
    Connection = request.getHeaderValue("Connection") == "close" ? "close" : "keep-alive";
    if (statusCode >= 400) {
        return setErrorPage(conf.getErrorPages());
    }
    
    std::string& method = request.getMethod();
    if (isCgiScript(request)) {
      handleCgiScript(request);
    } else if (method == "GET")
        GET(request);
    else if (method == "POST")
        POST(request);
    else if (method == "DELETE")
        DELETE(request);
}


void    HttpResponse::reset() {
    requestedContent.clear();
    statusCode = 200;
    contentLength = 0;
    contentType = "text/html";
    responseHeaders.clear();
    responseBody.clear();
    Date.clear();
    Connection.clear();
}