#include "../../include/ServerManager.hpp"
#include "../../include/HttpResponse.hpp"

static ServerManager *g_manager = NULL;

ServerManager::ServerManager() : events(), epollFd(-1) {}

void    ServerManager::shutDownManager()
{
    if (epollFd != -1)
    {
        close(epollFd);
        epollFd = -1;
    }
    for (size_t i = 0; i < servers.size(); i++)
        delete servers[i];
    servers.clear();
    events.clear();
    Clients.clear();
    listeningSockets.clear();
    serverPool.clear();
}

ServerManager::~ServerManager()
{
    std::clog << INFO << "INFO: Shutting down Cluster\n" << RESET;
    shutDownManager();
}

void    ServerManager::handleSignal(int sig)
{
    (void) sig;

    if (g_manager)
        g_manager->shutDownManager();
    exit(0);
}

void    ServerManager::handleSignals()
{
    g_manager = this;
    signal(SIGINT, handleSignal);
    signal(SIGTERM, handleSignal);
    signal(SIGQUIT, handleSignal);
}

bool    ServerManager::isListeningSocket(int fd)
{
    return listeningSockets.find(fd) != listeningSockets.end();
}

Server* ServerManager::findServerBySocket(int fd)
{
    if (fd < 0)
        return (NULL);
    for (size_t i = 0; i < servers.size(); i++)
    {
        if (isListeningSocket(fd))
        {
            const std::vector<Socket*>& listeningSockets = servers[i]->getListeningSockets();
            for (size_t j = 0; j < listeningSockets.size(); j++)
            {
                if (listeningSockets[j]->getFd() == fd)
                    return (servers[i]);
            }
        }
        else
        {
            const std::vector<int>& clientSockets = servers[i]->getClientSockets();     
            std::find(clientSockets.begin(), clientSockets.end(), fd);  
            for (size_t k = 0; k < clientSockets.size(); k++)
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
        throw std::runtime_error(ERROR + timeStamp() + "ERROR: Invalid file descriptor: " + toString(fd) + std::string(RESET));
    if (fcntl(fd, F_SETFL, O_NONBLOCK, O_CLOEXEC)) 
        throw std::runtime_error(ERROR + timeStamp() + "ERROR: Setting socket to non-blocking: " + std::string(strerror(errno)) + std::string(RESET));
}


void    ServerManager::addListeningSockets(std::vector<Server*>& servers)
{
    for (size_t i = 0; i < servers.size(); i++)
    {
        std::vector<Socket*> sockets = servers[i]->getListeningSockets();
        for (size_t j = 0; j < sockets.size(); j++)
        {
            int listeningSocket = sockets[j]->getFd();
            setNonBlocking(listeningSocket);
            struct epoll_event event;
            memset(&event, 0, sizeof(event));
            event.events = EPOLLIN;
            event.data.fd = listeningSocket;
            if (epoll_ctl(epollFd, EPOLL_CTL_ADD, listeningSocket, &event) == -1)
                throw std::runtime_error(ERROR + timeStamp() + "ERROR:  adding socket to epoll: " + std::string(strerror(errno)) + std::string(RESET));
            events.push_back(event);
        }
    }
}

void    ServerManager::addToEpoll(int clientSocket)
{
    setNonBlocking(clientSocket);
    struct epoll_event event;
    memset(&event, 0, sizeof(event));
    event.events = EPOLLIN;
    event.data.fd = clientSocket;
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, clientSocket, &event) == -1)
        throw std::runtime_error(ERROR + timeStamp() + "ERROR: Adding socket to epoll: " + std::string(strerror(errno)) + std::string(RESET));
}

void ServerManager::closeConnection(int fd) {
    Server* server = findServerBySocket(fd);
    if (server) {
        epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, NULL);
        server->closeConnection(fd);
        Clients.erase(fd);
    }
}

void ServerManager::modifyEpollEvent(int fd, uint32_t events) {
    struct epoll_event event;
    memset(&event, 0, sizeof(event));
    event.events = events;
    event.data.fd = fd;
    if (epoll_ctl(epollFd, EPOLL_CTL_MOD, fd, &event) == -1) {
        closeConnection(fd);
    }
}

