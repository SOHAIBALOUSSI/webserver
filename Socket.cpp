#include "Socket.hpp"

Socket::Socket() : fd(-1) {}

void  Socket::create()
{
  fd = socket(PF_INET, SOCK_STREAM, 0);
  if (fd == -1)
      throw std::runtime_error("Error creating socket");
  int opt = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
  {
    close(fd);
    throw std::runtime_error("Error setting socket opt");
  }
}

void  Socket::listen(int backlog)
{
  if (::listen(fd, backlog))
    throw std::runtime_error("Listening failed");
}

void  Socket::bind(const sockaddr server_addr)
{
  if (::bind(fd, (sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    throw std::runtime_error("Binding failed");
}

int  Socket::accept()
{
  sockaddr_in client_addr;
  socklen_t client_len = sizeof(client_addr);
  int client_fd = ::accept(fd, (struct sockaddr*)&client_addr, &client_len);
  if (client_fd == -1)
    throw std::runtime_error("Error accepting connection");
  return (client_fd);
}