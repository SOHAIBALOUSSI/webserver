#pragma once

#include "Common.h"
#include "Server.hpp"

class ServerManager
{
    private :
        std::vector<Config> serverPool;
        std::vector<Server> servers;
        std::map<int, Socket> listeningSockets;
        std::vector<struct epoll_event> events;
        int epoll_fd;

        void	startServers(const std::vector<Config>& _serverPool);
        void    epollListeningSockets(std::vector<Server>& servers);
        bool    isListeningSocket(int fd);
        void    epollListen();
    
    public  :
        ServerManager();
        ~ServerManager();
        ServerManager(const std::vector<Config>& _serverPool);

};