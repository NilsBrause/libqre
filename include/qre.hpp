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

class qre
{
public:

  // user interface -----------------------------------------------------------

  enum class match_type { none, full, partial };
  enum class match_flag : uint8_t
    { none = 0, partial = 1, fix_left = 2, fix_right = 4, multiline = 8, utf8 = 16, longest = 32 };

  struct match
  {
    match_type type; // type of match
    unsigned int pos; // position of match
    std::string str; // overall match
    std::map<uint32_t, std::vector<std::string>> sub; // sub matches
    std::map<std::string, std::vector<std::string>> named_sub; // named sub matches
    operator bool() { return type == match_type::full; }
  };

  qre();
  qre(const std::string &regex); // contruct a regular expression
  qre(const qre &q);
  qre(qre &&q);
  qre &operator=(const qre &p);
  qre &operator=(qre &&p);
  ~qre();
  bool operator()(const std::string &str, match &result,
                  match_flag flags = match_flag::none) const; // matching function

private:

  // UTF-8 handling -----------------------------------------------------------

  static char32_t advance(const std::string &str, unsigned int &pos);
  static char32_t peek(const std::string &str, unsigned int pos);
  static char32_t peek_prev(const std::string &str, unsigned int pos);
  std::u32string utf8toutf32(const std::string &str) const;
  std::string utf32toutf8(char32_t ch) const;
  std::string utf32toutf8(const std::u32string &str) const;

  // Helper structs -----------------------------------------------------------

  struct capture_t
  {
    bool named;
    signed int number;
    std::string name;
  };

  struct char_range
  {
    char32_t begin;
    char32_t end;

    bool operator==(const char_range &r) const;
    bool operator<(const char_range &r) const; // for std::set
  };

  // Tests --------------------------------------------------------------------

  struct test_t
  {
    enum class test_type
    { epsilon, any, bol, eol, newline, backref, character };

    test_type type;
    bool neg = false;
    std::set<char32_t> chars;
    std::set<char_range> ranges;
    std::vector<test_t> subtractions;
    std::vector<test_t> intersections;
    std::pair<capture_t, signed int> backref;
  };

  bool check(const test_t &test, const std::string &str,
             unsigned int &pos, bool multiline, bool utf8,
             match &match_sofar) const;

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
    std::string name;
    bool capture = true;
    bool atomic = false;
    bool named = false;
  };

  // regexp parse functions
  test_t read_char_class(const std::u32string &str, unsigned int &pos, bool leading_backet = true) const;
  bool read_range(const std::u32string &str, unsigned int &pos, range_t &r) const;
  std::pair<capture_t, signed int> read_backref(const std::u32string &str, unsigned int &pos) const;
  std::u32string read_cg_name(const std::u32string str, unsigned int &pos) const;
  std::list<symbol> tokeniser(const std::u32string &str) const;

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
    bool nonstop = false; // keep backtracking
    std::vector<capture_t> captures; // list of active capture groups
    std::vector<transition_t> transitions;
    std::vector<std::weak_ptr<state_t> > prev;
  };

  // espiloin transition
  void epsilon(std::shared_ptr<state_t> a, std::shared_ptr<state_t> b) const;

  // merges contents of src into contents of dst
  void merge_state(std::shared_ptr<state_t> &dst,
                   std::shared_ptr<state_t> &src) const;

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

  signed int id = 0; // current capture id
  std::vector<capture_t> captures; // list of active capture groups
  bool nonstop = false; // keep backtracking in atomic groups

  chain_t parse_atom(std::list<symbol> &syms);
  chain_t parse_factor(std::list<symbol> &syms);
  chain_t parse_term(std::list<symbol> &syms);
  chain_t parse_expression(std::list<symbol> &syms);

  chain_t the_chain;
};

// make match_flag behave like a normal enumeration
qre::match_flag operator|(const qre::match_flag &f1, const qre::match_flag &f2);
qre::match_flag operator&(const qre::match_flag &f1, const qre::match_flag &f2);

#endif // QRE_HPP
