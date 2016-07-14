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

bool qre::test_t::char_range::operator==(const char_range &r) const
{
  return begin == r.begin && end == r.end;
}

bool qre::test_t::char_range::operator<(const char_range &r) const
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

bool qre::check(const test_t &test, const std::string &str,
                unsigned int &pos, bool multiline, bool utf8,
                match &match_sofar)
{
  bool result = false;
  unsigned int newpos = pos;
  char32_t tmp;

  auto advance = qre::advance;
  auto peek = qre::peek;
  auto peek_prev = qre::peek_prev;

  if(!utf8)
    {
      advance = [] (const std::string &str, unsigned int &pos) -> char32_t
        { return static_cast<char32_t>(str[pos++]) & 0xFF; };
      peek = [] (const std::string &str, unsigned int pos) -> char32_t
        { return static_cast<char32_t>(str[pos]) & 0xFF; };
      peek_prev = [] (const std::string &str, unsigned int pos) -> char32_t
        { return static_cast<char32_t>(str[pos-1]) & 0xFF; };
    }

  switch(test.type)
    {
    case test_t::test_type::epsilon:
#ifdef DEBUG
      std::cerr << "epsilon" << std::endl;
#endif
      return true;
      break;

    case test_t::test_type::bol:
#ifdef DEBUG
      std::cerr << "bol" << std::endl;
#endif
      if(multiline)
        return pos == 0 || (pos < str.length() && peek_prev(str, pos) == '\n');
      else
        return pos == 0;
      break;

    case test_t::test_type::eol:
#ifdef DEBUG
      std::cerr << "eol" << std::endl;
#endif
      if(multiline)
        {
          if(pos == str.length() || (pos < str.length() && peek(str, pos) == '\n'))
            {
              advance(str, pos);
              return true;
            }
          else
            return false;
        }
      else
        return pos == str.length();
      break;

    case test_t::test_type::any:
#ifdef DEBUG
      std::cerr << "any" << std::endl;
#endif
      if(pos == str.length())
        return false; // no characters here
      else if(multiline)
        {
          if(peek(str, pos) != '\n')
            {
              advance(str, pos);
              return true;
            }
          else
            return false;
        }
      else
        {
          advance(str, pos);
          return true;
        }
      break;

    case test_t::test_type::newline:
#ifdef DEBUG
      std::cerr << "newline" << std::endl;
#endif
      if(pos < str.length())
        {
          tmp = advance(str, newpos);
          if(tmp == '\r')
            {
              if(pos+1 < str.length() && peek(str, newpos) == '\n')
                advance(str, newpos);
              result = true;
            }
          else if(tmp == '\n')
            result = true;

          if(test.neg)
            {
              if(result)
                return false;
              else
                {
                  advance(str, pos);
                  return true;
                }
            }
          else
            {
              if(result)
                {
                  pos = newpos;
                  return true;
                }
              else
                return false;
            }
        }
      else
        return false; // no characters here
      break;

    case test_t::test_type::character:
#ifdef DEBUG
      std::cerr << "character: " << std::flush;
#endif
      if(pos == str.length())
        return false; // no characters here

      tmp = advance(str, newpos);

      // single characters
      for(auto &c: test.chars)
        {
#ifdef DEBUG
          std::cerr << "\"" << (char)c << "\", " << std::flush;
#endif
          if(c == tmp)
            result = true;
        }

      // character ranges
      for(auto &r : test.ranges)
        {
#ifdef DEBUG
          std::cerr << "\"" << (char)r.begin << "\"-\"" << (char)r.end
                    << "\", " << std::flush;
#endif
          if(r.begin <= tmp && tmp <= r.end)
            result = true;
        }
#ifdef DEBUG
      std::cerr << std::endl;
#endif

      // negation
      result = result != test.neg;

      // subtraction
      for(auto &sub : test.subtractions)
        {
          unsigned int oldpos = pos;
          if(result && check(sub, str, pos, multiline, utf8, match_sofar))
            {
              pos = oldpos;
              result = false;
              break;
            }
        }

      // intersection
      for(auto &itr : test.intersections)
        {
          unsigned int oldpos = pos;
          if(result && !check(itr, str, pos, multiline, utf8, match_sofar))
            {
              result = false;
              break;
            }
          else
            pos = oldpos;
        }

      if(result)
        pos = newpos;
      return result;
      break;

    case test_t::test_type::backref:
#ifdef DEBUG
      std::cerr << "backref: " << test.backref.first << "," << test.backref.second << ": " << std::flush;
#endif
        {
          unsigned int brp = 0;
          if(test.backref.first > 0)
            brp = test.backref.first-1;
          else
            brp = match_sofar.sub.size()+test.backref.first;
          if(brp >= match_sofar.sub.size())
            return false;

          unsigned int brp2 = 0;
          if(test.backref.second > 0)
            brp2 = test.backref.second-1;
          else
            brp2 = match_sofar.sub.at(brp).size()+test.backref.second;
          if(brp2 >= match_sofar.sub.at(brp).size())
            return false;

          std::string br = match_sofar.sub.at(brp).at(brp2);
#ifdef DEBUG
          std::cerr << br << " == " << str.substr(pos, br.length()) << std::endl;
#endif
          // raw string comparison
          if(br == str.substr(pos, br.length()))
            {
              pos += br.length();
              return true;
            }
          else
            return false;
        }
      break;

    default:
      throw std::runtime_error("unknowen test type.");
      return false;
      break;
    }
}
