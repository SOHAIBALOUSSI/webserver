#pragma once

#include "HttpRequest.hpp"
#include "Config.hpp"

class Server
{
    private:
        int server_fd;
        sockaddr_in server_addr;
        std::vector<int> client_fds;
        int port;
        std::string host;
        std::vector<Config> serverPool;
        
        void setupServer();
        void handleConnections();
        void acceptConnection();
        void closeConnection(int client_fd);
        void handleHttpRequest(int client_fd);

    public:
        Server(std::string host, int port);
        ~Server();

        void start();
        void shutdownServer();
};
