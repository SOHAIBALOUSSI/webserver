#include "../../include/ServerManager.hpp"

ServerManager::ServerManager() : epollFd(-1), events(100) {}

ServerManager::~ServerManager()
{
    if (epollFd != -1)
    {
        close(epollFd);
        epollFd = -1;
    }
}

bool    ServerManager::isListeningSocket(int fd)
{
    return listeningSockets.find(fd) != listeningSockets.end();
}

Server* ServerManager::findServerBySocket(int fd)
{
    if (fd < 0)
        return (NULL);
    for (int i = 0; i < servers.size(); i++)
    {
        if (isListeningSocket(fd))
        {
            const std::vector<Socket*>& listeningSockets = servers[i]->getListeningSockets();
            for (int j = 0; j < listeningSockets.size(); j++)
            {
                if (listeningSockets[j]->getFd() == fd)
                    return (servers[i]);
            }
        }
        else
        {
            const std::vector<int>& clientSockets = servers[i]->getClientSockets();        
            for (int k = 0; k < clientSockets.size(); k++)
            {
                if (clientSockets[k] == fd)
                    return (servers[i]);
            }
        }
    }
    return (NULL);
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


void    ServerManager::addListeningSockets(std::vector<Server*>& servers)
{
    std::vector<Server*>::iterator server = servers.begin();
    int i = 0;
    while (server != servers.end())
    {
        std::clog << "INFO: Server number " << i + 1 << '\n';
        int j = 0;
        std::vector<Socket*>::const_iterator socket = (*server)->getListeningSockets().begin();
        while (socket != (*server)->getListeningSockets().end())
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
            event.events = EPOLLIN;
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
    event.events = EPOLLIN | EPOLLOUT | EPOLLET;
    event.data.fd = clientSocket;
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, clientSocket, &event) == -1)
        throw std::runtime_error("ERROR: Adding socket to epoll: " + std::string(strerror(errno)));
}

void ServerManager::closeConnection(int fd) {
    Server* server = findServerBySocket(fd);
    if (server) {
        server->closeConnection(fd);
        epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, NULL);
    }
}

void ServerManager::modifyEpollEvent(int fd, uint32_t events) {
    struct epoll_event event;
    event.events = events;
    event.data.fd = fd;
    if (epoll_ctl(epollFd, EPOLL_CTL_MOD, fd, &event) == -1) {
        closeConnection(fd);
    }
}

void ServerManager::sendErrorResponse(int clientSocket, const std::string& error) {
    if (send(clientSocket, error.c_str(), error.size(), 0) == -1) {
        std::cerr << "ERROR: sending error response to client socket N" << clientSocket << "\n";
        closeConnection(clientSocket);
    }
}

void ServerManager::sendResponse(int clientSocket) {
    std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 5\r\n\r\npain!";
    if (send(clientSocket, response.c_str(), response.size(), 0) == -1) {
        std::cerr << "ERROR: Sending data\n";
        closeConnection(clientSocket);
    } else {
        modifyEpollEvent(clientSocket, EPOLLIN);
    }
}


void    ServerManager::handleConnections(int listeningSocket)
{
    try {
        Server* server = findServerBySocket(listeningSocket);
        int clientFD = server->acceptConnection(listeningSocket);
        if (clientFD == -1)
            return ;
        addToEpoll(clientFD);
        Clients.insert(std::pair<int, Client>(clientFD, Client(clientFD)));
    }
    catch(const std::exception& e) {
        std::cerr << e.what() << '\n';
    }
}


std::string ServerManager::readRequest(int clientSocket) {
    // request 7atha f client.request; muhim chuf kidir hhh
    const size_t bufferSize = 8096;
    char buffer[bufferSize];
    std::string request;
    int bytesReceived;
    while ((bytesReceived = recv(clientSocket, buffer, bufferSize, 0)) > 0) {
        request.append(buffer, bytesReceived);
    }
    if (bytesReceived == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
        std::cerr << "ERROR: receiving data in client socket N" << clientSocket << "\n";
        closeConnection(clientSocket);
    } else if (bytesReceived == 0) {
        closeConnection(clientSocket);
    }
    return request;
}

// here where u should parse the request
void ServerManager::handleRequest(int clientSocket) {
    std::map<int, Client>::iterator Client = Clients.find(clientSocket);

    if (Client != Clients.end())
    {
        // hna l3b kima bghiti
        std::string request = readRequest(Client->second.getFd());
        // process request
        if (!request.empty()) {
            try {
                HttpRequest httpRequest(request);
                requests.push(httpRequest);
                modifyEpollEvent(clientSocket, EPOLLOUT);
            } catch (const std::exception& e) {
                sendErrorResponse(clientSocket, e.what());
            }
        }
    }
    // just skip if client not found.

}


void ServerManager::handleEvent(const epoll_event& event) {
    int fd = event.data.fd;
    if (event.events & EPOLLIN) { // ready to recv
        if (isListeningSocket(fd)) { 
            handleConnections(fd); // accept Client Connection
        }else{
            handleRequest(fd); // tkalef a m3ayzo
        }
    }
    else if (event.events & EPOLLOUT) { // ready to send 
        sendResponse(fd); 
    }
    else if (event.events & (EPOLLERR | EPOLLRDHUP | EPOLLHUP)) {
        std::cerr << "ERROR: " << strerror(errno) << "\n";
        closeConnection(fd);
    }
}

void    ServerManager::eventsLoop() // events Loop (main loop)
{
    while (1) {
        int eventsNum = epoll_wait(epollFd, events.data(), events.size(), -1);
        if (eventsNum == -1){
            std::cerr << "ERROR: in epoll_wait: " << strerror(errno) << std::endl;
            continue ;
        }
        if (eventsNum == events.size())
            events.resize(events.size() * 2);

        for (int i = 0; i < eventsNum; i++){
            handleEvent(events[i]);
        }
    }
}

void  ServerManager::initServers()
{
    std::vector<Config>::const_iterator it = serverPool.begin();
    while (it != serverPool.end())
    {
        try {    
            Server* server = new Server(*it);
            std::clog << "Setting & starting up server :\n   -host: " << it->getHost() << "\n";
            std::set<int>::const_iterator port =  it->getPorts().begin();
            while (port != it->getPorts().end()) {
                std::clog << "   -port: " << *port;
                ++port;
            }
            std::clog << std::endl;
            servers.push_back(server);
            std::vector<Socket*>::const_iterator socket = server->getListeningSockets().begin();
            while (socket != server->getListeningSockets().end()) {
                listeningSockets[(*socket)->getFd()] = (*socket);
                ++socket;
            }
        }
        catch(const std::exception& e) {
            std::cerr << e.what() << '\n';
        }
        ++it;
    }
}
void ServerManager::initEpoll() {
    epollFd = epoll_create1(0);
    if (epollFd == -1) {
        throw std::runtime_error("ERROR: creating epoll instance: " + std::string(strerror(errno)));
    }
    addListeningSockets(servers);
}

ServerManager::ServerManager(const std::vector<Config>& _serverPool) : serverPool(_serverPool), epollFd(-1), events(100)
{
    initServers();
    initEpoll();
    eventsLoop();
}