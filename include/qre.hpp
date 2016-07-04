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

#ifndef QRE_HPP
#define QRE_HPP

#include <iostream>
#include <cassert>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <stack>
#include <type_traits>
#include <vector>

//#define DEBUG

class qre
{
private:

  // 32 bit character type with the same signedness as 'char'
  typedef std::conditional<std::is_signed<char>::value,
                           int32_t, uint32_t>::type char_t;

  // Tests --------------------------------------------------------------------

  struct test_t
  {
    struct char_range
    {
      char_t begin;
      char_t end;

      bool operator==(const char_range &r) const;
      bool operator<(const char_range &r) const;
    };

    enum class test_type
    { epsilon, any, bol, eol, newline, character };

    test_type type;
    bool neg = false;
    std::set<char_t> chars;
    std::set<char_range> ranges;
    std::vector<test_t> subtractions;
    std::vector<test_t> intersections;

    bool check(const std::string &str, unsigned int &pos,
               bool multiline = false) const;
  };

  // tokenizer -------------------------------------------------------------------

  struct range_t
  {
    uint32_t begin = 1;
    uint32_t end = 1;
    bool infinite = false;
  };

  struct symbol
  {
    enum class type_t
    { test, range, qmark, star, plus, alt, lparan, rparan };
    type_t type;
    test_t test;
    range_t range;
    bool capture = true;
  };

  // regexp parse functions
  char_t read_escape(const std::string &str, unsigned int &pos);
  test_t read_escape_sequence(const std::string &str, unsigned int &pos);
  test_t read_char_class(const std::string &str, unsigned int &pos, bool leading_backet = true);
  bool read_range(const std::string &str, unsigned int &pos, range_t &r);

  std::list<symbol> tokeniser(const std::string &str);

  // state machine ------------------------------------------------------------

  struct state_t;

  struct transition_t
  {
    test_t test;
    std::shared_ptr<state_t> state;
  };

  struct state_t
  {
    bool begin_capture = false;
    bool end_capture = false;
    std::vector<uint32_t> captures; // list of active capture groups
    std::vector<transition_t> transitions;
    std::vector<std::weak_ptr<state_t> > prev;
  };

  // espiloin transition
  static void epsilon(std::shared_ptr<state_t> a, std::shared_ptr<state_t> b);

  // replaces oldstate pointer with newstate pointer
  static void replace_state(std::shared_ptr<state_t> &oldstate,
                            std::shared_ptr<state_t> &newstate);

  // merges contents of src into contents of dst
  static void merge_state(std::shared_ptr<state_t> &dst,
                          std::shared_ptr<state_t> &src);

  // state chain
  struct chain_t
  {
    std::shared_ptr<state_t> begin;
    std::shared_ptr<state_t> end;

    operator bool()
    {
      return begin && end;
    }
  };

  // clone a state chain
  static chain_t clone(chain_t chain);

  // parser -------------------------------------------------------------------

  uint32_t id = 0; // current capture id
  std::vector<uint32_t> captures; // list of active capture groups

  chain_t parse_atom(std::list<symbol> &syms);
  chain_t parse_factor(std::list<symbol> &syms);
  chain_t parse_term(std::list<symbol> &syms);
  chain_t parse_expression(std::list<symbol> &syms);

  chain_t the_chain;

public:

  // user interface -----------------------------------------------------------

  enum class match_type { none, full, partial };
  enum class match_flag : uint8_t
  { none = 0, partial = 1, fix_left = 2, fix_right = 4 };

  struct match
  {
    match_type type; // type of match
    unsigned int pos; // position of match
    std::string str; // overall match
    std::map<uint32_t, std::vector<std::string>> sub; // sub matches
    operator bool() { return type == match_type::full; }
  };

  qre(const std::string &str);
  bool operator()(const std::string &str, match &result,
                  match_flag flags = match_flag::none) const;
};

qre::match_flag operator|(const qre::match_flag &f1, const qre::match_flag &f2);
qre::match_flag operator&(const qre::match_flag &f1, const qre::match_flag &f2);

#endif // QRE_HPP
