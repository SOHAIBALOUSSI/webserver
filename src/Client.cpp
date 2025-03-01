#include "../include/Client.hpp"

Client::Client(int client_fd, Config &Conf)
    : client_fd(client_fd), client_config(Conf), request(Conf), response(Conf), sendOffset(0), state(READING_REQUEST), keepAlive(1)
{
}

bool Client::shouldKeepAlive()
{
    std::map<std::string, std::string>::iterator It = request.getHeaders().find("connection");
    if (It != request.getHeaders().end())
    {
        std::string connectionValue = It->second;
        if (connectionValue.find("close") != std::string::npos)
        {
            return false;
        }
        else if (connectionValue.find("keep-alive") != std::string::npos)
        {
            return true;
        }
    }
    return true;
}

void Client::resetState()
{
    request.reset();
    response.reset();
    if (file.is_open())
    {
        file.close();
    }
    sendBuffer.clear();
    sendOffset = 0;
}