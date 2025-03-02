#pragma once

#include <bits/stdc++.h>
#include "Route.hpp"
#include "Common.h"

class Config
{
public:
    std::set<int> ports;
    std::string host;
    std::set<std::string> allowed_methods;
    unsigned long long max_body_size; // for post request
    std::vector<std::string> server_names;
    std::map<int, std::string> error_pages;
    std::map<std::string, Route> routes; 
    std::map<std::string, std::string> redirLoopDetector;
public:
    std::string config_content;
    Config();
    void    parse_config(std::string confing_content);

    void    insertPort(std::string value);
    void    insertHost(std::string value);
    void    insertAllowedMethods(std::string value);
    void    insertMaxBodySize(std::string value);
    void    insertServerNames(std::string value);
    void    insertErrorPages(std::string value);
    void    insertRoute(std::string value);

    bool    validateRedirCode(std::string statusCode) {
        if (statusCode == "300" || statusCode == "301" 
            ||statusCode == "302" || statusCode == "303" 
            || statusCode == "304" || statusCode == "307"
            || statusCode == "308")
            return true;
        return false;
    }
    const std::string& getHost( void )  const;
    const std::set<int>& getPorts( void ) const;
    std::map<std::string, Route>& getRoutes() { return routes; }
    std::map<int, std::string>& getErrorPages() { return error_pages; }
    std::set<std::string>& getAllowedMethods() { return allowed_methods; }
};

std::string readConfigFile(std::string path);

