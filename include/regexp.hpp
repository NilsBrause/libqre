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

/*! \brief Regular Expression Matcher
 *
 * Features:
 * - Characters:
 *   + Literal characters
 *   + Any character: .
 *   + Escaped metacharacters: \( \) \[ \] \{ \} \? \* \+ \. \^ \$ \\
 *   + Control characters: \0, \a, \b, \e, \f, \n, \r, \t, \v, \cA-\cZ
 *   + Newline: \R (CR/CRLF/LF)
 *   + No newline: \N
 *   + Octal numbers: \o{...}
 *   + Hex numbers: \x00-\xFF, \x{...}
 * - Anchors:
 *   + Beginning of string: ^, \A, \`
 *   + End of string: $, \Z, \'
 * - Character classes
 *   + Single characters: e.g. [abc]
 *   + Character ranges: e.g. [0-9]
 *   + Negation: e.g. [^abc]
 *   + Literal ']' at the beginning: e.g. []abc], [^]abc]
 *   + Literal - outside ranges: e.g [-0-9-A-F-]
 *   + Subtractions: e.g. [a-z-[ij]]
 *   + TODO: Intersections
 * - Alternation: abc|def
 * - Quantifiers:
 *   + Optional: a?
 *   + Zero or more: a*
 *   + One or more: a+
 *   + Fixed: {n}
 *   + n or more: {n,}
 *   + Up to m: {,m}
 *   + Range: {n,m}
 *   + Greedy quanitfiers (default)
 * - Groups:
 *   + Capturing: (abc)
 *   + Non-capturing: (?:abc)
 *
 * Not supported:
 * - Literal '\' in a character class. It always escapes.
 * - Shorthands, POSIX classes and the like
 * - "multiline mode". Use \R instead.
 * - Word boundaries
 * - Lazy and possessive quanitfiers
 * - Unicode
 * - Named groups and Backraferences
 * - Special Groups
 * - Mode mofifiers
 * - Recursion and subroutines
 */
class regexp
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
    std::basic_string<char_t> sequence;
    std::vector<test_t> subtractions;

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
    /* Grammar:
     * expression = term, { "|", term }
     * term       = factor, { factor }
     * factor     = atom, [ range ]
     * atom       = group | test
     * group      = "(", expression, ")"
     */
    enum class type_t { test, range, alt, lparan, rparan };
    type_t type;
    test_t test;
    range_t range;
    bool capture = true;
  };

  // regexp parse functions
  char_t read_escape(const std::string &str, unsigned int &pos);
  test_t read_escape_sequence(const std::string &str, unsigned int &pos);
  test_t read_char_class(const std::string &str, unsigned int &pos);
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

  chain_t parse_atom(std::list<symbol> &syms);
  chain_t parse_factor(std::list<symbol> &syms);
  chain_t parse_term(std::list<symbol> &syms);
  chain_t parse_expression(std::list<symbol> &syms);

  chain_t the_chain;

public:

  // user interface -----------------------------------------------------------

  struct match_result_t
  {
    unsigned int pos;
    std::vector<std::string> sub;
  };

  match_result_t result;

  regexp(const std::string &str);
  bool match(std::string str);
};
