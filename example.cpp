#include <cassert>
#include <iostream>
#include <regexp.hpp>

int main()
{
  regexp::match result;

  // Literal characters
  regexp r00("abc");
  assert(r00("abc", result));
  assert(result.str == "abc");
  assert(!r00("def", result));

  // The dot
  regexp r01("a.c");
  assert(r01("abc", result));
  assert(result.str == "abc");

  // escaped characters
  regexp r02("\\(\\)\\[\\]\\{\\}\\?\\*\\+\\.\\^\\$\\B\\\\");
  assert(r02("()[]{}?*+.^$\\\\", result));
  assert(result.str == "()[]{}?*+.^$\\\\");

  // control characters
  regexp r03("\\a\\b\\e\\f\\n\\r\\t\\v\\cZ");
  assert(r03("\a\b\x1b\f\n\r\t\v\x1a", result));
  assert(result.str == "\a\b\x1b\f\n\r\t\v\x1a");

  // newline
  regexp r04("a\\Rb");
  assert(r04("a\nb", result));
  assert(result.str == "a\nb");
  assert(r04("a\r\nb", result));
  assert(result.str == "a\r\nb");
  assert(r04("a\rb", result));
  assert(result.str == "a\rb");
  assert(!r04("ab", result));

  // no newline
  regexp r05("a\\Nb");
  assert(r05("ab", result));
  assert(result.str == "ab");
  assert(!r05("a\nb", result));

  // octal and hex numbers
  regexp r06("\\o{141}\\x62\\x{63}");
  assert(r06("abc", result));
  assert(result.str == "abc");
  assert(!r00("def", result));

  // anchors
  regexp r07("^abc$");
  assert(r07("abc", result));
  assert(result.str == "abc");
  assert(!r07("0abc1", result));
  regexp r08("\\Aabc\\Z");
  assert(r08("abc", result));
  assert(result.str == "abc");
  assert(!r08("0abc1", result));
  regexp r09("\\`abc\\'");
  assert(r09("abc", result));
  assert(result.str == "abc");
  assert(!r09("0abc1", result));

  // charachter classes with single characters
  regexp r10("a[abc]c");
  assert(r10("abc", result));
  assert(result.str == "abc");
  assert(!r10("adc", result));

  // charachter classes with ranges
  regexp r11("a[a-c]c");
  assert(r11("abc", result));
  assert(result.str == "abc");
  assert(!r11("adc", result));

  // negation
  regexp r12("a[^ij]c");
  assert(r12("abc", result));
  assert(result.str == "abc");
  assert(!r12("aic", result));

  // literal '-' and ']' in character classes
  regexp r13("a[]-0-9-a-z-]c");
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
  regexp r14("a[a-z-[ij]]c");
  assert(r14("abc", result));
  assert(result.str == "abc");
  assert(!r14("aic", result));

  // alternations
  regexp r15("abc|def");
  assert(r15("abc", result));
  assert(result.str == "abc");
  assert(r15("def", result));
  assert(result.str == "def");
  assert(!r15("ghi", result));

  // zero or one quantifier
  regexp r16("ab?c");
  assert(r16("ac", result));
  assert(result.str == "ac");
  assert(r16("abc", result));
  assert(result.str == "abc");
  assert(!r16("abbc", result));

  // zero or more quantifier
  regexp r17("ab*c");
  assert(r17("ac", result));
  assert(result.str == "ac");
  assert(r17("abc", result));
  assert(result.str == "abc");
  assert(r17("abbc", result));
  assert(result.str == "abbc");
  assert(!r17("adc", result));

  // one or more quantifier
  regexp r18("ab+c");
  assert(!r18("ac", result));
  assert(r18("abc", result));
  assert(result.str == "abc");
  assert(r18("abbc", result));
  assert(result.str == "abbc");

  // fixed quantifier
  regexp r19("ab{2}c");
  assert(!r19("abc", result));
  assert(r19("abbc", result));
  assert(result.str == "abbc");
  assert(!r19("abbbc", result));

  // n or more quantifier
  regexp r20("ab{2,}c");
  assert(!r20("abc", result));
  assert(r20("abbc", result));
  assert(result.str == "abbc");
  assert(r20("abbbc", result));
  assert(result.str == "abbbc");

  // up to m quantifier
  regexp r21("ab{,2}c");
  assert(r21("abc", result));
  assert(result.str == "abc");
  assert(r21("abbc", result));
  assert(result.str == "abbc");
  assert(!r21("abbbc", result));

  // range quatifier
  regexp r22("ab{1,2}c");
  assert(!r22("ac", result));
  assert(r22("abc", result));
  assert(result.str == "abc");
  assert(r22("abbc", result));
  assert(result.str == "abbc");
  assert(!r22("abbbc", result));

  // capture groups
  regexp r23("a(.)c");
  assert(r23("abc", result));
  assert(result.str == "abc");
  assert(result.sub.size() == 1);
  assert(result.sub[0].back() == "b");

  // non-capture groups
  regexp r24("a(?:.)c");
  assert(r24("abc", result));
  assert(result.str == "abc");
  assert(result.sub.size() == 0);

  // varbatim characters
  regexp r25("\\Q?*+\\E");
  assert(r25("?*+", result));

  // partial matches
  regexp r26("abcd|abce");
  assert(!r26("abc", result));
  assert(r26("abc", result, { regexp::match_flag::partial }));

  // multiple captures
  regexp r27("a(..)+z");
  assert(r27("abcdefgz", result));
  assert(result.sub[0].size() == 3);
  assert(result.sub[0][0] == "bc");
  assert(result.sub[0][1] == "de");
  assert(result.sub[0][2] == "fg");

  // character class intersection
  regexp r28a("a[a-z&&[^ij]]c");
  assert(r28a("abc", result));
  assert(result.str == "abc");
  assert(!r28a("aic", result));
  regexp r28b("a[a-z&&a-hk-z]c");
  assert(r28b("abc", result));
  assert(result.str == "abc");
  assert(!r28b("aic", result));


  // more than one capture group
  regexp r96("a(bc)d(ef)g");
  assert(r96("abcdefg", result));
  assert(result.sub.size() == 2);
  assert(result.sub[0].back() == "bc");
  assert(result.sub[1].back() == "ef");
  
  // escapes inside character classes
  regexp r97("a[b\t]c");
  assert(r97("a\tc", result));
  
  // backtracking test
  regexp r98("a(?:bc|b)c");
  assert(r98("abc", result));

  // quantified groups test
  regexp r99("a(?:(?:bc){,2}de)+f");
  assert(r99("abcbcdebcdef", result));

  return 0;
}
