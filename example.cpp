#include <cassert>
#include <iostream>
#include <regexp.hpp>

int main()
{
  regexp::match result;

  // Literal characters
  regexp r00("abc");
  assert(r00("abc", result));
  assert(result.sub[0] == "abc");
  assert(!r00("def", result));

  // The dot
  regexp r01("a.c");
  assert(r01("abc", result));
  assert(result.sub[0] == "abc");

  // escaped characters
  regexp r02("\\(\\)\\[\\]\\{\\}\\?\\*\\+\\.\\^\\$\\B\\\\");
  assert(r02("()[]{}?*+.^$\\\\", result));
  assert(result.sub[0] == "()[]{}?*+.^$\\\\");

  // control characters
  regexp r03("\\a\\b\\e\\f\\n\\r\\t\\v\\cZ");
  assert(r03("\a\b\x1b\f\n\r\t\v\x1a", result));
  assert(result.sub[0] == "\a\b\x1b\f\n\r\t\v\x1a");

  // newline
  regexp r04("a\\Rb");
  assert(r04("a\nb", result));
  assert(result.sub[0] == "a\nb");
  assert(r04("a\r\nb", result));
  assert(result.sub[0] == "a\r\nb");
  assert(r04("a\rb", result));
  assert(result.sub[0] == "a\rb");
  assert(!r04("ab", result));

  // no newline
  regexp r05("a\\Nb");
  assert(r05("ab", result));
  assert(result.sub[0] == "ab");
  assert(!r05("a\nb", result));

  // octal and hex numbers
  regexp r06("\\o{141}\\x62\\x{63}");
  assert(r06("abc", result));
  assert(result.sub[0] == "abc");
  assert(!r00("def", result));

  // anchors
  regexp r07("^abc$");
  assert(r07("abc", result));
  assert(result.sub[0] == "abc");
  assert(!r07("0abc1", result));
  regexp r08("\\Aabc\\Z");
  assert(r08("abc", result));
  assert(result.sub[0] == "abc");
  assert(!r08("0abc1", result));
  regexp r09("\\`abc\\'");
  assert(r09("abc", result));
  assert(result.sub[0] == "abc");
  assert(!r09("0abc1", result));

  // charachter classes with single characters
  regexp r10("a[abc]c");
  assert(r10("abc", result));
  assert(result.sub[0] == "abc");
  assert(!r10("adc", result));

  // charachter classes with ranges
  regexp r11("a[a-c]c");
  assert(r11("abc", result));
  assert(result.sub[0] == "abc");
  assert(!r11("adc", result));

  // negation
  regexp r12("a[^ij]c");
  assert(r12("abc", result));
  assert(result.sub[0] == "abc");
  assert(!r12("aic", result));

  // literal '-' and ']' in character classes
  regexp r13("a[]-0-9-a-z-]c");
  assert(r13("abc", result));
  assert(result.sub[0] == "abc");
  assert(r13("a]c", result));
  assert(result.sub[0] == "a]c");
  assert(r13("a-c", result));
  assert(result.sub[0] == "a-c");
  assert(!r13("a_c", result));

  // character class subtraction
  regexp r14("a[a-z-[ij]]c");
  assert(r14("abc", result));
  assert(result.sub[0] == "abc");
  assert(!r14("aic", result));

  // alternations
  regexp r15("abc|def");
  assert(r15("abc", result));
  assert(result.sub[0] == "abc");
  assert(r15("def", result));
  assert(result.sub[0] == "def");
  assert(!r15("ghi", result));

  // zero or one quantifier
  regexp r16("ab?c");
  assert(r16("ac", result));
  assert(result.sub[0] == "ac");
  assert(r16("abc", result));
  assert(result.sub[0] == "abc");
  assert(!r16("abbc", result));

  // zero or more quantifier
  regexp r17("ab*c");
  assert(r17("ac", result));
  assert(result.sub[0] == "ac");
  assert(r17("abc", result));
  assert(result.sub[0] == "abc");
  assert(r17("abbc", result));
  assert(result.sub[0] == "abbc");
  assert(!r17("adc", result));

  // one or more quantifier
  regexp r18("ab+c");
  assert(!r18("ac", result));
  assert(r18("abc", result));
  assert(result.sub[0] == "abc");
  assert(r18("abbc", result));
  assert(result.sub[0] == "abbc");

  // fixed quantifier
  regexp r19("ab{2}c");
  assert(!r19("abc", result));
  assert(r19("abbc", result));
  assert(result.sub[0] == "abbc");
  assert(!r19("abbbc", result));

  // n or more quantifier
  regexp r20("ab{2,}c");
  assert(!r20("abc", result));
  assert(r20("abbc", result));
  assert(result.sub[0] == "abbc");
  assert(r20("abbbc", result));
  assert(result.sub[0] == "abbbc");

  // up to m quantifier
  regexp r21("ab{,2}c");
  assert(r21("abc", result));
  assert(result.sub[0] == "abc");
  assert(r21("abbc", result));
  assert(result.sub[0] == "abbc");
  assert(!r21("abbbc", result));

  // range quatifier
  regexp r22("ab{1,2}c");
  assert(!r22("ac", result));
  assert(r22("abc", result));
  assert(result.sub[0] == "abc");
  assert(r22("abbc", result));
  assert(result.sub[0] == "abbc");
  assert(!r22("abbbc", result));

  // capture groups
  regexp r23("a(.)c");
  assert(r23("abc", result));
  assert(result.sub[0] == "abc");
  assert(result.sub.size() == 2);
  assert(result.sub[1] == "b");

  // non-capture groups
  regexp r24("a(?:.)c");
  assert(r24("abc", result));
  assert(result.sub[0] == "abc");
  assert(result.sub.size() == 1);

  // varbatim characters
  regexp r25("\\Q?*+\\E");
  assert(r25("?*+", result));


  // escapes inside character classes
  regexp r97("a[b\t]c");
  assert(r97("a\tc", result));
  
  // backtracking test
  regexp r98("a(?:bc|b)c");
  assert(r98("abc", result));

  // quantified groups test
  regexp r99("a((bc){,2}de)+f");
  assert(r99("abcbcdebcdef", result));

  return 0;
}
