#include "../../include/Server.hpp"
#include "../../include/HttpRequest.hpp"

Server::Server(const Config& serverConfig)
{
    std::set<int>::const_iterator it = serverConfig.ports.begin();
    while (it != serverConfig.ports.end())
    {
        try
        {
            Socket* serverSocket = new Socket;
            serverSocket->create();
            sockaddr_in serverAddr;
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_port = htons(*it);
            serverAddr.sin_addr.s_addr = htonl(stringToIpBinary(serverConfig.host));
            serverSocket->bind(serverAddr);
            serverSocket->listen(SOMAXCONN);
            listeningSockets.push_back(serverSocket);
        }
        catch(const std::exception& e)
        {
            std::cerr << "ERROR:  setting up server on port " << *it << ": " << e.what() << std::endl;
        }
        ++it;
    }
}

int Server::acceptConnection(int listeningSocket)
{
    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int clientSocket = accept(listeningSocket, (struct sockaddr*)&client_addr, &client_len);
    if (clientSocket == -1)
    {
        // if (errno == EAGAIN || errno == EWOULDBLOCK)
        //     return -1;
        throw std::runtime_error("ERROR:  accepting connection: " + std::string(strerror(errno)));
    }
    clientSockets.push_back(clientSocket);
    std::clog << "LOG: New client connected, client socket N" << clientSocket << "\n";
    return (clientSocket);
}

void Server::closeConnection(int client_fd)
{
    std::vector<int>::iterator it = std::find(clientSockets.begin(), clientSockets.end(), client_fd);
    if (it != clientSockets.end())
    {
        close(client_fd);
        clientSockets.erase(it);
        std::clog << "LOG: Connection closed.\n";
    }
}

void Server::shutdownServer()
{
    for (std::vector<Socket*>::iterator socket = listeningSockets.begin(); 
        socket != listeningSockets.end();
        socket++)
        delete *socket;
    for (size_t i = 0; i < clientSockets.size(); ++i)
        close(clientSockets[i]);
    close(server_fd);
    std::clog << "LOG: Server shut down.\n";
}


Server::~Server()
{
    std::cerr << "LOG: Server shutting down...\n";
    shutdownServer();
}

const std::vector<Socket*>& Server::getListeningSockets( void ) const
{
    return (listeningSockets);
}

const std::vector<int>& Server::getClientSockets( void ) const
{
    return (clientSockets);
}