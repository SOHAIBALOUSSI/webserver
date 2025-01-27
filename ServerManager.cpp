#include "ServerManager.hpp"

ServerManager::ServerManager() : epollFd(-1), events(100)
{

}

ServerManager::~ServerManager()
{
    if (epollFd != -1)
    {
        close(epollFd);
        epollFd = -1;
    }
}

ServerManager::ServerManager(const std::vector<Config>& _serverPool) : serverPool(_serverPool), epollFd(-1), events(100)
{
    startServers(serverPool);
    epollFd = epoll_create1(0);
    if (epollFd == -1)
        throw std::runtime_error("Error creating epoll instance: " + std::string(strerror(errno)));
    addListeningSockets(servers);
    epollListen();
}

void    ServerManager::addListeningSockets(std::vector<Server*>& servers)
{
    std::vector<Server*>::iterator server = servers.begin();
    int i = 0;
    while (server != servers.end())
    {
        std::clog << "LOG: Server number " << i + 1 << '\n';
        int j = 0;
        std::vector<Socket*>::const_iterator socket = (*server)->getSockets().begin();
        while (socket != (*server)->getSockets().end())
        {
            std::clog << "LOG: Socket number " << j + 1 << " for Server number " << i + 1 << '\n';
            int listeningSocket = (*socket)->getFd();
            socklen_t optval;
            socklen_t optlen = sizeof(optval);
            if (getsockopt(listeningSocket, SOL_SOCKET, SO_ACCEPTCONN, &optval, &optlen) == -1)
                throw std::runtime_error("getsockopt failed: " + std::string(strerror(errno)));
            else 
            {
                if (optval)
                    std::cout << "Socket is in listening state.\n";
                else
                    std::cerr << "Socket is NOT in listening state.\n";
            }
            struct epoll_event event;
            memset(&event, 0, sizeof(event));
            event.events = EPOLLIN;
            event.data.fd = listeningSocket;
            std::cout << "SOCKET'S FD : " << listeningSocket << '\n';
            if (epoll_ctl(epollFd, EPOLL_CTL_ADD, listeningSocket, &event) == -1)
                throw std::runtime_error("Error adding socket to epoll1: " + std::string(strerror(errno)));
            // events.push_back(event);
            ++socket;
            j++;
        }
        ++server;
        i++;
    }
}

void    ServerManager::addToEpoll(int clientSocket)
{
    if (clientSocket < 0)
        throw std::runtime_error("Invalid file descriptor: " + std::to_string(clientSocket));
    int flags = fcntl(clientSocket, F_GETFL, 0);
    if (flags == -1)
        throw std::runtime_error("Error getting flags for client socket: " + std::string(strerror(errno)));
    if (fcntl(clientSocket, F_SETFL, flags | O_NONBLOCK))
        throw std::runtime_error("Error setting socket to non-blocking: " + std::string(strerror(errno)));
    struct epoll_event event;
    memset(&event, 0, sizeof(event));
    event.events = EPOLLIN | EPOLLOUT;
    event.data.fd = clientSocket;
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, clientSocket, &event) == -1)
        throw std::runtime_error("Error adding socket to epoll2: " + std::string(strerror(errno)));
}

void    ServerManager::epollListen()
{
    while (true)
    {
        int numEvents = epoll_wait(epollFd, events.data(), events.size(), 0);
        if (numEvents == -1)
        {
           std::cerr << "Error in epoll_wait: " << strerror(errno) << std::endl;
           std::cerr << "epfd: " << epollFd
              << ", events size: " << events.size()
              << ", errno: " << errno << std::endl;
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
                std::cout << "APPAH\n";
            }
            else if (events[i].events & EPOLLOUT)
            {
                std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 13\r\n\r\nHello, World!";
                if (send(currentFd, response.c_str(), response.size(), 0) == -1) 
                {
                    std::cerr << "Error sending data\n";
                    findServerBySocket(currentFd)->closeConnection(currentFd);
                }
            }
            else if (events[i].events & EPOLLERR)
            {
                std::cerr << "ERROR\n";
            }
        }
    }
}

void    ServerManager::handleConnections(int listeningSocket)
{
    try
    {
        Server* server = findServerBySocket(listeningSocket);
        int clientSocket = server->acceptConnection(listeningSocket);
        addToEpoll(clientSocket);
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
    while (true)
    {
        bytes_received = recv(clientSocket, buffer, buffer_size, 0);
        if (bytes_received == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
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
        full_request.append(buffer, bytes_received);
        buffer[bytes_received] = '\0';
        std::string response;
        try
        {
            HttpRequest request(buffer);
            requests.push(request);
        }
        catch (const std::exception& e)
        {
            response = e.what();
            if (send(clientSocket, response.c_str(), response.size(), 0) == -1)
            {
                std::cerr << "Error sending data\n";
                findServerBySocket(clientSocket)->closeConnection(clientSocket);
            }
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
            Server* server = new Server(*it);
            servers.push_back(server);
            std::vector<Socket*>::const_iterator socket = server->getSockets().begin();
            while (socket != server->getSockets().end())
            {
                listeningSockets[(*socket)->getFd()] = (*socket);
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
        std::vector<Socket*>::const_iterator socket = (*server)->getSockets().begin();
        while (socket != (*server)->getSockets().end())
        {
            if ((*socket)->getFd() == fd)
                return (*server);
        }
        ++server;
    }
    throw std::runtime_error("Socket FD not found in any server: " + std::string(strerror(errno)));
}