void ELOG(std::string emsg) {
    std::cerr << emsg << std::endl; 
}

void ServerManager::LOG(long statusCode, HttpRequest& request) {
    std::string statusColor = CYAN;

    if (statusCode >= 400) { statusColor = RED; }
    else if (statusCode >= 300) { statusColor = BLUE; }
    else if (statusCode >= 200) { statusColor = GREEN; }
    
    std::cout << statusColor << timeStamp() 
                << statusColor << request.getMethod() << " " 
                << statusColor << request.getURI() << " " 
                << statusColor << statusCode << " " 
                << HttpResponse::statusCodesMap[statusCode] << RESET << std::endl;
}

void ServerManager::sendResponse(int clientSocket) {
    std::map<int, Client>::iterator it = Clients.find(clientSocket);
    if (it == Clients.end()) return;
    
    Client& client = it->second;
    HttpResponse& response = client.getResponse();
    
    switch (client.getClientState()) {
        case GENERATING_RESPONSE: {
            
            if (response.getStatuscode() < 400 && response.getResponseBody().empty()) {
                client.file.open(client.getRequest().getUriPath().c_str(), std::ios::binary);
                if (!client.file.is_open()) {
                    client.setState(COMPLETED);
                    break;
                }
                
                client.sendBuffer = response.getResponseHeaders();
            } else {
                client.sendBuffer = response.getResponseHeaders();
                client.sendBuffer += response.getResponseBody();
                response.getResponseBody().clear();
            }
            
            response.getResponseHeaders().clear();
            client.sendOffset = 0;
            client.setState(SENDING_DATA);
            break;
        }
        
        case SENDING_DATA: {
            if (!client.sendBuffer.empty()) {
                ssize_t bytesSent = send(clientSocket, 
                                        client.sendBuffer.c_str() + client.sendOffset, 
                                        client.sendBuffer.size() - client.sendOffset, 
                                        MSG_NOSIGNAL);
                if (bytesSent < 0)
                    return closeConnection(clientSocket);
        
                client.sendOffset += bytesSent;
                if (client.sendOffset >= client.sendBuffer.size()) {
                    client.sendBuffer.clear();
                    client.sendOffset = 0; 
                    if (!client.file.is_open())
                        client.setState(COMPLETED);
                }
            }
            
            if (client.sendBuffer.empty() && client.file.is_open()) {
                char buffer[READ_BUFFER_SIZE];
                client.file.read(buffer, READ_BUFFER_SIZE);
                std::streamsize bytesRead = client.file.gcount();

                if (bytesRead > 0) {
                    client.sendBuffer.append(buffer, bytesRead);
                } else {
                    client.file.close();
                    client.setState(COMPLETED);
                }
            }
            break;
        }

        case COMPLETED: {
            if (client.file.is_open())
                client.file.close();    
            
            LOG(response.getStatuscode(), client.request);
            if (response.getStatuscode() == 301 || response.getStatuscode() == 201)
                return closeConnection(clientSocket);
            if (client.shouldKeepAlive()) {
                client.resetState();
                client.setState(READING_REQUEST);
                modifyEpollEvent(clientSocket, EPOLLIN);
            }
            else { closeConnection(clientSocket); }
            break;
        }
        
        default:
            break;
    }
}

void    ServerManager::handleConnections(int listeningSocket)
{

    try {
        Server* server = findServerBySocket(listeningSocket);
        int clientFD = server->acceptConnection(listeningSocket);
        if (clientFD == -1) return ;
        addToEpoll(clientFD);
        Clients.insert(std::make_pair(clientFD, Client(clientFD, server->getserverConfig())));
    }
    catch(const std::exception& e) {
        std::cerr << e.what() << '\n';
    }
}

