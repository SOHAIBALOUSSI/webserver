#pragma once

#include <bits/stdc++.h>


class Route {
public:
    std::string root;
    std::set<std::string> allowed_methods;
    std::string redirectUri;
    std::string redirectStatusCode;
    std::string default_file; 
    bool dir_listing;
    unsigned long long max_body_size;
    std::vector<std::string> cgi_extensions;
    std::string upload_dir;

public:
    Route();
    std::string& getRoot() { return root; }
    std::string& getDefaultFile() { return default_file; }
    bool        getAutoIndexState () {return dir_listing; }
    std::set<std::string>& getAllowedMethods() { return allowed_methods; }

};