#pragma once

#include <bits/stdc++.h>
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Config.hpp"


enum ClientState {
    READING_REQUEST,
    GENERATING_RESPONSE,
    SENDING_DATA,
    COMPLETED
};

class Client {
    public:
        int client_fd;
        Config& client_config;
        HttpRequest     request;
        HttpResponse    response;
        size_t         sendOffset;
        ClientState     state;
        bool            keepAlive;
        time_t          lastActivityTime;
        int             timeout;
        std::string    sendBuffer;
        std::ifstream  file;
        size_t          fileOffset;
        int serverPort;

        Client(int client_fd, Config& Conf);
        Client(const Client& C);
        void resetState();
        int getFd() const { return client_fd; }
        bool    shouldKeepAlive();

        HttpRequest& getRequest() { return request; }
        HttpResponse& getResponse() { return response; }
        std::string& getSendBuffer() { return sendBuffer; }
        ClientState getClientState() { return state; }

        void    setKeepAlive(bool keep) { keepAlive = keep; }
        bool    getKeepAlive() { return keepAlive; }
        void   setState(ClientState _state) { state = _state; }
        int getServerPort() const { return serverPort; }
        void setServerPort(int port) { serverPort = port; }
        void setLastActivity(time_t time) { lastActivityTime = time; }
};