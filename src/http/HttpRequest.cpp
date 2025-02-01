#include "../../include/HttpRequest.hpp"
#include <algorithm>
#include <stdexcept>

HttpRequest::HttpRequest(const std::string &request)
{
    std::istringstream iss(request);
    std::string requestLine;
    std::getline(iss, requestLine);
    if (requestLine.empty() || requestLine[requestLine.size() - 1] != '\r')
        throw std::runtime_error("HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 21\r\n\r\nMalformed request line");
    std::istringstream oss(requestLine);
    oss >> method >> uri >> version;
    if ((method.empty() || uri.empty() || version.empty()))
        throw std::runtime_error("HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 21\r\n\r\nMalformed request line");
    std::cout << "LOG: received request:\n      \"" << method << "\" " <<  "\"" << uri << "\" " <<  "\"" << version << "\" " << std::endl; 
    if ((method == "GET" || method == "POST" || method == "DELETE")
        && (version == "HTTP/1.0" || version == "HTTP/1.1"))
    {
        while (std::getline(iss, requestLine)) {
            if (requestLine == "\r" || requestLine.empty()) break;
            std::size_t pos = requestLine.find(':');
            std::cout << "      " << requestLine << std::endl;
            if (pos == std::string::npos)
                throw std::runtime_error("HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 25\r\n\r\nMalformed request header");
            std::string key = requestLine.substr(0, pos);
            std::string value = requestLine.substr(pos + 1);
            key.erase(key.find_last_not_of(" \t\r\n") + 1);
            value.erase(0, value.find_first_not_of(" \t\r\n"));        
            if (value.empty())
                throw std::runtime_error("HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 19\r\n\r\nEmpty header value");
            headers[toLowerCase(key)] = value;
        }
        if (method == "POST")
        {
            // size_t contentLen = stoi(headers["content-length"]);
            std::string body;
            while (std::getline(iss, requestLine))
                body += requestLine.substr(0, requestLine.find_last_not_of("\r") + 1);
            std::cout << "==>BODY : \n" << body << "\n_________________\n" << std::endl;
            // if (body.size() == contentLen)
            this->body = body;
            // throw std::runtime_error("HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 21\r\n\r\nMalformed request line");
        }
    }
    else
        throw std::runtime_error("HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 21\r\n\r\nMalformed request line");
}


HttpRequest::HttpRequest() {}

HttpRequest::~HttpRequest() {}

HttpRequest::HttpRequest(const Config& _configs) : state(REQUESTLINE), _pos(0), _bufferLen(0), configs(_configs) {}

size_t    HttpRequest::parse(const char* _buffer, size_t bufferLen)
{
    this->_bufferLen = bufferLen;
    size_t bytesReceived = 0;
    try
    {
        switch (state)
        {
            case REQUESTLINE : 
                bytesReceived = parseRequestLine(); 
                break ;
            case HEADERS : 
                bytesReceived = parseHeaders();
                break ;
            case BODY : 
                bytesReceived = parseBody(); 
                break ;
            case COMPLETE : 
                return 0;
        }
    }
    catch (const HttpIncompleteRequest&)
    {
        return 0;
    }
    return (bytesReceived);
}

// METHOD SP URI SP VERSION CRLF
size_t    HttpRequest::parseRequestLine()
{
    std::string line = readLine();
    size_t firstSpace = line.find(' ');
    size_t secondSpace = line.find(' ', firstSpace + 1);
    if (firstSpace == std::string::npos || secondSpace == std::string::npos)
        throw (HttpRequestError("HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 22\r\n\r\nMalformed request line")); 
    if (line.find(' ', secondSpace + 1) != std::string::npos)
        throw (HttpRequestError("HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 22\r\n\r\nMalformed request line")); 
    method = line.substr(0, firstSpace);
    uri = line.substr(firstSpace + 1, secondSpace - firstSpace - 1);
    version = line.substr(secondSpace + 1);
    validateMethod();
    validateURI();
    validateVersion();
    state = HEADERS;
    return (line.size() + 2);
}

void    HttpRequest::validateMethod()
{
    if (method != "GET" && method != "DELETE" && method != "POST")
        throw (HttpRequestError("HTTP/1.1 405 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 19\r\n\r\nMethod Not Allowed"));
}

