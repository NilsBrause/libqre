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

  // FSM state
  std::shared_ptr<state_t> state = the_chain.begin;
  unsigned int pos = 0;
  unsigned int transition = 0;

  // remeber
  unsigned int oldpos = pos;
  std::vector<unsigned int> oldcaptures = captures;

  // backtracking
  struct history_item
  {
    std::shared_ptr<state_t> state; // current state
    unsigned int pos; // position in input stream
    unsigned int transition; // last tried transition
  };

  std::list<history_item> history;

  history_item current;

  while(true)
    {
#ifdef DEBUG
      std::cerr << "state " << state
                << " (" << state->nonstop << ")" << std::endl;
      for(auto &t : state->transitions)
        std::cerr << "  ->" << t.state.get() << std::endl;
#endif
      // final state?
      if(state == the_chain.end
         // -> accept if whole string is matched or in search mode
         && (fix_right ? pos == str.size() : true))
        {
#ifdef DEBUG
          std::cerr << "accept" << std::endl << std::endl;
#endif
          result.type = match_type::full;
          return true;
        }
      // transitions left?
      else if(transition < state->transitions.size())
        {
          // save for history rewinding
          oldpos = pos;

          // close capture group
          if(state->end_capture)
            {
#ifdef DEBUG
              std::cerr << "end caputre: " << captures.back() << std::endl;
#endif
            }

          // open capture group
          if(state->begin_capture)
            {
              result.sub[state->captures.back()].push_back("");
#ifdef DEBUG
              std::cerr << "new caputre: " << captures.back() << std::endl;
#endif
            }

#ifdef DEBUG
          std::cerr << "testing transition " << transition+1 << "/"
                    << state->transitions.size() << std::endl;
#endif
          // test transition
          if(check(state->transitions.at(transition).test, str, pos, multiline, utf8, result))
            {
#ifdef DEBUG
              std::cerr << "test succeeded" << std::endl;
#endif
              // record captures
              for(auto &c : state->captures)
                result.sub.at(c).back().append(str.substr(oldpos, pos-oldpos));
              result.str.append(str.substr(oldpos, pos-oldpos));

              // successful test -> advance state
              history_item item = { state, oldpos, transition };
              history.push_back(item);
              state = state->transitions.at(transition).state;
              transition = 0;
            }
          // unsuccessfull test -> try next transition
          else
            {
#ifdef DEBUG
              std::cerr << "test failed" << std::endl;
#endif
              transition++;
            }
        }
      // no more transitions available
      else
        {
          // partial match?
          if(partial && state != the_chain.end && pos == str.length())
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
                  // reverting state
                  oldpos = pos;
                  state = history.back().state;
                  pos = history.back().pos;
                  transition = history.back().transition + 1; // next transition

                  // uncapture
                  for(auto &c : state->captures)
                    result.sub.at(c).back().erase(result.sub.at(c).back().length()
                                                  -(oldpos-pos), oldpos-pos);
                  if(state->begin_capture)
                    result.sub.at(state->captures.back()).pop_back();
                  result.str.erase(result.str.length()-(oldpos-pos), oldpos-pos);

                  history.pop_back();
                }
              while(state->nonstop);
            }
          // try next starting point if in search mode
          else if(!fix_left && pos < str.size())
            {
#ifdef DEBUG
              std::cerr << "advance" << std::endl << std::endl;
#endif
              transition = 0;
              if(utf8)
                {
                  advance(str, pos);
                  result.pos = pos;
                }
              else
                {
                  pos++;
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
