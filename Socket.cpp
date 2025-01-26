#include "Socket.hpp"

Socket::Socket() : fd(-1) {}

void  Socket::create()
{
  fd = socket(PF_INET, SOCK_STREAM, 0);
  if (fd == -1)
      throw std::runtime_error("Error creating socket" + std::string(strerror(errno)));
  int opt = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    throw std::runtime_error("Error setting socket opt" + std::string(strerror(errno)));
}

void  Socket::listen(int backlog)
{
  if (::listen(fd, backlog))
    throw std::runtime_error("Listening failed" + std::string(strerror(errno)));
}

void  Socket::bind(const sockaddr_in server_addr)
{
  if (::bind(fd, (sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    throw std::runtime_error("Binding failed" + std::string(strerror(errno)));
}

int  Socket::accept()
{
  sockaddr_in client_addr;
  socklen_t client_len = sizeof(client_addr);
  int client_fd = ::accept(fd, (struct sockaddr*)&client_addr, &client_len);
  if (client_fd == -1)
    throw std::runtime_error("Error accepting connection" + std::string(strerror(errno)));
  return (client_fd);
}

void  Socket::setNonBlocking()
{
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1)
    throw std::runtime_error("Error getting flags for client socket" + std::string(strerror(errno)));
  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK))
      throw std::runtime_error("Error setting socket to non-blocking: " + std::string(strerror(errno)));
}

int Socket::getFd( void ) const
{
  return (fd);
}

Socket::~Socket()
{
  if (fd != -1)
    close(fd);
}