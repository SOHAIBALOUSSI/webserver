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
            std::cout << "PORT : " << *it << std::endl;
            serverAddr.sin_port = htons(*it);
            std::cout << "IP ADDRESS : (STRING FORM) : " << serverConfig.host << " (BINARY FORM) : " << stringToIpBinary(serverConfig.host) << '\n';
            serverAddr.sin_addr.s_addr = htonl(stringToIpBinary(serverConfig.host));
            serverSocket->bind(serverAddr);
            serverSocket->listen(SOMAXCONN);
            listeningSockets.push_back(serverSocket);
        }
        catch(const std::exception& e)
        {
            std::cerr << "Error setting up server on port " << *it << ": " << e.what() << std::endl;
        }
        ++it;
    }
}

void Server::setupServer()
{
    server_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
        throw std::runtime_error("Error creating socket");
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        perror("setsockopt failed");
        close(server_fd);
        throw std::runtime_error("Error setting socket opt");
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(server_fd, (sockaddr *)&server_addr, sizeof(server_addr)) == -1)
        throw std::runtime_error("Binding failed");
    listen(server_fd, SOMAXCONN);
}

// void Server::handleConnections()
// {
//     struct pollfd fds[clientSockets.size() + 1];
//     fds[0].fd = server_fd;
//     fds[0].events = POLLIN;
//     for (size_t i = 0; i < clientSockets.size(); ++i)
//     {
//         fds[i + 1].fd = clientSockets[i];
//         fds[i + 1].events = POLLIN | POLLOUT;
//     }
//     //change to epoll later
//     int ret = poll(fds, clientSockets.size() + 1, -1);
//     if (ret == -1)
//         throw std::runtime_error("Poll failed");
//     if (fds[0].revents & POLLIN)
//         acceptConnection(fds[0].fd);
//     for (size_t i = 0; i < clientSockets.size(); ++i)
//     {
//         if (fds[i + 1].revents & POLLIN)
//             handleHttpRequest(clientSockets[i]);
//     }
// }

int Server::acceptConnection(int listeningSocket)
{
    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int clientSocket = accept(listeningSocket, (struct sockaddr*)&client_addr, &client_len);
    if (clientSocket == -1)
    {
        // if (errno == EAGAIN || errno == EWOULDBLOCK)
        //     return -1;
        throw std::runtime_error("Error accepting connection: " + std::string(strerror(errno)));
    }
    clientSockets.push_back(clientSocket);
    std::cout << "New client connected.\n";
    return (clientSocket);
}

void Server::closeConnection(int client_fd)
{
    std::vector<int>::iterator it = std::find(clientSockets.begin(), clientSockets.end(), client_fd);
    if (it != clientSockets.end())
    {
        close(client_fd);
        clientSockets.erase(it);
        std::cout << "Connection closed.\n";
    }
}

// void Server::handleHttpRequest(int client_fd)
// {
//     const size_t buffer_size = 1024;
//     char buffer[buffer_size];
//     memset(buffer, 0, buffer_size);
//     std::string full_request;

//     int bytes_received;
//     while ((bytes_received = recv(client_fd, buffer, buffer_size, 0)) > 0)
//         full_request.append(buffer, bytes_received);
//     if (bytes_received == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
//     {
//         std::cerr << "Error receiving data\n";
//         closeConnection(client_fd);
//     }
//     else if (bytes_received == 0)
//     {
//         std::cout << "Client disconnected\n";
//         closeConnection(client_fd);
//         return;
//     }
//     std::string response;
//     try
//     {
//         HttpRequest req(buffer);
//         response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 13\r\n\r\nHello, World!";
//     }
//     catch (const std::exception& e)
//     {
//         response = e.what();
//     }
//     if (send(client_fd, response.c_str(), response.size(), 0) == -1) 
//     {
//         std::cerr << "Error sending data\n";
//         closeConnection(client_fd);
//     }
// }

// void Server::start()
// {
//     try
//     {
//         setupServer();
//         while (true)
//             handleConnections();
//     }
//     catch (const std::exception& e)
//     {
//         std::cerr << "Error: " << e.what() << std::endl;
//     }
// }

void Server::shutdownServer()
{
    for (size_t i = 0; i < clientSockets.size(); ++i)
        close(clientSockets[i]);
    close(server_fd);
    std::cout << "Server shut down.\n";
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