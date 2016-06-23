#include <regexp.hpp>

regexp::char_t regexp::read_escape(const std::string &str, unsigned int &pos)
{
  unsigned int oldpos = pos;
  assert(str[pos++] == '\\');
  char_t ch = str[pos++];
  switch(ch)
    {
    case '(':
    case ')':
    case '[':
    case ']':
    case '{':
    case '}':
    case '?':
    case '*':
    case '+':
    case '.':
    case '^':
    case '$':
    case '\\':
      break;
    case '0': // null
      ch = '\0';
      break;
    case 'a': // bell
      ch = '\a';
      break;
    case 'b': // backspace
      ch = '\b';
      break;
    case 'e': // escape
      ch = '\e';
      break;
    case 'f': // form feed
      ch = '\f';
      break;
    case 'n': // new line
      ch = '\n';
      break;
    case 'r': // carriage return
      ch = '\r';
      break;
    case 't': // horizontal tab
      ch = '\t';
      break;
    case 'v': // vertical tab
      ch = '\v';
      break;
    case 'c': // ASCII control characters
      ch = str[pos++] & 0x1F;
      break;
    case 'o': // Octal numbers
      if(str[pos] == '{')
        {
          typename std::basic_string<char_t>::size_type end
            = str.find('}', pos++);
          if(end == std::basic_string<char_t>::npos)
            throw std::runtime_error("Expected '}'.");
          ch = std::stoul(str.substr(pos, end-pos), nullptr, 8);
          pos = end + 1;
        }
      else
        throw std::runtime_error("Missing '{'.");
    case 'u':
    case 'x': // Hexadecimal characters
      if(str[pos] == '{')
        {
          typename std::basic_string<char_t>::size_type end
            = str.find('}', pos++);
          if(end == std::basic_string<char_t>::npos)
            throw std::runtime_error("Expected '}'.");
          ch = std::stoul(str.substr(pos, end-pos), nullptr, 16);
          pos = end + 1;
        }
      else
        {
          ch = std::stoul(str.substr(pos, 2), nullptr, 16) & 0xFF;
          pos += 2;
        }
      break;
    default:
      pos = oldpos;
      throw std::runtime_error("Invalid escape sequence.");
    }

  return ch;
}

regexp::test_t regexp::read_escape_sequence(const std::string &str, unsigned int &pos)
{
  test_t test;
  try
    {
      test.chars.insert(read_escape(str, pos));
      test.type = test_t::test_type::character;
    }
  catch(...)
    {
      unsigned int oldpos = pos;
      assert(str[pos++] == '\\');
      switch(str[pos++])
        {
        case '`':
        case 'A':
          test.type = test_t::test_type::bol;
          break;
        case 'N': // No Newline
          test.type = test_t::test_type::newline;
          test.neg = true;
          break;
        case 'Q': // Literal sequence
          test.type = test_t::test_type::sequence;
          while(true)
            {
              if(pos == str.size())
                throw std::runtime_error("Unterminated escape sequence.");
              if(str.substr(pos, 2) == "\\E")
                {
                  pos += 2;
                  break;
                }
              else
                test.sequence += str[pos++];
            }
          break;
        case 'R': // New line
          test.type = test_t::test_type::newline;
          break;
        case '\'':
        case 'Z':
          test.type = test_t::test_type::eol;
          break;
        default:
          pos = oldpos;
          throw std::runtime_error("Invalid escape sequence.");
        }
    }
  return test;
}

regexp::test_t regexp::read_char_class(const std::string &str, unsigned int &pos)
{
  assert(str[pos++] == '[');
  test_t test;
  test.type = test_t::test_type::character;

  // negation
  if(str[pos] == '^')
    {
      test.neg = true;
      pos++;
    }

  // unescaped bracket and hyphen at the beginning
  while(str[pos] == ']' || str[pos] == '-')
    test.chars.insert(str[pos++]);

  while(pos < str.length() && str[pos] != ']')
    {
      char_t ch;
      if(str[pos] == '\\')
        ch = read_escape(str, pos);
      else if(str[pos] == '-')
        {
          if(str[pos+1] == '[') // character class subtraction
            {
              test.chars.insert(ch);
              pos++;
              test.subtractions.push_back(read_char_class(str, pos));
            }
          else  // unescaped hyphen outside ranges
            {
              test.chars.insert(ch);
              test.chars.insert(str[pos++]);
            }
        }
      else
        ch = str[pos++];

      if(str[pos] == '-')
        {
          if(str[pos+1] == ']') // unescaped hyphen at the end
            {
              test.chars.insert(ch);
              test.chars.insert(str[pos++]);
            }
          else if(str[pos+1] == '[') // character class subtraction
            {
              test.chars.insert(ch);
              pos++;
              test.subtractions.push_back(read_char_class(str, pos));
            }
          else // range
            {
              typename test_t::char_range cr;
              cr.begin = ch;
              pos++;
              if(str[pos] == '\\')
                cr.end = read_escape(str, pos);
              else
                cr.end = str[pos++];
              if(cr.begin > cr.end)
                throw std::runtime_error("Invalid character range.");
              test.ranges.insert(cr);
            }
        }
      else
        test.chars.insert(ch);
    }
  pos++;
  return test;
}