void    HttpRequest::validateURI()
{
    if (uri.empty())
        throw (HttpRequestError("HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 7\r\n\r\nBad URI"));
    std::pair<std::string, std::string> pathAndQuery = splitKeyValue(uri, '?');
    uriPath = pathAndQuery.first;
    std::string query = pathAndQuery.second;
    if (!isAbsoluteURI())
    {
        if (uriPath[0] != '/')
            throw (HttpRequestError("HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 7\r\n\r\nBad URI"));
    }
    else
    {
        size_t schemeEnd = uriPath.find('/');
        if (schemeEnd != std::string::npos)
            uriPath = uriPath.substr(schemeEnd + 2);
        else
            uriPath = "/";
    }
    std::cout << "Before decoding and normalizing: " << uriPath << "\n";
    uriPath = decodeAndNormalize();
    std::cout << "After decoding and normalizing: " << uriPath << "\n";
    if (!query.empty())
        uriQueryParams = decodeAndParseQuery(query);
}

std::map<std::string, std::string> HttpRequest::decodeAndParseQuery(std::string& query)
{
    std::istringstream iss(query);
    std::map<std::string, std::string> queryParams;
    std::string queryParam, key, value;
    while (std::getline(iss, queryParam, '&'))
    {
        size_t pos = queryParam.find('=');
        key = queryParam.substr(0, pos);
        value = (pos != std::string::npos ? queryParam.substr(pos + 1) : "");
        if (value.find('#') != std::string::npos)
            value.erase(value.find('#'));
        queryParams[key] = value;
    }
    return (queryParams);
}

std::string HttpRequest::decode(std::string& encoded)
{
    std::string decoded;
    for (size_t i = 0; i < encoded.size(); i++)
    {
        if (encoded[i] == '%')
        {
            if (i + 2 >= encoded.size())
                throw (HttpRequestError("HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 24\r\n\r\nInvalid percent-encoding"));
            if (!isHexDigit(encoded[i + 1]) || !isHexDigit(encoded[i + 1]))                
                throw (HttpRequestError("HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 24\r\n\r\nInvalid percent-encoding"));
            decoded += hexToValue(encoded[i + 1]) * 16 + hexToValue(encoded[i + 2]);
            i += 2;
        }
        else if (!isURIchar(encoded[i]))
            throw (HttpRequestError("HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 21\r\n\r\nInvalid URI character"));
        else
            decoded += encoded[i];
    }
    return (decoded);
}

std::string HttpRequest::normalize(std::string& decoded)
{
    std::istringstream iss(decoded);
    std::string segment;
    std::vector<std::string> normalizedSegments;
    std::string normalized;
    while (std::getline(iss, segment, '/'))
    {
        if (segment == ".") continue ;
        else if (segment == "..")
        {
            if (!normalizedSegments.empty()) 
                normalizedSegments.pop_back();
        }
        else 
            normalizedSegments.push_back(segment);
    }
    for (size_t i = 0; i < normalizedSegments.size(); i++)
    {
        if (normalizedSegments[i].empty()) continue;
        normalized += "/" + normalizedSegments[i];
    }
    if (normalized.empty())
        return ("/");
    return (normalized);
}

std::string HttpRequest::decodeAndNormalize()
{
    std::istringstream iss(uriPath);
    std::string segment;
    std::string decodedAndNormalized;
    while (std::getline(iss, segment, '/'))
        decodedAndNormalized += "/" + decode(segment);
    return (normalize(decodedAndNormalized));
}

bool    HttpRequest::isAbsoluteURI()
{
    return (uri.find("http://") == 0);
}

bool    HttpRequest::isURIchar(char c)
{
    const std::string allowed = "!$&'()*+,-./:;=@_~";
    return (std::isalnum(c) || allowed.find(c) != std::string::npos);
}

void    HttpRequest::validateVersion()
{
    if (version.empty())
        throw (HttpRequestError("HTTP/1.1 505 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 21\r\n\r\nVersion not supported"));
}

//header-field   = field-name ":" OWS field-value OWS
size_t    HttpRequest::parseHeaders()
{
    size_t startPos = _pos;
    std::string line = readLine();
    while (!line.empty())
    {
        std::pair<std::string, std::string> keyValue = splitKeyValue(line, ':');
        std::string key = toLowerCase(keyValue.first);
        std::string value = strTrim(keyValue.second);
        if (key.empty() || key.find_first_of(" \t") != std::string::npos || key.find_last_of(" \t") != std::string::npos)
            throw (HttpRequestError("HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 20\r\n\r\nMalformed Field Name"));
        if (headers.find(key) != headers.end())
            headers[key] += "," + value;
        else
            headers[key] = value;
        line = readLine();
    }
    state = BODY;
    return (_pos - startPos);
}

