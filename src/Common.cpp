#include "../include/Common.h"

std::string&  toLowerCase(std::string& str)
{
  for (size_t i = 0; i < str.size(); i++)
    str[i] = std::tolower(str[i]);
  return (str);
}

uint32_t stringToIpBinary(std::string addressIp)
{
  uint32_t ip[4];
  std::istringstream iss(addressIp);
  std::string octet;
  uint32_t actualIpAddress = 0;
  for (int i = 0; i < 4; i++)
  {
    std::getline(iss, octet, '.');
    ip[i] = std::atoi(octet.c_str());
    actualIpAddress |= (ip[i] << (24 - (i * 8)));
  }
  if (iss.peek() != EOF)
    throw std::invalid_argument("Invalid IP address format: " + addressIp);
  return (actualIpAddress);
}

int hexToValue(char c)
{
  if (c >= 'a' && c <= 'z') return c - 'a' + 10;
  if (c >= 'A' && c <= 'Z') return c - 'A' + 10;
  if (c >= '0' && c <= '9') return c - '0';
}

bool isHexDigit(char c)
{
  return (std::isdigit(c) || (std::tolower(c) >= 'a' && std::tolower(c) <= 'f'));
}