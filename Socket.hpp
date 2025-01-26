#pragma once

#include "Common.h"

class Socket
{
  private :
    int fd;

  public  :
    Socket();
    void  create();
    void  listen(int backlog);
    void  bind(const sockaddr server_addr);
    int   accept();
};