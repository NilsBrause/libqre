/*
 * Copyright 2016 Nils Christopher Brause
 *
 * This file is part of libqre.
 *
 * libqre is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libqre is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libqre.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <qre.hpp>

char32_t parse_octal(const std::u32string &str)
{
  char32_t result = 0;
  for(auto &ch : str)
    if('0' <= ch && ch <= '7')
      {
        result *= 8;
        result += ch - '0';
      }
    else
      throw std::runtime_error("Invalid octal number");
  return result;
}

char32_t parse_decimal(const std::u32string &str)
{
  char32_t result = 0;
  for(auto &ch : str)
    if('0' <= ch && ch <= '9')
      {
        result *= 10;
        result += ch - '0';
      }
    else
      throw std::runtime_error("Invalid decimal number");
  return result;
}

char32_t parse_hex(const std::u32string &str)
{
  char32_t result = 0;
  for(auto &ch : str)
    if('0' <= ch && ch <= '9')
      {
        result *= 16;
        result += ch - '0';
      }
    else if('a' <= ch && ch <= 'f')
      {
        result *= 16;
        result += ch - 'a' + 10;
      }
    else if('A' <= ch && ch <= 'F')
      {
        result *= 16;
        result += ch - 'A' + 10;
      }
    else
      throw std::runtime_error("Invalid hex number");
  return result;
}

char32_t read_escape(const std::u32string &str, unsigned int &pos)
{
  unsigned int oldpos = pos;
  assert(str[pos++] == '\\');
  char32_t ch = str[pos++];
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
    case '|':
    case '\\':
      break;
    case 'B':
      ch = '\\';
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
      if(str[pos++] == '{')
        {
          std::u32string tmp;
          while(true)
            {
              if(pos >= str.length())
                {
                  pos = oldpos;
                  return 0;
                }
              else
                {
                  ch = str[pos++];
                  if(ch == '}')
                    break;
                  else
                    tmp.push_back(ch);
                }
            }
          if(ch != '}')
            throw std::runtime_error("Expected '}'.");
          ch = parse_octal(tmp);
        }
      else
        throw std::runtime_error("Missing '{'.");
      break;
    case 'u': // Hexadecimal unicode code points
      if(str[pos++] == '{')
        {
          std::basic_string<char32_t> tmp;
          while(true)
            {
              if(pos >= str.length())
                {
                  pos = oldpos;
                  return 0;
                }
              else
                {
                  ch = str[pos++];
                  if(ch == '}')
                    break;
                  else
                    tmp.push_back(ch);
                }
            }
          if(ch != '}')
            throw std::runtime_error("Expected '}'.");
          ch = parse_hex(tmp);
        }
      else
        throw std::runtime_error("Missing '{'.");
      break;
      // hexadecimal characters
    case 'x':
      {
        std::basic_string<char32_t> tmp;
        for(unsigned int c = 0; c < 2; c++)
          {
            if(pos >= str.length())
              {
                pos = oldpos;
                return 0;
              }
            else
              tmp.push_back(str[pos++]);
          }
        ch = parse_hex(tmp);
      }
      break;
    default:
      pos = oldpos;
      std::cerr << ch << std::endl;
      throw std::runtime_error("Unknowen escape sequence.");
    }

  return ch;
}

qre::test_t qre::read_char_class(const std::u32string &str, unsigned int &pos, bool leading_backet) const
{
  if(leading_backet)
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
      // character class subtraction
      if(str.substr(pos, 2) == U"-[")
        {
          pos++;
          test.subtractions.push_back(read_char_class(str, pos));
        }
      // character class intersection
      else if(str.substr(pos, 3) == U"&&[")
        {
          pos += 2;
          test.intersections.push_back(read_char_class(str, pos));
        }
      // character class intersection without brackets
      else if(str.substr(pos, 2) == U"&&")
        {
          pos += 2;
          test.intersections.push_back(read_char_class(str, pos, false));
          return test; // closing bracket already read
        }
      else
        {
          // read character
          char32_t ch;
          if(str[pos] == '\\')
            {
              // escaping hypen is only valid in character classes
              if(str[pos+1] == '-')
                {
                  ch = '-';
                  pos += 2;
                }
              else
                ch = read_escape(str, pos);
            }
          else
            ch = str[pos++];

          // possible range
          if(str[pos] == '-')
            {
              // character class subtraction
              if(str[pos+1] == '[')
                continue;// handle above
              // literal '-' at the end
              else if(str[pos+1] == ']')
                {
                  test.chars.insert(ch);
                  test.chars.insert('-');
                  pos++;
                }
              // range
              else
                {
                  typename test_t::char_range cr;
                  cr.begin = ch;
                  pos++;
                  if(str[pos] == '\\')
                    {
                      // escaping hypen is only valid in character classes
                      if(str[pos+1] == '-')
                        {
                          cr.end = '-';
                          pos += 2;
                        }
                      else
                        cr.end = read_escape(str, pos);
                    }
                  else
                    cr.end = str[pos++];

                  if(cr.begin > cr.end)
                    throw std::runtime_error("Invalid character range.");
                  test.ranges.insert(cr);
                }
            }
          // individual character
          else
            test.chars.insert(ch);
        }
    }
  pos++;
  return test;
}

bool qre::read_range(const std::u32string &str, unsigned int &pos, range_t &r) const
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
  else if(str[pos++] == ',')
    {
      r.begin = 0;
      if(read_int())
        {
          r.end = n;
          if(str[pos++] == '}')
            return true;
        }
    }

  pos = oldpos;
  return false;
}

std::pair<signed int, signed int> qre::read_backref(const std::u32string &str, unsigned int &pos) const
{
  std::pair<signed int, signed int> result = { 0, -1 };
  char32_t paran = str[pos++];
  switch(paran)
    {
    case '\'':
      break;
    case '<':
      paran = '>';
      break;
    case '{':
      paran = '}';
      break;
    }

  std::u32string tmp;
  char32_t ch = 0;
  bool neg = false;

  if(str[pos] == '-')
    {
      pos++;
      neg = true;
    }
  else
    neg = false;
  while(true)
    {
      if(pos >= str.length())
        break;
      else
        {
          ch = str[pos++];
          if(ch == paran || ch == ',')
            break;
          else
            tmp.push_back(ch);
        }
    }
  if(ch != paran && ch != ',')
    throw std::runtime_error("Invalid backreference.");
  result.first = parse_decimal(tmp);
  if(neg)
    result.first = -result.first;

  if(ch == ',')
    {
      tmp.clear();
      if(str[pos] == '-')
        {
          pos++;
          neg = true;
        }
      else
        neg = false;
      while(true)
        {
          if(pos >= str.length())
            break;
          else
            {
              ch = str[pos++];
              if(ch == paran)
                break;
              else
                tmp.push_back(ch);
            }
        }
      if(ch != paran)
        throw std::runtime_error("Invalid backreference.");
      result.second = parse_decimal(tmp);
      if(neg)
        result.second = -result.second;
    }

  if(!result.first || !result.second)
    throw std::runtime_error("Invalid backreference number.");
  return result;
}

std::list<qre::symbol> qre::tokeniser(const std::u32string &str) const
{
  std::list<symbol> syms;
  unsigned int pos = 0;
  while(pos < str.length())
    {
      symbol sym;
      if(str[pos] == '(')
        {
          sym.type = symbol::type_t::lparan;
          pos++;
          if(str[pos] == '?')
            {
              // all special groups are non capturing
              sym.capture = false;
              switch(str[pos+1])
                {
                case ':':
                  break;
                case '>':
                  sym.atomic = true;
                  break;
                default:
                  throw std::runtime_error("Unsupported group.");
                  break;
                }
              pos += 2;
            }
          else
            sym.capture = true;
        }
      else if(str[pos] == ')')
        {
          sym.type = symbol::type_t::rparan;
          pos++;
        }
      else if(str[pos] == '|')
        {
          sym.type = symbol::type_t::alt;
          pos++;
        }
      else if(str[pos] == '.')
        {
          sym.type = symbol::type_t::test;
          sym.test.type = test_t::test_type::any;
          pos++;
        }
     else  if(str[pos] == '?')
        {
          sym.type = symbol::type_t::qmark;
          pos++;
        }
      else if(str[pos] == '*')
        {
          sym.type = symbol::type_t::star;
          pos++;
        }
      else if(str[pos] == '+')
        {
          sym.type = symbol::type_t::plus;
          pos++;
        }
      else if(str[pos] == '^')
        {
          sym.type = symbol::type_t::test;
          sym.test.type = test_t::test_type::bol;
          pos++;
        }
      else if(str[pos] == '$')
        {
          sym.type = symbol::type_t::test;
          sym.test.type = test_t::test_type::eol;
          pos++;
        }
      else if(str[pos] == '{')
        {
          if(read_range(str, pos, sym.range))
            sym.type = symbol::type_t::range;
          else
            {
              sym.type = symbol::type_t::test;
              sym.test.type = test_t::test_type::character;
              sym.test.chars.insert(str[pos]);
              pos++;
            }
        }
      else if(str[pos] == '[')
        {
          sym.type = symbol::type_t::test;
          sym.test = read_char_class(str, pos);
        }
      else if(str[pos] == ']')
        throw std::runtime_error("misplaceed paranthesis");
      else if(str[pos] == '\\')
          {
            unsigned int oldpos = pos;
            pos++;
            switch(str[pos++])
              {
              case '`':
              case 'A':
                sym.type = symbol::type_t::test;
                sym.test.type = test_t::test_type::bol;
                break;
              case 'N': // No Newline
                sym.type = symbol::type_t::test;
                sym.test.type = test_t::test_type::newline;
                sym.test.neg = true;
                break;
              case 'Q': // Literal sequence
                while(true)
                  {
                    if(pos == str.size())
                      throw std::runtime_error("Unterminated escape sequence.");
                    if(str.substr(pos, 2) == U"\\E")
                      {
                        pos += 2;
                        break;
                      }
                    else
                      {
                        symbol sym2;
                        sym2.type = symbol::type_t::test;
                        sym2.test.type = test_t::test_type::character;
                        sym2.test.chars.insert(str[pos++]);
                        syms.push_back(sym2);
                      }
                  }
                continue; // discard 'sym'
                break;
              case 'R': // New line
                sym.type = symbol::type_t::test;
                sym.test.type = test_t::test_type::newline;
                break;
              case '\'':
              case 'Z':
                sym.type = symbol::type_t::test;
                sym.test.type = test_t::test_type::eol;
                break;
              case 'g':
              case 'k':
                sym.type = symbol::type_t::test;
                sym.test.type = test_t::test_type::backref;
                sym.test.backref = read_backref(str, pos);
                break;
              case '-':
              case '1':
              case '2':
              case '3':
              case '4':
              case '5':
              case '6':
              case '7':
              case '8':
              case '9':
                sym.type = symbol::type_t::test;
                sym.test.type = test_t::test_type::backref;
                if(str[pos-1] == '-')
                  {
                    if('1' <= str[pos] && str[pos] <= '9')
                      sym.test.backref = { -1*(str[pos++]-'0'), -1 };
                    else
                      throw std::runtime_error("Invalid negative backreference number.");
                  }
                else
                  sym.test.backref = { str[pos-1]-'0', -1 };
                break;
              default:
                pos = oldpos;
                sym.type = symbol::type_t::test;
                sym.test.type = test_t::test_type::character;
                sym.test.chars.insert(read_escape(str, pos));
              }
          }
      else
        {
          sym.type = symbol::type_t::test;
          sym.test.type = test_t::test_type::character;
          sym.test.chars.insert(str[pos]);
          pos++;
        }
      syms.push_back(sym);
    }

  return syms;
}