bool regexp::read_range(const std::string &str, unsigned int &pos, range_t &r)
{
  unsigned int oldpos = pos;
  assert(str[pos++] == '{');

  unsigned int n;
  auto read_int = [&str, &pos, &n] () -> unsigned int
    {
      unsigned int len;
      for(len = 0, n = 0; '0' <= str[pos] && str[pos] <= '9'; len++)
        n = n * 10 + str[pos++] - '0';
      return len;
    };

  if(read_int())
    {
      r.begin = n;
      char ch = str[pos++];
      if(ch == '}')
        {
          r.end = n;
          return true;
        }
      else if(ch == ',')
        {
          if(str[pos] == '}')
            {
              pos++;
              r.infinite = true;
              return true;
            }
          else if(read_int())
            {
              r.end = n;
              if(str[pos++] == '}')
                return true;
            }
        }
    }
  else if(str[pos] == ',')
    {
      r.begin = 0;
      if(read_int())
        {
          r.end = n;
          if(str[pos] == '}')
            return true;
        }
    }

  pos = oldpos;
  return false;
}

std::list<regexp::symbol> regexp::tokeniser(const std::string &str)
{
  std::list<symbol> syms;
  unsigned int pos = 0;
  while(pos < str.length())
    {
      symbol sym;
      switch(str[pos])
        {
        case '(':
          sym.type = symbol::type_t::lparan;
          pos++;
          if(str[pos] == '?' && str[pos+1] == ':')
            {
              sym.capture = false;
              pos += 2;
            }
          else
            sym.capture = true;
          break;
        case ')':
          sym.type = symbol::type_t::rparan;
          pos++;
          break;
        case '|':
          sym.type = symbol::type_t::alt;
          pos++;
          break;
        case '.':
          sym.type = symbol::type_t::test;
          sym.test.type = test_t::test_type::any;
          pos++;
          break;
        case '?':
          sym.type = symbol::type_t::range;
          sym.range.begin = 0;
          sym.range.end = 1;
          pos++;
          break;
        case '*':
          sym.type = symbol::type_t::range;
          sym.range.begin = 0;
          sym.range.infinite = true;
          pos++;
          break;
        case '+':
          sym.type = symbol::type_t::range;
          sym.range.begin = 1;
          sym.range.infinite = true;
          pos++;
          break;
        case '^':
          sym.type = symbol::type_t::test;
          sym.test.type = test_t::test_type::bol;
          pos++;
          break;
        case '$':
          sym.type = symbol::type_t::test;
          sym.test.type = test_t::test_type::eol;
          pos++;
          break;
        case '{':
          if(read_range(str, pos, sym.range))
            sym.type = symbol::type_t::range;
          else
            {
              sym.type = symbol::type_t::test;
              sym.test.type = test_t::test_type::character;
              sym.test.chars.insert(str[pos]);
              pos++;
            }
          break;
        case '[':
          sym.type = symbol::type_t::test;
          sym.test = read_char_class(str, pos);
          break;
        case '\\':
          sym.type = symbol::type_t::test;
          sym.test = read_escape_sequence(str, pos);
          break;
        case ']':
          throw std::runtime_error("mispaceed ']'");
          break;
        case '}':
          // literal '}' if not part of a range expression.
        default:
          sym.type = symbol::type_t::test;
          sym.test.type = test_t::test_type::character;
          sym.test.chars.insert(str[pos]);
          pos++;
          break;
        }

      syms.push_back(sym);
    }

  return syms;
}
