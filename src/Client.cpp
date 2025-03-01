#include "../include/Client.hpp"

Client::Client(int client_fd, Config& Conf)
: client_fd(client_fd), client_config(Conf), request(Conf), response(Conf), sendOffset(0), state(READING_REQUEST), keepAlive(1), lastActivityTime(time(NULL)), timeout(60)
{}


bool Client::shouldKeepAlive()
{
    if (request.getHeaders().count("connection")) {
        if (request.getHeaders()["connection"].find("keep-alive")) {
            timeout = KEEP_ALIVE_TIMEOUT;
            return (1);
        } else {
            timeout = DEFAULT_TIMEOUT;
            return (0);
        }
    }
    return 1;
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