void ServerManager::readRequest(Client& Client) {
    uint8_t buffer[READ_BUFFER_SIZE];
    memset(buffer, 0, READ_BUFFER_SIZE);

    int bytesReceived = 0;
    int clientFd = Client.getFd();
    HttpRequest& request = Client.getRequest();
    bytesReceived = recv(clientFd, buffer, READ_BUFFER_SIZE, 0);
    if (bytesReceived == -1) {
        std::clog << ERROR << timeStamp()<< "ERROR: receiving data in client socket N" << clientFd << "\n" << RESET;
        return closeConnection(clientFd);
    }
    else if (bytesReceived == 0)
        return closeConnection(clientFd);

    std::vector<uint8_t>& requestBuffer = request.getRequestBuffer();
    requestBuffer.insert(requestBuffer.end(), buffer, buffer + bytesReceived);
    bytesReceived = request.parse(requestBuffer.data(), requestBuffer.size());
    // if (bytesReceived > 0)
    //     requestBuffer.erase(requestBuffer.begin(), requestBuffer.begin() + bytesReceived);
}

void ServerManager::handleRequest(int clientSocket)
{
    std::map<int, Client>::iterator It = Clients.find(clientSocket);
    if (It == Clients.end()) return ;
    
    Client& client = It->second;
    HttpRequest& request = client.getRequest();
    HttpResponse& response = client.getResponse();

    if (client.getClientState() == READING_REQUEST) {
        readRequest(client);
        if (request.getState() == COMPLETE) {
            client.setState(GENERATING_RESPONSE);
            response.generateResponse(request);
                
            modifyEpollEvent(clientSocket, EPOLLOUT);
        }
    }
    It->second.setLastActivity(time(NULL));
}


void ServerManager::handleEvent(const epoll_event& event) {
    int fd = event.data.fd;
    
    if (event.events & (EPOLLERR | EPOLLRDHUP | EPOLLHUP))
        return closeConnection(fd);

    if (event.events & EPOLLIN) {
        if (isListeningSocket(fd))
            handleConnections(fd);
        else
            handleRequest(fd);
    }
    if (event.events & EPOLLOUT)
        sendResponse(fd);
    
}

void    ServerManager::checkTimeouts()
{
    std::map<int, Client>::iterator it = Clients.begin();
    while (it != Clients.end())
    {
        time_t now = time(NULL);
        if (now - it->second.lastActivityTime >= it->second.timeout) {
            std::clog << INFO << timeStamp() << "INFO: Connection timed out, client socket N" << it->first <<".\n" << RESET;
            std::map<int, Client>::iterator toRem = it;
            ++it;
            closeConnection(toRem->first);
            continue;
        }
        ++it;
    }
}

void    ServerManager::eventsLoop()
{
    while (1) {
        int eventsNum = epoll_wait(epollFd, events.data(), events.size(), 1000);
        if (eventsNum == -1){
            std::cerr << ERROR << timeStamp() << "ERROR: in epoll_wait: " << strerror(errno) << std::endl << RESET;
            continue ;
        }
        if ((size_t)eventsNum == events.size())
            events.resize(events.size() * 2);

        for (int i = 0; i < eventsNum; i++)
            handleEvent(events[i]);
        checkTimeouts();
    }
}


void  ServerManager::initServers()
{
    handleSignals();
    for (size_t i = 0; i < serverPool.size(); i++)
    {
        try {    
            Server* server = new Server(serverPool[i]);
            std::clog << INFO << timeStamp() << "INFO: Setting & starting up server :\n" << RESET << "   -host: " << serverPool[i].getHost() << "\n";
            std::set<int>::const_iterator port =  serverPool[i].getPorts().begin();
            while (port != serverPool[i].getPorts().end()) {
                std::clog << "   -port: " << *port;
                ++port;
            }
            std::clog << std::endl;
            servers.push_back(server);
            std::vector<Socket*> sockets = server->getListeningSockets();
            for (size_t j = 0; j < sockets.size(); j++)
                listeningSockets[sockets[j]->getFd()] = sockets[j];
        }
        catch(const std::exception& e) {
            std::cerr << e.what() << '\n';
        }
    }
}
void ServerManager::initEpoll() {
    epollFd = epoll_create1(O_CLOEXEC);
    if (epollFd == -1) {
        throw std::runtime_error(ERROR + timeStamp() + "ERROR: creating epoll instance: " + std::string(strerror(errno) + std::string(RESET)));
    }
    addListeningSockets(servers);
}

ServerManager::ServerManager(const std::vector<Config>& _serverPool)
: serverPool(_serverPool), events(0), epollFd(-1)
{
    initServers();
    initEpoll();
    eventsLoop();
}