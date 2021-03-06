# Q Regular Expressions Library (`libqre`)

libqre is a regular expressions library written in modern C++11 with an unique feature set including partial matches and multiple captures.

## Features

As of now, libqre has the following features:

### Match types

- serach match
- left and right anchors
- partial matches (string is shorter than regex)
- longest match

### Characters:

- Literal characters
- Any character: `.`
- Escaped metacharacters: `\(`, `\)`, `\[`, `\]`, `\{`, `\}`, `\?`, `\*`, `\+`, `\.`, `\^`, `\$`, `\B`, `\|`, `\\`
- Control characters: `\0`, `\a`, `\b`, `\e`, `\f`, `\n`, `\r`, `\t`, `\v`, `\cA` - `\cZ`
- Newline: `\R` (CR/CRLF/LF)
- No newline: `\N`
- Octal numbers: `\o{...}`
- Hex numbers: `\x00`-`\xFF`
- Unicode code points: `\u{...}`
- Verbatim characters `\Q...\E`

### Anchors

- Beginning of string: `^`, `\A`, ``\` ``
- End of string: `$`, `\Z`, `\'`
- Beginning of line: `^` (multiline mode)
- End of line: `$` (multiline mode)

### Character classes

- Single characters: e.g. `[abc]`
- Character ranges: e.g. `[0-9]`
- Negation: e.g. `[^abc]`
- Literal `]` at the beginning: e.g. `[]abc]`, `[^]abc]`
- Literal `-` outside ranges: e.g `[-0-9-A-F-]`
- Subtractions: e.g. `[a-z-[ij]]`
- Intersections: e.g. `[a-z&&[^ij]]` or `[a-z&&^ij]`

### Alternation

e.g. `abc|def`

### Quantifiers

- Optional: `a?`
- Zero or more: `a*`
- One or more: `a+`
- Fixed: `a{n}`
- n or more: `a{n,}`
- Up to m: `a{,m}`
- Range: `a{n,m}`
- Greedy quanitfiers (default)
- Lazy quanitfiers: `a??`, `a*?`, `a+?`, `a{...}?`

### Groups

- Capturing: e.g. `(abc)`
- Non-capturing: e.g. `(?:abc)`
- Multiple captures: e.g. `(a.c)+`
- Backrefernces: `\k'n'`, `\k<n>`, `\k{n}`, `\g'n'`, `\g<n>`, `\g{n}` referring to the nth capture group (n != 0)
- Short backreferences: `\1` to `\9` and `\-1` to `\-9`
- Backreferenced for multiple captures: `\k'n,m'`, `\k<n,m>`, `\k{n,m}`, `\g'n,m'`, `\g<n,m>`, `\g{n,m}` accessing the mth capture (m > 0)
- Forwardreferences
- Named capture groups: `(?'name'.*)`, `(?<name>.*)`
- Backreferences for named capture groups: Replace `n` with the capture group name

See example.cpp for more examples.

## Not supported

There is still a lot stuff that is not supported.
Some stuff will never be supported:

 - Literal `\` in a character class. Backslash always escapes.
 - Possesive quantifiers. Use atomic groups instead.
 - Unicode aside from simple code points.
 - Predefined character classes.
 - Match attempt anchors.
 - Recursion and subroutines.
