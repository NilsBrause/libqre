#include <regexp.hpp>

regexp::regexp(const std::string &str)
{
  std::list<symbol> syms = tokeniser(str);
  the_chain = parse_expression(syms);
  if(!the_chain)
    throw std::runtime_error("Expected expression.");
  if(syms.size() > 0)
    throw std::runtime_error("Unparsed tokens.");
}

bool regexp::match(std::string str)
{
  result.pos = 0;
  result.sub.clear();

  // FSM state
  std::shared_ptr<state_t> state = the_chain.begin;
  unsigned int pos = 0;
  unsigned int transition = 0;
  std::vector<unsigned int> captures;

  // overall match
  result.sub.push_back("");
  captures.push_back(0);

  // remeber
  unsigned int oldpos = pos;
  std::vector<unsigned int> oldcaptures = captures;

  struct history_item
  {
    std::shared_ptr<state_t> state; // current state
    unsigned int pos; // position in input stream
    unsigned int transition; // last tried transition
    std::vector<unsigned int> captures; // active capture groups
    std::vector<unsigned int> uncaptures; // active capture groups
  };

  std::list<history_item> history;

  history_item current;

  while(true)
    {
#ifdef DEBUG
      std::cerr << "state " << state << std::endl;
#endif
      // final state?
      if(state == the_chain.end)
        {
#ifdef DEBUG
          std::cerr << "accept" << std::endl << std::endl;
#endif
          return true;
        }
      // transitions left?
      else if(transition < state->transitions.size())
        {
          // save for history rewinding
          oldcaptures = captures;
          oldpos = pos;

          // close capture group
          if(state->end_capture)
            {
#ifdef DEBUG
              std::cerr << "end caputre: " << captures.back() << std::endl;
#endif
              captures.pop_back();
            }

          // open capture group
          if(state->begin_capture)
            {
              result.sub.push_back("");
              captures.push_back(result.sub.size()-1);
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
              for(auto &c : captures)
                result.sub.at(c).append(str.substr(oldpos, pos-oldpos));

              // successful test -> advance state
              history_item item = { state, oldpos, transition, oldcaptures, captures };
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
              captures = history.back().captures;

              // uncapture
              for(auto &c : history.back().uncaptures)
                result.sub.at(c).erase(result.sub.at(c).length()-(oldpos-pos),
                                       oldpos-pos);
              if(state->begin_capture)
                result.sub.pop_back();

              history.pop_back();
            }
          // no more history items -> no match
          else if(pos < str.size())
            {
#ifdef DEBUG
              std::cerr << "advance" << std::endl << std::endl;
#endif
              transition = 0;
              pos++;
              result.pos++;
            }
          else
            {
#ifdef DEBUG
              std::cerr << "reject" << std::endl << std::endl;
#endif
              return false;
            }
        }
#ifdef DEBUG
      std::cerr << std::endl;
#endif
    }

  return false;
}
