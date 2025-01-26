#include "Common.h"

std::string&  toLowerCase(std::string& str)
{
  for (size_t i = 0; i < str.size(); i++)
    str[i] = std::tolower(str[i]);
  return (str);
}

int stringToIpBinary(std::string addressIp)
{
  int ip[4];
  std::istringstream iss(addressIp);
  std::string octet;
  int actualIpAddress;
  for (int i = 0; i < 4; i++)
  {
    std::getline(iss, octet, '.');
    ip[i] = std::atoi(octet.c_str());
  }
  actualIpAddress = (ip[0] << 24) | (ip[1] << 16) | (ip[2] << 8) | ip[3];
  return (actualIpAddress);
}
