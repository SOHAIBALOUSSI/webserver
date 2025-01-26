#pragma once

#include <string>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <cstdlib>
#include <cstdio>

#include <unistd.h>
#include <poll.h>
#include <fcntl.h>

#include <errno.h>

#include <sstream>
#include <iostream>

#include <algorithm>
#include <map>
#include <vector>

std::string&  toLowerCase(std::string& str);
int stringToIpBinary(std::string addressIp);
