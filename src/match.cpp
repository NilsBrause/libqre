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

bool qre::operator()(const std::string &str, match &result,
                     match_flag flags) const
{
  // initialise match
  result.pos = 0;
  result.str = "";
  result.sub.clear();

  // parameters
  bool partial = (flags & match_flag::partial) != match_flag::none;
  bool fix_left = (flags & match_flag::fix_left) != match_flag::none;
  bool fix_right = (flags & match_flag::fix_right) != match_flag::none;
  bool multiline = (flags & match_flag::multiline) != match_flag::none;
  bool utf8 = (flags & match_flag::utf8) != match_flag::none;

  // possible partial matches
  std::vector<match> partials;

  // backtracking
  struct fsm_state
  {
    std::shared_ptr<state_t> state; // current state
    unsigned int pos; // position in input stream
    unsigned int transition; // last tried transition
  };
  std::list<fsm_state> history;

  // current FSM state
  fsm_state current = { the_chain.begin, 0, 0 };

  // helper
  unsigned int newpos;

  while(true)
    {
#ifdef DEBUG
      std::cerr << "state " << current.state
                << " (" << current.state->nonstop << ")" << std::endl;
      for(auto &t : current.state->transitions)
        std::cerr << "  ->" << t.state.get() << std::endl;
#endif
      // final state?
      if(current.state == the_chain.end
         // -> accept if whole string is matched or in search mode
         && (fix_right ? current.pos == str.size() : true))
        {
#ifdef DEBUG
          std::cerr << "accept" << std::endl << std::endl;
#endif
          result.type = match_type::full;
          return true;
        }
      // transitions left?
       else if(current.transition < current.state->transitions.size())
        {
          newpos = current.pos;

          // open capture group
          if(current.state->begin_capture)
            {
              if(!current.state->captures.back().named)
                result.sub[current.state->captures.back().number].push_back("");
              else
                result.named_sub[current.state->captures.back().name].push_back("");
#ifdef DEBUG
              if(current.state->captures.back().named)
                std::cerr << "new caputre: " << current.state->captures.back().name << std::endl;
              else
                std::cerr << "new caputre: #" << current.state->captures.back().number << std::endl;
#endif
            }

#ifdef DEBUG
          std::cerr << "testing transition " << current.transition+1 << "/"
                    << current.state->transitions.size() << std::endl;
#endif
          // test transition
          if(check(current.state->transitions.at(current.transition).test, str, newpos, multiline, utf8, result))
            {
#ifdef DEBUG
              std::cerr << "test succeeded" << std::endl;
#endif
              // record captures
              for(auto &c : current.state->captures)
                if(!c.named)
                  result.sub.at(c.number).back().append(str.substr(current.pos, newpos-current.pos));
                else
                  result.named_sub.at(c.name).back().append(str.substr(current.pos, newpos-current.pos));
              result.str.append(str.substr(current.pos, newpos-current.pos));

              // successful test -> advance state
              history.push_back(current);
              current.state = current.state->transitions.at(current.transition).state;
              current.transition = 0;
              current.pos = newpos;
            }
          // unsuccessfull test -> try next transition
          else
            {
#ifdef DEBUG
              std::cerr << "test failed" << std::endl;
#endif
              current.transition++;
            }
        }
      // no more transitions available
      else
        {
          // partial match?
          if(partial && current.state != the_chain.end && current.pos == str.length())
            {
              result.type = match_type::partial;
              partials.push_back(result);
            }

          // going back in history
          if(history.size() > 0)
            {
              do
                {
#ifdef DEBUG
                  std::cerr << "reverting history to "
                            << history.back().state << std::endl;
#endif
                  // save for history rewinding
                  newpos = current.pos;

                  // reverting state
                  current = history.back();
                  current.transition++; // next transition

                  // uncapture
                  for(auto &c : current.state->captures)
                    if(!c.named)
                      result.sub.at(c.number).back().erase(result.sub.at(c.number).back().length()
                                                           -(newpos-current.pos), newpos-current.pos);
                    else
                      result.named_sub.at(c.name).back().erase(result.named_sub.at(c.name).back().length()
                                                               -(newpos-current.pos), newpos-current.pos);

                  if(current.state->begin_capture)
                    {
                      if(!current.state->captures.back().named)
                        result.sub.at(current.state->captures.back().number).pop_back();
                      else
                        result.named_sub.at(current.state->captures.back().name).pop_back();
                    }

                  result.str.erase(result.str.length()-(newpos-current.pos), newpos-current.pos);

                  history.pop_back();
                }
              while(current.state->nonstop);
            }
          // try next starting point if in search mode
          else if(!fix_left && current.pos < str.size())
            {
#ifdef DEBUG
              std::cerr << "advance" << std::endl << std::endl;
#endif
              current.transition = 0;
              if(utf8)
                {
                  advance(str, current.pos);
                  result.pos = current.pos;
                }
              else
                {
                  current.pos++;
                  result.pos++;
                }
            }
          // partial match?
          else if(partials.size() > 0)
            {
#ifdef DEBUG
              std::cerr << "partial match" << std::endl << std::endl;
#endif
              result = partials.front();
              result.type = match_type::partial;
              return true;
            }
          // no match at all
          else
            {
#ifdef DEBUG
              std::cerr << "reject" << std::endl << std::endl;
#endif
              result.type = match_type::none;
              return false;
            }
        }
#ifdef DEBUG
      std::cerr << std::endl;
#endif
    }

  result.type = match_type::none;
  return false;
}
