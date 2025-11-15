#include <string>
#include <iostream>

std::pair<std::string, std::string> splitStr(const std::string &in, char sep)
{
  auto pos = in.find(sep);
  if (pos == std::string::npos)
    return {"", ""};
  return {in.substr(0, pos), in.substr(pos + 1)};
}