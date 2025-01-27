#pragma once

#include "Common.h"
#include "Server.hpp"


class ServerManager
{
    private :
        std::vector<Config> serverPool;
        std::vector<Server*> servers;
        std::map<int, Socket*> listeningSockets;
        std::vector<struct epoll_event> events;
        std::queue<HttpRequest> requests;
        int epollFd;

        void	startServers(const std::vector<Config>& _serverPool);
        void    addListeningSockets(std::vector<Server*>& servers);
        bool    isListeningSocket(int fd);
        void    epollListen();
        void    addToEpoll(int clientsocket);

        Server* findServerBySocket(int fd);

        void    handleConnections(int listeningSocket);
        void    handleRequests(int clientSocket);

        void    setNonBlocking(int fd);
    
    public  :
        ServerManager();
        ServerManager(const std::vector<Config>& _serverPool);
        ~ServerManager();

};