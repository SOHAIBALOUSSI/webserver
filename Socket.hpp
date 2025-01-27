#pragma once

#include "Common.h"

class Socket
{
  private :
    int fd;

  public  :
    Socket();
    ~Socket();
    void  create();
    void  listen(int backlog);
    void  bind(const sockaddr_in server_addr);
    int   accept();
    int getFd( void ) const;
};