#include <qre.hpp>

char32_t qre::advance(const std::string &str, unsigned int &pos)
{
  char32_t result;

  // EOF
  if(pos >= str.length())
    return 0;
  // ASCII
  else if((static_cast<uint8_t>(str[pos]) & 0x80) == 0)
    result = static_cast<uint8_t>(str[pos++]);
  // UTF-8
  else
    {
      unsigned int old = pos;
      unsigned int tail;
      // two byte sequence
      if((static_cast<uint8_t>(str[pos]) & 0xC0) == 0xC0
         && (static_cast<uint8_t>(str[pos]) & 0x20) == 0)
        {
          result = static_cast<uint8_t>(str[pos++]) & 0x1F;
          tail = 1;
        }
      // three byte sequence
      else if((static_cast<uint8_t>(str[pos]) & 0xE0) == 0xE0
              && (static_cast<uint8_t>(str[pos]) & 0x10) == 0)
        {
          result = static_cast<uint8_t>(str[pos++]) & 0x0F;
          tail = 2;
        }
      // four byte sequence
      else if((static_cast<uint8_t>(str[pos]) & 0xF0) == 0xF0
              && (static_cast<uint8_t>(str[pos]) & 0x08) == 0)
        {
          result = static_cast<uint8_t>(str[pos++]) & 0x07;
          tail = 3;
        }
      else
        throw std::runtime_error("Invalid UTF-8");

      for(unsigned int c = 0; c < tail; c++)
        {
          if(pos >= str.length())
            {
              pos = old;
              return 0;
            }
          else if((static_cast<uint8_t>(str[pos]) & 0x80) == 0x80
             && (static_cast<uint8_t>(str[pos]) & 0x40) == 0)
            {
              result <<= 6;
              result |= static_cast<uint8_t>(str[pos++]) & 0x3F;
            }
          else
            throw std::runtime_error("Invalid UTF-8");
        }
    }

  return result;
}

char32_t qre::peek(const std::string &str, unsigned int pos)
{
  return advance(str, pos);
}

char32_t qre::peek_prev(const std::string &str, unsigned int pos)
{
  unsigned int old = pos;
  while(true)
    {
      if(pos == 0)
        {
          pos = old;
          return 0;
        }
      else
        pos--;

      if((static_cast<uint8_t>(str[pos]) & 0x80) == 0x80
              && (static_cast<uint8_t>(str[pos]) & 0x40) == 0)
        continue;
      else
        return advance(str, pos);
    }
}

std::u32string qre::utf8toutf32(const std::string &str) const
{
  unsigned int pos = 0;
  char32_t tmp;
  std::u32string result;
  while((tmp = qre::advance(str, pos)) != 0)
    result.push_back(tmp);
  return result;
}

std::string qre::utf32toutf8(char32_t ch) const
{
  std::string result;
  if(ch <= 0x7F)
    result += static_cast<char>(ch);
  else if(ch <= 0x7FF)
    {
      result += static_cast<char>(((ch >> 6) & 0x1F) | 0xC0);
      result += static_cast<char>(((ch >> 0) & 0x3F) | 0x80);
    }
  else if(ch <= 0xFFFF)
    {
      result += static_cast<char>(((ch >> 12) & 0x0F) | 0xE0);
      result += static_cast<char>(((ch >> 6) & 0x3F) | 0x80);
      result += static_cast<char>(((ch >> 0) & 0x3F) | 0x80);
    }
  else if(ch <= 0x10FFFF)
    {
      result += static_cast<char>(((ch >> 18) & 0x07) | 0xF0);
      result += static_cast<char>(((ch >> 12) & 0x3F) | 0x80);
      result += static_cast<char>(((ch >> 6) & 0x3F) | 0x80);
      result += static_cast<char>(((ch >> 0) & 0x3F) | 0x80);
    }
  else
    throw std::runtime_error("Invalid UTF-32");
  return result;
}

std::string qre::utf32toutf8(const std::u32string &str) const
{
  std::string result;
  for(auto &ch : str)
    result += utf32toutf8(ch);
  return result;
}
