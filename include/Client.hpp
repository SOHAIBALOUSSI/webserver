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
        std::string    sendBuffer;
        size_t         sendOffset;
        std::ifstream  file;
        size_t          fileOffset;
        HttpRequest     request;
        HttpResponse    response;
        ClientState     state;
        bool            keepAlive;
        int serverPort;
        Config& client_config;

    
        Client(int client_fd, Config& Conf);
        void resetState();
        int getFd() const { return client_fd; }
        bool    shouldKeepAlive();

        HttpRequest& getRequest() { return request; }
        HttpResponse& getResponse() { return response; }
        std::string& getSendBuffer() { return sendBuffer; }
        ClientState getClientState() { return state; }

        void    setKeepAlive(bool keep) { keepAlive = keep; }
        bool    getKeepAlive() { return keepAlive; }
        void    setState(ClientState _state) { state = _state; }
        int     getServerPort() const { return serverPort; }
        void    setServerPort(int port) { serverPort = port; }
};