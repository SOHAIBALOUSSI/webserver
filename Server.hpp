#pragma once

#include "HttpRequest.hpp"
#include "Config.hpp"
#include "Socket.hpp"

class Server
{
    private:
        int server_fd;
        sockaddr_in server_addr;
        int port;
        std::vector<int> clientSockets;
        std::vector<Socket> listeningSockets;
        std::string host;
        Config serverConfig;
        
        void setupServer();
        void handleConnections();
        void handleHttpRequest(int client_fd);

    public:
        Server(std::string host, int port);
        Server(const Config& serverConfig);
        ~Server();

        int acceptConnection(int listeningSocket);
        void closeConnection(int client_fd);

        const std::vector<Socket>& getSockets( void ) const;
        void start();
        void shutdownServer();
};
