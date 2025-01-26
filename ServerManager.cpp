#include "ServerManager.hpp"

ServerManager::ServerManager() : epoll_fd(-1), events(100)
{

}

ServerManager::~ServerManager()
{
    if (epoll_fd != -1)
        close(epoll_fd);
}

ServerManager::ServerManager(const std::vector<Config>& _serverPool) : serverPool(_serverPool), epoll_fd(-1)
{
    startServers(serverPool);
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1)
        throw std::runtime_error("Error creating epoll instance: " + std::string(strerror(errno)));
    epollListeningSockets(servers);
    
}

void    ServerManager::epollListeningSockets(std::vector<Server>& servers)
{
    std::vector<Server>::iterator server = servers.begin();
    while (server != servers.end())
    {
        std::vector<Socket>::iterator socket = server->getSockets().begin();
        while (socket != server->getSockets().end())
        {
            struct epoll_event event;
            event.events = EPOLLIN;
            event.data.fd = socket->getFd();
            if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket->getFd(), &event) == -1)
                throw std::runtime_error("Error adding socket to epoll: " + std::string(strerror(errno)));
            ++socket;
        }
        ++server;
    }
}

void    ServerManager::epollListen()
{
    while (true)
    {
        int numEvents = epoll_wait(epoll_fd, events.data(), events.size(), 0);
        if (numEvents == -1)
        {
           std::cerr << "Error in epoll_wait: " << strerror(errno) << std::endl;
            continue ;
        }
        if (numEvents = events.size())
            events.resize(events.size() * 2);
        for (int i = 0; i < events.size(); i++)
        {
            int currentFd = events[i].data.fd;
            if (events[i].events & EPOLLIN)
            {
                if (isListeningSocket(currentFd))
                    //handle incoming connections
                else
                    //handle client request
            }
            else if (events[i].events & EPOLLOUT)
                //handle response
            else if (events[i].events & EPOLLERR)
                //handle errors
        }
    }
}

void  ServerManager::startServers(const std::vector<Config>& _serverPool)
{
    std::vector<Config>::const_iterator it = serverPool.begin();
    while (it != serverPool.end())
    {
        try
        {    
            Server server(*it);
            servers.push_back(server);
            std::vector<Socket>::iterator socket = server.getSockets().begin();
            while (socket != server.getSockets().end())
            {
                listeningSockets[socket->getFd()] = *socket;
                ++socket;
            }
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
        ++it;
    }
}

bool    ServerManager::isListeningSocket(int fd)
{
    return listeningSockets.find(fd) != listeningSockets.end();
}