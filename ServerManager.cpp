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
    addListeningSockets(servers);
    epollListen();
}

void    ServerManager::addListeningSockets(std::vector<Server*>& servers)
{
    std::vector<Server*>::iterator server = servers.begin();
    while (server != servers.end())
    {
        std::vector<Socket>::const_iterator socket = (*server)->getSockets().begin();
        while (socket != (*server)->getSockets().end())
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

void    ServerManager::addToEpoll(int clientSocket)
{
    int flags = fcntl(clientSocket, F_GETFL, 0);
    if (flags == -1)
        throw std::runtime_error("Error getting flags for client socket: " + std::string(strerror(errno)));
    if (fcntl(clientSocket, F_SETFL, flags | O_NONBLOCK))
        throw std::runtime_error("Error setting socket to non-blocking: " + std::string(strerror(errno)));
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = clientSocket;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, clientSocket, &event) == -1)
        throw std::runtime_error("Error adding socket to epoll: " + std::string(strerror(errno)));
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
        if (numEvents == events.size())
            events.resize(events.size() * 2);
        for (int i = 0; i < numEvents; i++)
        {
            int currentFd = events[i].data.fd;
            if (events[i].events & EPOLLIN)
            {
                if (isListeningSocket(currentFd))
                    handleConnections(currentFd);
                else
                    handleRequests(currentFd);
            }
            // else if (events[i].events & EPOLLOUT)
            //     //handle response
            // else if (events[i].events & EPOLLERR)
            //     //handle errors
        }
    }
}

void    ServerManager::handleConnections(int listeningSocket)
{
    try
    {
        Server* server = findServerBySocket(listeningSocket);
        addToEpoll(server->acceptConnection(listeningSocket));
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
}


void    ServerManager::handleRequests(int clientSocket)
{
    const size_t buffer_size = 1024;
    char buffer[buffer_size];
    memset(buffer, 0, buffer_size);
    std::string full_request;
    int bytes_received;
    while ((bytes_received = recv(clientSocket, buffer, buffer_size, 0)) > 0)
        full_request.append(buffer, bytes_received);
    if (bytes_received == -1)
    {
        std::cerr << "Error receiving data\n";
        findServerBySocket(clientSocket)->closeConnection(clientSocket);
    }
    else if (bytes_received == 0)
    {
        std::cout << "Client disconnected\n";
        findServerBySocket(clientSocket)->closeConnection(clientSocket);
        return;
    }
    buffer[bytes_received] = '\0';
    std::string response;
    try
    {
        HttpRequest request(buffer);
        requests.push(request);
        response = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, World!";
    }
    catch (const std::exception& e)
    {
        response = e.what();
    }
    if (send(clientSocket, response.c_str(), response.size(), 0) == -1)
    {
        std::cerr << "Error sending data\n";
        findServerBySocket(clientSocket)->closeConnection(clientSocket);
    }
}

void  ServerManager::startServers(const std::vector<Config>& _serverPool)
{
    std::vector<Config>::const_iterator it = serverPool.begin();
    while (it != serverPool.end())
    {
        try
        {    
            Server* server = new Server(*it);
            servers.push_back(server);
            std::vector<Socket>::const_iterator socket = server->getSockets().begin();
            while (socket != server->getSockets().end())
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

Server* ServerManager::findServerBySocket(int fd)
{
    std::vector<Server*>::iterator server = servers.begin();
    while (server != servers.end())
    {
        std::vector<Socket>::const_iterator socket = (*server)->getSockets().begin();
        while (socket != (*server)->getSockets().end())
        {
            if (socket->getFd() == fd)
            return (*server);
        }
        ++server;
    }
    throw std::runtime_error("Socket FD not found in any server: " + std::string(strerror(errno)));
}