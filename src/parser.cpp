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

qre::chain_t qre::parse_atom(std::list<symbol> &syms)
{
  // an atom it either a single test...
  if(syms.size() && syms.front().type == symbol::type_t::test)
    {
      chain_t result;
      result.begin = std::make_shared<state_t>();
      result.end = std::make_shared<state_t>();
      transition_t transition;
      transition.test = syms.front().test;
      transition.state = result.end;
      result.begin->transitions.push_back(transition);
      result.end->prev.push_back(result.begin);
      result.begin->captures = captures;
      result.begin->nonstop = nonstop;
      syms.pop_front();
      return result;
    }
  // ...or a parenthesised group expression
  else if(syms.size() && syms.front().type == symbol::type_t::lparan)
    {
      // get options
      bool capture = syms.front().capture;
      bool atomic = syms.front().atomic;
      bool named = syms.front().named;
      std::string name = syms.front().name;
      syms.pop_front();

      // new capture group
      if(capture)
        {
          if(named)
            captures.push_back({ true, 0, name });
          else
            captures.push_back({ false, id++, "" });
        }

      // start atomic group
      bool oldnonstop = nonstop;
      if(atomic)
        nonstop = true;

      // parse group contents
      chain_t result = parse_expression(syms);
      if(!result)
        throw std::runtime_error("Expected expression after '('.");
      if(syms.size() && syms.front().type != symbol::type_t::rparan)
        throw std::runtime_error("Expected ')'.");
      syms.pop_front();

      // end of atomic group
      if(atomic)
        nonstop = oldnonstop;

      // Append/Prepend epsilon transition
      std::shared_ptr<state_t> tmp = std::make_shared<state_t>();
      tmp->nonstop = nonstop;
      epsilon(tmp, result.begin);
      result.begin = tmp;
      tmp = std::make_shared<state_t>();
      //tmp->nonstop = nonstop;
      epsilon(result.end, tmp);
      result.end = tmp;

      // capture information
      if(capture)
        {
          result.begin->begin_capture = true;
          result.begin->captures = captures;
          captures.pop_back();
        }

      return result;
    }
  return chain_t();
}

qre::chain_t qre::parse_factor(std::list<symbol> &syms)
{
  chain_t result;
  result.begin = std::make_shared<state_t>();

  // parse an atom
  chain_t atom = parse_atom(syms);
  if(atom)
    {
      // check for modifier
      range_t range;
      if(syms.size() && syms.front().type == symbol::type_t::range)
        {
          range = syms.front().range;
          syms.pop_front();
        }
      else if(syms.size() && syms.front().type == symbol::type_t::qmark)
        {
          range.begin = 0;
          range.end = 1;
          syms.pop_front();
        }
      else if(syms.size() && syms.front().type == symbol::type_t::star)
        {
          range.begin = 0;
          range.infinite = true;
          syms.pop_front();
        }
      else if(syms.size() && syms.front().type == symbol::type_t::plus)
        {
          range.begin = 1;
          range.infinite = true;
          syms.pop_front();
        }

      bool lazy = false;
      if(syms.size() && syms.front().type == symbol::type_t::qmark)
        {
          lazy = true;
          syms.pop_front();
        }

      // chain position
      std::shared_ptr<state_t> pos = result.begin;

      // append minimum
      unsigned int c = 0;
      for(; c < range.begin; c++)
        {
          chain_t tmp = clone(atom);
          merge_state(pos, tmp.begin);
          pos = tmp.end;
        }

      // append infinity
      if(range.infinite)
        {
          std::shared_ptr<state_t> end = std::make_shared<state_t>();
          epsilon(pos, atom.begin);
          if(lazy)
            {
              epsilon(atom.end, end);
              epsilon(atom.end, atom.begin);
            }
          else
            {
              epsilon(atom.end, atom.begin);
              epsilon(atom.end, end);
            }
          epsilon(pos, end);
          pos = end;
        }
      // append maximum
      else
        for(; c < range.end; c++)
          {
            chain_t tmp = clone(atom);
            if(lazy)
              {
                std::shared_ptr<state_t> begin = std::make_shared<state_t>();
                // make sure the epsilon comes before the atom.
                epsilon(begin, tmp.end);
                merge_state(begin, tmp.begin);
                tmp.begin = begin;
              }
            else
              epsilon(tmp.begin, tmp.end);
            merge_state(pos, tmp.begin);
            pos = tmp.end;
          }

      result.end = pos;

      return result;
    }
  return chain_t();
}

qre::chain_t qre::parse_term(std::list<symbol> &syms)
{
  chain_t chain = parse_factor(syms);
  if(!chain)
    return chain_t();

  // extend chain
  chain_t tmp;
  while((tmp = parse_factor(syms)))
    {
      //epsilon(chain.end, tmp.begin);
      merge_state(chain.end, tmp.begin);
      chain.end = tmp.end;
    }

  return chain;
}

qre::chain_t qre::parse_expression(std::list<symbol> &syms)
{
  chain_t tmp = parse_term(syms);
  if(!tmp)
    return chain_t();

  // add alternations
  if(syms.size() && syms.front().type == symbol::type_t::alt)
    {
      chain_t result;
      result.begin = std::make_shared<state_t>();
      result.end = std::make_shared<state_t>();

      epsilon(result.begin, tmp.begin);
      epsilon(tmp.end, result.end);

      while(syms.size() && syms.front().type == symbol::type_t::alt)
        {
          syms.pop_front();
          tmp = parse_term(syms);
          if(!tmp)
            throw std::runtime_error("Expected expression after '|'.");

          epsilon(result.begin, tmp.begin);
          epsilon(tmp.end, result.end);
        }
      return result;
    }
  else
    return tmp;
}
