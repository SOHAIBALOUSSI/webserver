#pragma once

#include <string>
#include <string.h>
#include <utility>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <cstdlib>
#include <cstdio>

#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <sys/epoll.h>

#include <errno.h>

#include <sstream>
#include <iostream>

#include <algorithm>
#include <map>
#include <vector>
#include <queue>
#include <unordered_map>

std::string&  toLowerCase(std::string& str);
uint32_t stringToIpBinary(std::string addressIp);
int hexToValue(char c);
bool    isURIchar(char c);
bool isHexDigit(char c);