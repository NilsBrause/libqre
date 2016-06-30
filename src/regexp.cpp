#include <regexp.hpp>

regexp::regexp(const std::string &str)
  : id(0)
{
  std::list<symbol> syms = tokeniser(str);
  the_chain = parse_expression(syms);

  if(!the_chain)
    throw std::runtime_error("Expected expression.");
  if(syms.size() > 0)
    throw std::runtime_error("Unparsed tokens.");
}

bool regexp::operator()(const std::string &str, match &result,
                        std::set<match_flag> flags) const
{
  result.pos = 0;
  result.str = "";
  result.sub.clear();

  std::vector<match> partials;

  // FSM state
  std::shared_ptr<state_t> state = the_chain.begin;
  unsigned int pos = 0;
  unsigned int transition = 0;

  // remeber
  unsigned int oldpos = pos;
  std::vector<unsigned int> oldcaptures = captures;

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
      std::cerr << "state " << state << std::endl;
      for(auto &t : state->transitions)
        std::cerr << "  ->" << t.state.get() << std::endl;
#endif
      // final state?
      if(state == the_chain.end)
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
          if(state->transitions.at(transition).test.check(str, pos))
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
          if(pos == str.length() && flags.find(match_flag::partial)
             != flags.end())
            {
              result.type = match_type::partial;
              partials.push_back(result);
            }

#ifdef DEBUG
          std::cerr << "reverting history" << std::endl;
#endif
          // going back in history
          if(history.size() > 0)
            {
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
          // try next starting point
          else if(pos < str.size())
            {
#ifdef DEBUG
              std::cerr << "advance" << std::endl << std::endl;
#endif
              transition = 0;
              pos++;
              result.pos++;
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
