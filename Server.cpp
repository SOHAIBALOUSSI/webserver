#include "Server.hpp"
#include "HttpRequest.hpp"

Server::Server(std::string host, int port)
{
    this->host = host;
    this->port = port;
    server_fd = -1;
    memset(&server_addr, 0, sizeof(server_addr));
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

void Server::handleConnections()
{
    struct pollfd fds[client_fds.size() + 1];
    fds[0].fd = server_fd;
    fds[0].events = POLLIN;
    for (size_t i = 0; i < client_fds.size(); ++i)
    {
        fds[i + 1].fd = client_fds[i];
        fds[i + 1].events = POLLIN | POLLOUT;
    }
    int ret = poll(fds, client_fds.size() + 1, -1);
    if (ret == -1)
        throw std::runtime_error("Poll failed");
    if (fds[0].revents & POLLIN)
        acceptConnection();
    for (size_t i = 0; i < client_fds.size(); ++i)
    {
        if (fds[i + 1].revents & POLLIN)
            handleHttpRequest(client_fds[i]);
    }
}

void Server::acceptConnection()
{
    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd == -1)
        throw std::runtime_error("Error accepting connection");
    client_fds.push_back(client_fd);
    std::cout << "New client connected.\n";
}

void Server::closeConnection(int client_fd)
{
    std::vector<int>::iterator it = std::find(client_fds.begin(), client_fds.end(), client_fd);
    if (it != client_fds.end())
    {
        close(client_fd);
        client_fds.erase(it);
        std::cout << "Connection closed.\n";
    }
}

void Server::handleHttpRequest(int client_fd)
{
    int flags = fcntl(client_fd, F_GETFL, 0);
    if (flags == -1)
    {
        std::cerr << "Error getting flags for client socket\n";
        closeConnection(client_fd);
        return;
    }
    fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

    const size_t buffer_size = 1024;
    char buffer[buffer_size];
    std::string full_request;

    int bytes_received;
    while ((bytes_received = recv(client_fd, buffer, buffer_size, 0)) > 0)
        full_request.append(buffer, bytes_received);
    if (bytes_received == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
    {
        std::cerr << "Error receiving data\n";
        closeConnection(client_fd);
    }
    else if (bytes_received == 0)
    {
        std::cout << "Client disconnected\n";
        closeConnection(client_fd);
        return;
    }
    buffer[bytes_received] = '\0';
    std::string response;
    try
    {
        HttpRequest req(buffer);
        response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 13\r\n\r\nHello, World!";
    }
    catch (const std::exception& e)
    {
        response = e.what();
    }
    if (send(client_fd, response.c_str(), response.size(), 0) == -1) {
        std::cerr << "Error sending data\n";
        closeConnection(client_fd);
    }
}

void Server::start()
{
    try
    {
        setupServer();
        while (true)
            handleConnections();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

void Server::shutdownServer()
{
    for (size_t i = 0; i < client_fds.size(); ++i)
        close(client_fds[i]);
    close(server_fd);
    std::cout << "Server shut down.\n";
}


Server::~Server()
{
    shutdownServer();
}