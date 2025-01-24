#pragma once

#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <cstdio>
#include <netinet/in.h>
#include <cstdlib>
#include "Request.hpp"
#include <poll.h>
#include <algorithm>
#include <vector>
#include <fcntl.h>


class Server
{
    private:
        int server_fd;
        sockaddr_in server_addr;
        std::vector<int> client_fds;
        int port;
        std::string host;
        //need config file class here
        
        void setupServer();
        void handleConnections();
        void acceptConnection();
        void closeConnection(int client_fd);
        void handleRequest(int client_fd);

    public:
        Server(std::string host, int port);
        ~Server();

        void start();
        void shutdownServer();
};
