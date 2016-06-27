#include <cassert>
#include <iostream>
#include <regexp.hpp>

int main()
{
  // Literal characters
  regexp r00("abc");
  assert(r00.match("abc"));
  assert(r00.result.sub[0] == "abc");
  assert(!r00.match("def"));

  // The dot
  regexp r01("a.c");
  assert(r01.match("abc"));
  assert(r01.result.sub[0] == "abc");

  // escaped characters
  regexp r02("\\(\\)\\[\\]\\{\\}\\?\\*\\+\\.\\^\\$\\B\\\\");
  assert(r02.match("()[]{}?*+.^$\\\\"));
  assert(r02.result.sub[0] == "()[]{}?*+.^$\\\\");

  // control characters
  regexp r03("\\a\\b\\e\\f\\n\\r\\t\\v\\cZ");
  assert(r03.match("\a\b\x1b\f\n\r\t\v\x1a"));
  assert(r03.result.sub[0] == "\a\b\x1b\f\n\r\t\v\x1a");

  // newline
  regexp r04("a\\Rb");
  assert(r04.match("a\nb"));
  assert(r04.result.sub[0] == "a\nb");
  assert(r04.match("a\r\nb"));
  assert(r04.result.sub[0] == "a\r\nb");
  assert(r04.match("a\rb"));
  assert(r04.result.sub[0] == "a\rb");
  assert(!r04.match("ab"));

  // no newline
  regexp r05("a\\Nb");
  assert(r05.match("ab"));
  assert(r05.result.sub[0] == "ab");
  assert(!r05.match("a\nb"));

  // octal and hex numbers
  regexp r06("\\o{141}\\x62\\x{63}");
  assert(r06.match("abc"));
  assert(r06.result.sub[0] == "abc");
  assert(!r00.match("def"));

  // anchors
  regexp r07("^abc$");
  assert(r07.match("abc"));
  assert(r07.result.sub[0] == "abc");
  assert(!r07.match("0abc1"));
  regexp r08("\\Aabc\\Z");
  assert(r08.match("abc"));
  assert(r08.result.sub[0] == "abc");
  assert(!r08.match("0abc1"));
  regexp r09("\\`abc\\'");
  assert(r09.match("abc"));
  assert(r09.result.sub[0] == "abc");
  assert(!r09.match("0abc1"));

  // charachter classes with single characters
  regexp r10("a[abc]c");
  assert(r10.match("abc"));
  assert(r10.result.sub[0] == "abc");
  assert(!r10.match("adc"));

  // charachter classes with ranges
  regexp r11("a[a-c]c");
  assert(r11.match("abc"));
  assert(r11.result.sub[0] == "abc");
  assert(!r11.match("adc"));

  // negation
  regexp r12("a[^ij]c");
  assert(r12.match("abc"));
  assert(r12.result.sub[0] == "abc");
  assert(!r12.match("aic"));

  // literal '-' and ']' in character classes
  regexp r13("a[]-0-9-a-z-]c");
  assert(r13.match("abc"));
  assert(r13.result.sub[0] == "abc");
  assert(r13.match("a]c"));
  assert(r13.result.sub[0] == "a]c");
  assert(r13.match("a-c"));
  assert(r13.result.sub[0] == "a-c");
  assert(!r13.match("a_c"));

  // character class subtraction
  regexp r14("a[a-z-[ij]]c");
  assert(r14.match("abc"));
  assert(r14.result.sub[0] == "abc");
  assert(!r14.match("aic"));

  // alternations
  regexp r15("abc|def");
  assert(r15.match("abc"));
  assert(r15.result.sub[0] == "abc");
  assert(r15.match("def"));
  assert(r15.result.sub[0] == "def");
  assert(!r15.match("ghi"));

  // zero or one quantifier
  regexp r16("ab?c");
  assert(r16.match("ac"));
  assert(r16.result.sub[0] == "ac");
  assert(r16.match("abc"));
  assert(r16.result.sub[0] == "abc");
  assert(!r16.match("abbc"));

  // zero or more quantifier
  regexp r17("ab*c");
  assert(r17.match("ac"));
  assert(r17.result.sub[0] == "ac");
  assert(r17.match("abc"));
  assert(r17.result.sub[0] == "abc");
  assert(r17.match("abbc"));
  assert(r17.result.sub[0] == "abbc");
  assert(!r17.match("adc"));

  // one or more quantifier
  regexp r18("ab+c");
  assert(!r18.match("ac"));
  assert(r18.match("abc"));
  assert(r18.result.sub[0] == "abc");
  assert(r18.match("abbc"));
  assert(r18.result.sub[0] == "abbc");

  // fixed quantifier
  regexp r19("ab{2}c");
  assert(!r19.match("abc"));
  assert(r19.match("abbc"));
  assert(r19.result.sub[0] == "abbc");
  assert(!r19.match("abbbc"));

  // n or more quantifier
  regexp r20("ab{2,}c");
  assert(!r20.match("abc"));
  assert(r20.match("abbc"));
  assert(r20.result.sub[0] == "abbc");
  assert(r20.match("abbbc"));
  assert(r20.result.sub[0] == "abbbc");

  // up to m quantifier
  regexp r21("ab{,2}c");
  assert(r21.match("abc"));
  assert(r21.result.sub[0] == "abc");
  assert(r21.match("abbc"));
  assert(r21.result.sub[0] == "abbc");
  assert(!r21.match("abbbc"));

  // range quatifier
  regexp r22("ab{1,2}c");
  assert(!r22.match("ac"));
  assert(r22.match("abc"));
  assert(r22.result.sub[0] == "abc");
  assert(r22.match("abbc"));
  assert(r22.result.sub[0] == "abbc");
  assert(!r22.match("abbbc"));

  // capture groups
  regexp r23("a(.)c");
  assert(r23.match("abc"));
  assert(r23.result.sub[0] == "abc");
  assert(r23.result.sub.size() == 2);
  assert(r23.result.sub[1] == "b");

  // non-capture groups
  regexp r24("a(?:.)c");
  assert(r24.match("abc"));
  assert(r24.result.sub[0] == "abc");
  assert(r24.result.sub.size() == 1);


  // backtracking test
  regexp r98("a(?:bc|b)c");
  assert(r98.match("abc"));

  // quantified groups test
  regexp r99("a((bc){,2}de)+f");
  assert(r99.match("abcbcdebcdef"));

  return 0;
}
