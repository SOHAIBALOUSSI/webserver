#include "Server.hpp"
#include "HttpRequest.hpp"

Server::Server(std::string host, int port)
{
    this->host = host;
    this->port = port;
    server_fd = -1;
    memset(&server_addr, 0, sizeof(server_addr));
}

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
            std::clog << "PORT : " << *it << std::endl;
            serverAddr.sin_port = htons(*it);
            std::clog << "IP ADDRESS : (STRING FORM) : " << serverConfig.host << " (BINARY FORM) : " << stringToIpBinary(serverConfig.host) << '\n';
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

void Server::setupServer()
{
    server_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
        throw std::runtime_error("ERROR: creating socket" + std::string(strerror(errno)));
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        close(server_fd);
        throw std::runtime_error("ERROR:  setting socket opt" + std::string(strerror(errno)));
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(server_fd, (sockaddr *)&server_addr, sizeof(server_addr)) == -1)
        throw std::runtime_error("Binding failed");
    listen(server_fd, SOMAXCONN);
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
    for (size_t i = 0; i < clientSockets.size(); ++i)
        close(clientSockets[i]);
    close(server_fd);
    std::clog << "LOG: Server shut down.\n";
}


Server::~Server()
{
    std::cerr << "LOG: Server shutting down...\n";
    // shutdownServer();
}

const std::vector<Socket*>& Server::getSockets( void ) const
{
    return (listeningSockets);
}