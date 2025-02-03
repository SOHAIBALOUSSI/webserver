#pragma once

#include <bits/stdc++.h>
#include "HttpRequest.hpp"


class Client {
    private:
        int client_fd;
        HttpRequest request;
        std::string response;

    public:
        Client();
        Client(int client_fd);

        int getFd() const { return client_fd; }
        HttpRequest& getRequest() { return request; }
        std::string getResponse() const { return response; }
};