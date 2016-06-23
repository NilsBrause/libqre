#include <iostream>
#include <regexp.hpp>

int main()
{
  regexp r("\\Q.*\\E");
  std::vector<std::string> result;
  if(r.match(".*"))
    {
      std::cout << "pos: " << r.result.pos << std::endl;
      for(auto &sub : r.result.sub)
        std::cout << "sub: " << sub << std::endl;
    }
  return 0;
}

