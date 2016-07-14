#include <cassert>
#include <iostream>
#include <qre.hpp>

int main()
{
  qre::match result;

  // Literal characters
  qre r00("abc");
  assert(r00("abc", result));
  assert(result.str == "abc");
  assert(!r00("def", result));

  // The dot
  qre r01("a.c");
  assert(r01("abc", result));
  assert(result.str == "abc");

  // escaped characters
  qre r02("\\(\\)\\[\\]\\{\\}\\?\\*\\+\\.\\^\\$\\B\\\\");
  assert(r02("()[]{}?*+.^$\\\\", result));
  assert(result.str == "()[]{}?*+.^$\\\\");

  // control characters
  qre r03("\\a\\b\\e\\f\\n\\r\\t\\v\\cZ");
  assert(r03("\a\b\x1b\f\n\r\t\v\x1a", result));
  assert(result.str == "\a\b\x1b\f\n\r\t\v\x1a");

  // newline
  qre r04("a\\Rb");
  assert(r04("a\nb", result));
  assert(result.str == "a\nb");
  assert(r04("a\r\nb", result));
  assert(result.str == "a\r\nb");
  assert(r04("a\rb", result));
  assert(result.str == "a\rb");
  assert(!r04("ab", result));

  // no newline
  qre r05("a\\Nc");
  assert(r05("abc", result));
  assert(result.str == "abc");
  assert(!r05("a\nc", result));

  // octal and hex numbers
  qre r06("\\o{141}\\x62\\u{63}");
  assert(r06("abc", result));
  assert(result.str == "abc");
  assert(!r00("def", result));

  // anchors
  qre r07("^abc$");
  assert(r07("abc", result));
  assert(result.str == "abc");
  assert(!r07("0abc1", result));
  qre r08("\\Aabc\\Z");
  assert(r08("abc", result));
  assert(result.str == "abc");
  assert(!r08("0abc1", result));
  qre r09("\\`abc\\'");
  assert(r09("abc", result));
  assert(result.str == "abc");
  assert(!r09("0abc1", result));

  // charachter classes with single characters
  qre r10("a[abc]c");
  assert(r10("abc", result));
  assert(result.str == "abc");
  assert(!r10("adc", result));

  // charachter classes with ranges
  qre r11("a[a-c]c");
  assert(r11("abc", result));
  assert(result.str == "abc");
  assert(!r11("adc", result));

  // negation
  qre r12("a[^ij]c");
  assert(r12("abc", result));
  assert(result.str == "abc");
  assert(!r12("aic", result));

  // literal '-' and ']' in character classes
  qre r13("a[]-0-9-a-z-]c");
  assert(r13("abc", result));
  assert(result.str == "abc");
  assert(r13("a8c", result));
  assert(result.str == "a8c");
  assert(r13("a]c", result));
  assert(result.str == "a]c");
  assert(r13("a-c", result));
  assert(result.str == "a-c");
  assert(!r13("a_c", result));

  // character class subtraction
  qre r14("a[a-z-[ij]]c");
  assert(r14("abc", result));
  assert(result.str == "abc");
  assert(!r14("aic", result));

  // alternations
  qre r15("abc|def");
  assert(r15("abc", result));
  assert(result.str == "abc");
  assert(r15("def", result));
  assert(result.str == "def");
  assert(!r15("ghi", result));

  // zero or one quantifier
  qre r16("ab?c");
  assert(r16("ac", result));
  assert(result.str == "ac");
  assert(r16("abc", result));
  assert(result.str == "abc");
  assert(!r16("abbc", result));

  // zero or more quantifier
  qre r17("ab*c");
  assert(r17("ac", result));
  assert(result.str == "ac");
  assert(r17("abc", result));
  assert(result.str == "abc");
  assert(r17("abbc", result));
  assert(result.str == "abbc");
  assert(!r17("adc", result));

  // one or more quantifier
  qre r18("ab+c");
  assert(!r18("ac", result));
  assert(r18("abc", result));
  assert(result.str == "abc");
  assert(r18("abbc", result));
  assert(result.str == "abbc");

  // fixed quantifier
  qre r19("ab{2}c");
  assert(!r19("abc", result));
  assert(r19("abbc", result));
  assert(result.str == "abbc");
  assert(!r19("abbbc", result));

  // n or more quantifier
  qre r20("ab{2,}c");
  assert(!r20("abc", result));
  assert(r20("abbc", result));
  assert(result.str == "abbc");
  assert(r20("abbbc", result));
  assert(result.str == "abbbc");

  // up to m quantifier
  qre r21("ab{,2}c");
  assert(r21("abc", result));
  assert(result.str == "abc");
  assert(r21("abbc", result));
  assert(result.str == "abbc");
  assert(!r21("abbbc", result));

  // range quatifier
  qre r22("ab{1,2}c");
  assert(!r22("ac", result));
  assert(r22("abc", result));
  assert(result.str == "abc");
  assert(r22("abbc", result));
  assert(result.str == "abbc");
  assert(!r22("abbbc", result));

  // capture groups
  qre r23("a(.)c");
  assert(r23("abc", result));
  assert(result.str == "abc");
  assert(result.sub.size() == 1);
  assert(result.sub[0].back() == "b");

  // non-capture groups
  qre r24("a(?:.)c");
  assert(r24("abc", result));
  assert(result.str == "abc");
  assert(result.sub.size() == 0);

  // varbatim characters
  qre r25("\\Q?*+\\E");
  assert(r25("?*+", result));

  // partial matches
  qre r26("abcd|abce");
  assert(!r26("abc", result));
  assert(r26("abc", result, qre::match_flag::partial));
  assert(result.str == "abc");

  // multiple captures
  qre r27("a(..)+z");
  assert(r27("abcdefgz", result));
  assert(result.sub[0].size() == 3);
  assert(result.sub[0][0] == "bc");
  assert(result.sub[0][1] == "de");
  assert(result.sub[0][2] == "fg");

  // character class intersection
  qre r28a("a[a-z&&[^ij]]c");
  assert(r28a("abc", result));
  assert(result.str == "abc");
  assert(!r28a("aic", result));
  qre r28b("a[a-z&&a-hk-z]c");
  assert(r28b("abc", result));
  assert(result.str == "abc");
  assert(!r28b("aic", result));

  // lazy quantifiers
  qre r29a("ab??c");
  assert(r29a("ac", result));
  assert(result.str == "ac");
  assert(r29a("abc", result));
  assert(result.str == "abc");
  assert(!r29a("abbc", result));
  qre r29b("ab*?c");
  assert(r29b("ac", result));
  assert(result.str == "ac");
  assert(r29b("abc", result));
  assert(result.str == "abc");
  assert(r29b("abbc", result));
  assert(result.str == "abbc");
  assert(!r29b("adc", result));

  // search
  qre r30("[0-9]+");
  assert(r30("abc123def", result));
  assert(result.str == "123");
  assert(result.pos == 3);
  assert(!r30("abc123def", result, qre::match_flag::fix_left));
  assert(!r30("abc123def", result, qre::match_flag::fix_right));
  assert(r30("123def", result, qre::match_flag::fix_left));
  assert(r30("abc123", result, qre::match_flag::fix_right));
  assert(r30("123", result, qre::match_flag::fix_left | qre::match_flag::fix_right));

  // atomic group
  qre r31("a(?>bc|b)d");
  assert(r31("abcd", result));
  assert(result.str == "abcd");
  assert(!r31("abd", result));

  // multiline mode
  qre r32("^123$^abc$^456$");
  assert(r32("123\nabc\n456", result, qre::match_flag::multiline));
  assert(!r32("123\nabc\n456", result));

  // unicode code points
  qre r33a(u8"€");
  assert(r33a(u8"€", result, qre::match_flag::utf8));
  assert(result.str == u8"€");
  assert(!r33a(u8"€", result));
  qre r33b("\\u{20AC}");
  assert(r33b(u8"€", result, qre::match_flag::utf8));
  assert(result.str == u8"€");
  assert(!r33b(u8"€", result));
  qre r33c("\\xE2\\x82\\xAC");
  assert(r33c(u8"€", result));
  assert(result.str == u8"€");
  assert(!r33c(u8"€", result, qre::match_flag::utf8));

  // backreferences
  qre r34a("(.)(.)\\g<2>\\g<1>");
  assert(r34a("abba", result));
  assert(result.str == "abba");
  assert(result.sub.size() == 2);
  assert(result.sub[0].back() == "a");
  assert(result.sub[1].back() == "b");
  qre r34b("(.)(.)\\g<-1>\\g<-2>");
  assert(r34b("abba", result));
  assert(result.str == "abba");
  assert(result.sub.size() == 2);
  assert(result.sub[0].back() == "a");
  assert(result.sub[1].back() == "b");
  qre r34c("(.){2,}\\g<1,1>\\g<1,2>");
  assert(r34c("abcab", result));
  assert(result.str == "abcab");
  assert(result.sub.size() == 1);
  assert(result.sub[0].size() == 3);
  assert(result.sub[0][0] == "a");
  assert(result.sub[0][1] == "b");
  assert(result.sub[0][2] == "c");
  qre r34d("(.){2,}\\g<1,-2>\\g<1,-1>");
  assert(r34d("abcbc", result));
  assert(result.str == "abcbc");
  assert(result.sub.size() == 1);
  assert(result.sub[0].size() == 3);
  assert(result.sub[0][0] == "a");
  assert(result.sub[0][1] == "b");
  assert(result.sub[0][2] == "c");
  qre r34e("(.)\\1");
  assert(r34e("aa", result));
  assert(result.str == "aa");
  assert(result.sub.size() == 1);
  assert(result.sub[0].size() == 1);
  assert(result.sub[0][0] == "a");
  qre r34f("(.)\\-1");
  assert(r34f("aa", result));
  assert(result.str == "aa");
  assert(result.sub.size() == 1);
  assert(result.sub[0].size() == 1);
  assert(result.sub[0][0] == "a");

  // forward references
  qre r35("(?:\\g<1,2>def|(abc))+");
  assert(r35("abcabcabcdef", result));
  assert(result.str == "abcabcabcdef");
  assert(result.sub.size() == 1);
  assert(result.sub[0].size() == 2);
  assert(result.sub[0][0] == "abc");
  assert(result.sub[0][1] == "abc");

  // more than one capture group
  qre r96("a(bc)d(ef)g");
  assert(r96("abcdefg", result));
  assert(result.sub.size() == 2);
  assert(result.sub[0].back() == "bc");
  assert(result.sub[1].back() == "ef");
  
  // escapes inside character classes
  qre r97("a[b\t]c");
  assert(r97("a\tc", result));
  
  // backtracking test
  qre r98("a(?:bc|b)c");
  assert(r98("abc", result));

  // quantified groups test
  qre r99("a(?:(?:bc){,2}de)+f");
  assert(r99("abcbcdebcdef", result));

  return 0;
}
