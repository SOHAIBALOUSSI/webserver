#include "../include/Client.hpp"

Client::Client() : client_fd(-1) {}

Client::Client(int client_fd) : client_fd(client_fd) {}

