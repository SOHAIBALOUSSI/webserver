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
        throw std::runtime_error("ERROR:  creating epoll instance: " + std::string(strerror(errno)));
    addListeningSockets(servers);
    epollListen();
}

void    ServerManager::addListeningSockets(std::vector<Server*>& servers)
{
    std::vector<Server*>::iterator server = servers.begin();
    int i = 0;
    while (server != servers.end())
    {
        std::clog << "INFO: Server number " << i + 1 << '\n';
        int j = 0;
        std::vector<Socket*>::const_iterator socket = (*server)->getSockets().begin();
        while (socket != (*server)->getSockets().end())
        {
            std::clog << "INFO: Socket number " << j + 1 << " for Server number " << i + 1 << ":\n";
            int listeningSocket = (*socket)->getFd();
            socklen_t optval;
            socklen_t optlen = sizeof(optval);
            if (getsockopt(listeningSocket, SOL_SOCKET, SO_ACCEPTCONN, &optval, &optlen) == -1)
                throw std::runtime_error("getsockopt failed: " + std::string(strerror(errno)));
            else 
            {
                if (optval)
                    std::cout << "   -Socket is in listening state.\n";
                else
                    std::cerr << "   -Socket is NOT in listening state.\n";
            }
            setNonBlocking(listeningSocket);
            struct epoll_event event;
            memset(&event, 0, sizeof(event));
            event.events = EPOLLIN | EPOLLET;
            event.data.fd = listeningSocket;
            std::cout << "   -Socket's fd : " << listeningSocket << '\n';
            if (epoll_ctl(epollFd, EPOLL_CTL_ADD, listeningSocket, &event) == -1)
                throw std::runtime_error("ERROR:  adding socket to epoll: " + std::string(strerror(errno)));
            events.push_back(event);
            ++socket;
            j++;
        }
        ++server;
        i++;
    }
}

void    ServerManager::addToEpoll(int clientSocket)
{
    setNonBlocking(clientSocket);
    struct epoll_event event;
    memset(&event, 0, sizeof(event));
    event.events = EPOLLIN | EPOLLOUT;
    event.data.fd = clientSocket;
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, clientSocket, &event) == -1)
        throw std::runtime_error("ERROR: Adding socket to epoll: " + std::string(strerror(errno)));
}

void    ServerManager::epollListen()
{
    while (true)
    {
        int numEvents = epoll_wait(epollFd, events.data(), events.size(), -1);
        if (numEvents == -1)
        {
           std::cerr << "ERROR: in epoll_wait: " << strerror(errno) << std::endl;
           std::cerr << "epollFd: " << epollFd
              << ", events size: " << events.size()
              << ", errno: " << errno << std::endl;
            continue ;
        }
        if (numEvents == events.size())
            events.resize(events.size() * 2);
        for (int i = 0; i < numEvents; i++)
        {
            int currentFd = events[i].data.fd;
            std::clog << "LOG: processing socket N" << currentFd << '\n';
            if (events[i].events & EPOLLIN)
            {
                std::clog << "LOG: Handling incoming connections/requests on socket N" << currentFd << '\n';
                if (isListeningSocket(currentFd))
                    handleConnections(currentFd);
                else
                    handleRequests(currentFd);
            }
            else if (events[i].events & EPOLLOUT)
            {
                std::clog << "LOG: Sending a response to client socket N" << currentFd << '\n'; 
                std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 13\r\n\r\nHello, World!";
                if (send(currentFd, response.c_str(), response.size(), 0) == -1) 
                {
                    std::cerr << "ERROR: Sending data\n";
                    findServerBySocket(currentFd)->closeConnection(currentFd);
                }
            }
            else if (events[i].events & EPOLLERR)
            {
                std::cerr << "ERROR: " << strerror(errno) << "\n";
                findServerBySocket(currentFd)->closeConnection(currentFd);
            }
        }
    }
}

void    ServerManager::handleConnections(int listeningSocket)
{
    std::clog << "LOG: Handling connection on listening socket N" << listeningSocket <<'\n';
    try
    {
        Server* server = findServerBySocket(listeningSocket);
        int clientSocket = server->acceptConnection(listeningSocket);
        if (clientSocket == -1)
            return ;
        addToEpoll(clientSocket);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
}


void    ServerManager::handleRequests(int clientSocket)
{
    std::clog << "LOG: Handling requests on client socket N" << clientSocket <<'\n';
    const size_t buffer_size = 1024;
    char buffer[buffer_size];
    memset(buffer, 0, buffer_size);
    std::string full_request;
    int bytes_received;
    while ((bytes_received = recv(clientSocket, buffer, buffer_size, 0)) > 0)
        full_request.append(buffer, bytes_received);
    std::clog << "LOG: Received " << bytes_received << " bytes from client socket N" << clientSocket
            << ": " << full_request << '\n';
    if (bytes_received == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
    {
        std::cerr << "ERROR: receiving data in client socket N" << clientSocket << "\n";
        findServerBySocket(clientSocket)->closeConnection(clientSocket);
    }
    else if (bytes_received == 0)
    {
        std::cout << "LOG: Client disconnected, closing client socket N" << clientSocket << "\n";
        findServerBySocket(clientSocket)->closeConnection(clientSocket);
        return;
    }
    else if (bytes_received > 0)
    {
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
                std::cerr << "ERROR: sending data to client socket N" << clientSocket << "\n";
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
    throw std::runtime_error("LOG: Socket FD not found in any server: " + std::string(strerror(errno)));
}

void  ServerManager::setNonBlocking(int fd)
{
    if (fd < 0)
        throw std::runtime_error("ERROR: Invalid file descriptor: " + std::to_string(fd));
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        throw std::runtime_error("ERROR: Getting flags for client socket: " + std::string(strerror(errno)));
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK))
        throw std::runtime_error("ERROR: Setting socket to non-blocking: " + std::string(strerror(errno)));
}