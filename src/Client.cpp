#include "../include/Client.hpp"

Client::Client(int client_fd, Config &Conf)
    : client_fd(client_fd), sendOffset(0), request(Conf), response(Conf),
      state(READING_REQUEST), keepAlive(1), client_config(Conf)
{
}

void Client::resetState()
{
    request.reset();
    response.reset();
    if (file.is_open())
        file.close();
    sendBuffer.clear();
    sendOffset = 0;
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