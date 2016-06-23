#include <regexp.hpp>

bool regexp::test_t::char_range::operator==(const char_range &r) const
{
  return begin == r.begin && end == r.end;
}

bool regexp::test_t::char_range::operator<(const char_range &r) const
{
  if(begin < r.begin)
    return true;
  else if(begin > r.begin)
    return false;
  if(end < r.end)
    return true;
  else
    return false;
}

bool regexp::test_t::check(const std::string &str, unsigned int &pos,
                           bool multiline) const
{
  bool result = false;
  switch(type)
    {
    case test_type::epsilon:
#ifdef DEBUG
      std::cerr << "epsilon" << std::endl;
#endif
      return true;
      break;

    case test_type::bol:
#ifdef DEBUG
      std::cerr << "bol" << std::endl;
#endif
      if(multiline)
        return pos == 0 || (pos < str.length() && str[pos-1] == '\n');
      else
        return pos == 0;
      break;

    case test_type::eol:
#ifdef DEBUG
      std::cerr << "eol" << std::endl;
#endif
      if(multiline)
        return pos == str.length() || (pos < str.length() && str[pos] == '\n' && pos++);
      else
        return pos == str.length();
      break;

    case test_type::any:
#ifdef DEBUG
      std::cerr << "any" << std::endl;
#endif
      if(pos == str.length())
        return false; // no characters here
      else if(multiline)
        return str[pos] != '\n' ? (pos++, true) : false;
      else
        return pos++, true;
      break;

    case test_type::newline:
#ifdef DEBUG
      std::cerr << "newline" << std::endl;
#endif
      if(pos == str.length())
        result = false; // no characters here
      else if(str[pos] == '\r')
        result = str[pos+1] == '\n' ? pos += 2, true : (pos++, true);
      else if(str[pos] == '\n')
        result = (pos++, true);
      return result != neg;

    case test_type::character:
#ifdef DEBUG
      std::cerr << "character: " << std::flush;
#endif
      if(pos == str.length())
        return false; // no characters here
      // single characters
      for(auto &c: chars)
        {
#ifdef DEBUG
          std::cerr << "\"" << c << "\", " << std::endl;
#endif
          if(c == str[pos])
            result = true;
        }
      // character ranges
      for(auto &r : ranges)
        {
#ifdef DEBUG
          std::cerr << "\"" << r.begin << "\"-\"" << r.end
                    << "\", " << std::endl;
#endif
          if(r.begin <= str[pos] && str[pos] <= r.end)
            result = true;
        }
      // negation
      result = result != neg;
      // subtraction
      for(auto &sub : subtractions)
        if(result && sub.check(str, pos))
          result = false;
      if(result)
        pos++;
      return result;
      break;

    case test_type::sequence:
#ifdef DEBUG
      std::cerr << "sequence: " << std::flush;
      for(unsigned int c = 0; c < sequence.length(); c++)
        std::cerr << sequence[c] << std::endl;
#endif
      for(unsigned int c = 0; c < sequence.length(); c++)
        if(pos+c == str.length() || sequence[c] != str[pos+c])
          return false;
      pos += sequence.length();
      return true;
      break;
    default:
      throw std::runtime_error("unknowen test type.");
      return false;
      break;
    }
}
