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
        void acceptConnection();
        void closeConnection(int client_fd);
        void handleHttpRequest(int client_fd);

    public:
        Server(std::string host, int port);
        Server(const Config& serverConfig);
        ~Server();


        std::vector<Socket> getSockets( void ) const;
        void start();
        void shutdownServer();
};
