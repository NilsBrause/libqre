#include <regexp.hpp>

regexp::chain_t regexp::parse_atom(std::list<symbol> &syms)
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
      syms.pop_front();
      return result;
    }
  // ...or a parenthesised group expression
  else if(syms.size() && syms.front().type == symbol::type_t::lparan)
    {
      bool capture = syms.front().capture;
      syms.pop_front();
      chain_t result = parse_expression(syms);
      if(!result)
        throw std::runtime_error("Expected expression after '('.");
      if(syms.size() && syms.front().type != symbol::type_t::rparan)
        throw std::runtime_error("Expected ')'.");
      syms.pop_front();
      if(capture)
        {
          // Append/Prepend epsilon transition with capture information
          std::shared_ptr<state_t> tmp = std::make_shared<state_t>();
          epsilon(tmp, result.begin);
          result.begin = tmp;
          result.begin->begin_capture = true;
          tmp = std::make_shared<state_t>();
          epsilon(result.end, tmp);
          result.end->end_capture = true;
          result.end = tmp;
        }
      return result;
    }
  return chain_t();
}

regexp::chain_t regexp::parse_factor(std::list<symbol> &syms)
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
          epsilon(atom.end, atom.begin);
          epsilon(atom.end, end);
          epsilon(pos, end);
          pos = end;
        }
      // append maximum
      else
        for(; c < range.end; c++)
          {
            chain_t tmp = clone(atom);
            epsilon(tmp.begin, tmp.end);
            merge_state(pos, tmp.begin);
            pos = tmp.end;
          }

      result.end = pos;

      return result;
    }
  return chain_t();
}

regexp::chain_t regexp::parse_term(std::list<symbol> &syms)
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

regexp::chain_t regexp::parse_expression(std::list<symbol> &syms)
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
