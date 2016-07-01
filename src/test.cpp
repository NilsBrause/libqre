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
      if(pos < str.length())
        {
          unsigned int oldpos = pos;
          if(str[pos] == '\r')
            {
              if(pos+1 < str.length() && str[pos+1] == '\n')
                pos += 2;
              else
                pos++;
              result = true;
            }
          else if(str[pos] == '\n')
            {
              pos++;
              result = true;
            }

          if(result != neg)
            return true;
          else
            {
              pos = oldpos;
              return false;
            }
        }
      else
        return neg; // no characters here
      break;

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
          std::cerr << "\"" << (char)c << "\", " << std::flush;
#endif
          if(c == str[pos])
            result = true;
        }

      // character ranges
      for(auto &r : ranges)
        {
#ifdef DEBUG
          std::cerr << "\"" << (char)r.begin << "\"-\"" << (char)r.end
                    << "\", " << std::flush;
#endif
          if(r.begin <= str[pos] && str[pos] <= r.end)
            result = true;
        }
#ifdef DEBUG
      std::cerr << std::endl;
#endif

      // negation
      result = result != neg;

      // subtraction
      for(auto &sub : subtractions)
        {
          unsigned int oldpos = pos;
          if(result && sub.check(str, pos))
            {
              pos = oldpos;
              result = false;
              break;
            }
        }

      // intersection
      for(auto &itr : intersections)
        {
          unsigned int oldpos = pos;
          if(result && !itr.check(str, pos))
            {
              result = false;
              break;
            }
          else
            pos = oldpos;
        }

      if(result)
        pos++;
      return result;
      break;

    default:
      throw std::runtime_error("unknowen test type.");
      return false;
      break;
    }
}