bool    HttpRequest::validateValue(std::string& value)
{
    const std::string allowed = "!#$%&\'\"*+-.^_`|~";
    return (value.find_first_of(allowed) != std::string::npos);
}

//curl -H "Tranfert-Encoding: chunked" -F "filename=/path" 127.0.0.1:8080 (for testing POST)
size_t    HttpRequest::parseBody()
{
    state = COMPLETE;

}

size_t    HttpRequest::parseChunkedBody()
{
    
}

std::string HttpRequest::readLine()
{
    size_t start = _pos;
    while (_pos < _bufferLen)
    {
        if (_buffer[_pos] == '\r' && _pos + 1 < _bufferLen && _buffer[_pos + 1] == '\n')
        {
            std::string line(_buffer + start, _pos - start);
            _pos += 2;
            return (line);
        }
        _pos++;
    }
    throw (HttpIncompleteRequest());
}

std::pair<std::string, std::string> HttpRequest::splitKeyValue(const std::string& uri, char delim)
{
    size_t queryStart = uri.find(delim);
    std::string path = uri.substr(0, queryStart);
    std::string query = (queryStart != std::string::npos ? uri.substr(queryStart + 1) : "");
    return (std::make_pair(path, query));
}



//UNIT TESTING URI PARSING
int main(int ac, char **av)
{
    std::cout << "______________________________________________________________________________________\n";
    try 
    {
        HttpRequest req;
        req.setURI("////path/to/resource?name=John%20Doe&age=30");
        req.validateURI();
        std::cout << "URI path : " << req.getUriPath() << std::endl;
        for (auto queryParam : req.getUriQueryParams())
            std::cout << "key: " << queryParam.first << ", value: " << queryParam.second << std::endl; 
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl << std::endl;
    }
    std::cout << "______________________________________________________________________________________\n";
    try 
    {
        HttpRequest req;
        req.setURI("/../../../path/to/resource?name=John%20Doe&age=30");
        req.validateURI();
        std::cout << "URI path : " << req.getUriPath() << std::endl;
        for (auto queryParam : req.getUriQueryParams())
            std::cout << "key: " << queryParam.first << ", value: " << queryParam.second << std::endl; 
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl << std::endl;
    }
    std::cout << "______________________________________________________________________________________\n";
    try 
    {
        HttpRequest req;
        req.setURI("/path/to/resource?name=John%20Doe&age=30");
        req.validateURI();
        std::cout << "URI path : " << req.getUriPath() << std::endl;
        for (auto queryParam : req.getUriQueryParams())
            std::cout << "key: " << queryParam.first << ", value: " << queryParam.second << std::endl; 
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl << std::endl;
    }
    std::cout << "______________________________________________________________________________________\n";
    try 
    {
        HttpRequest req;
        req.setURI("http://user:password@localhost:8080/path/to/resource?name=John%20Doe&age=30#nose");
        req.validateURI();
        std::cout << "URI path : " << req.getUriPath() << std::endl;
        for (auto queryParam : req.getUriQueryParams())
            std::cout << "key: " << queryParam.first << ", value: " << queryParam.second << std::endl; 
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl << std::endl;
    }
    std::cout << "______________________________________________________________________________________\n";
    try 
    {
        HttpRequest req;
        req.setURI("..");
        req.validateURI();
        std::cout << "URI path : " << req.getUriPath() << std::endl;
        for (auto queryParam : req.getUriQueryParams())
            std::cout << "key: " << queryParam.first << ", value: " << queryParam.second << std::endl; 
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl << std::endl;
    }
    std::cout << "______________________________________________________________________________________\n";
    try 
    {
        HttpRequest req;
        req.setURI("/../../etc/passwd");
        req.validateURI();
        std::cout << "URI path : " << req.getUriPath() << std::endl;
        for (auto queryParam : req.getUriQueryParams())
            std::cout << "key: " << queryParam.first << ", value: " << queryParam.second << std::endl; 
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl << std::endl;
    }
    std::cout << "______________________________________________________________________________________\n";
}