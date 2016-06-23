#include <regexp.hpp>

void regexp::epsilon(std::shared_ptr<state_t> a, std::shared_ptr<state_t> b)
{
  transition_t t;
  t.test.type = test_t::test_type::epsilon;
  t.state = b;
  a->transitions.push_back(t);
  b->prev.push_back(a);
}

void regexp::replace_state(std::shared_ptr<state_t> &oldstate,
                           std::shared_ptr<state_t> &newstate)
{
  for(auto &p : oldstate->prev)
    {
      for(auto &t : p.lock()->transitions)
        if(t.state == oldstate)
          t.state = newstate;
      newstate->prev.push_back(p);
    }
  newstate->begin_capture = oldstate->begin_capture;
  newstate->end_capture = oldstate->end_capture;
  oldstate = newstate;
}

void regexp::merge_state(std::shared_ptr<state_t> &dst,
                         std::shared_ptr<state_t> &src)
{
  assert(src->prev.size() == 0);
  dst->transitions.insert(dst->transitions.end(),
                          src->transitions.begin(),
                          src->transitions.end());
  for(auto &t : dst->transitions)
    for(auto &p : t.state->prev)
      if(p.lock() == src)
        p = dst;
  dst->begin_capture |= src->begin_capture;
  dst->end_capture |= src->end_capture;
}

regexp::chain_t regexp::clone(chain_t chain)
{
  std::map<std::shared_ptr<state_t>, std::shared_ptr<state_t>> state_map;

  std::function<std::shared_ptr<state_t>(std::shared_ptr<state_t>)> clone
    = [&] (std::shared_ptr<state_t> state) -> std::shared_ptr<state_t>
    {
      // state already cloned?
      auto it = state_map.find(state);
      if(it != state_map.end())
        return it->second;

      // clone state
      std::shared_ptr<state_t> newstate = std::make_shared<state_t>();
      state_map[state] = newstate;
      newstate->begin_capture = state->begin_capture;
      newstate->end_capture = state->end_capture;
      for(auto &t : state->transitions)
        {
          transition_t t2;
          t2.test = t.test;
          t2.state = clone(t.state);
          newstate->transitions.push_back(t2);
          t2.state->prev.push_back(newstate);
        }

      return newstate;
    };

  chain_t result;
  result.begin = clone(chain.begin);
  result.end = state_map.at(chain.end);
  return result;
}
