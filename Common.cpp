#include "Common.h"

std::string&  toLowerCase(std::string& str)
{
  for (size_t i = 0; i < str.size(); i++)
    str[i] = std::tolower(str[i]);
  return (str);
}

// int  toNumber(std::string& str)
// {
 

// }