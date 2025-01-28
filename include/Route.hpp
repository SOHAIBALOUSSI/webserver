#pragma once

#include <bits/stdc++.h>


class Route {
public:
    std::string root; // a directory or a file from where the file should be searched
    std::set<std::string> allowed_methods;
    std::string redirect;
    std::string default_file; // default file to answer if the request is a directory
    bool dir_listing;
    unsigned long long max_body_size;
    std::vector<std::string> cgi_extensions;
    std::string upload_dir;

public:
    Route();
};