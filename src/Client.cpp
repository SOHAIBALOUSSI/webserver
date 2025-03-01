#include "../include/Client.hpp"

Client::Client(int client_fd, Config& Conf)
: client_fd(client_fd), client_config(Conf), request(Conf), response(Conf), sendOffset(0), state(READING_REQUEST), keepAlive(1), lastActivityTime(time(NULL)),
  timeout(1)
{}

Client::Client(const Client& C)
: client_fd(C.client_fd), client_config(C.client_config),
    request(C.request), response(C.response), sendOffset(C.sendOffset),
    state(C.state), keepAlive(C.keepAlive), lastActivityTime(C.lastActivityTime), timeout(C.timeout)
{}

bool Client::shouldKeepAlive()
{
    if (request.getHeaders().count("connection")) {
        if (toLowerCase(request.getHeaders()["connection"]).find("close") != std::string::npos)
            return (0);
    }
    return (1);
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