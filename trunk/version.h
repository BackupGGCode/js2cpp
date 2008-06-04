// version.h - version info for js2cpp

#define APP_MAJOR 1
#define APP_MINOR 5
#define APP_BUILD 1

/*
1.05.01 2008.03.03 spike
'Indexed call' works (a[0]=f, a[0](x) calls f(x) with this===a.)
Checked with the gurus on comp.lang.javascript to make sure I read ECMA262 right.
Implemented === and !==.

1.04.04 2008.03.01 spike
Nested functions work! (Of course, I haven't tried a garbage collect...)
Crude array literals are also working - Efficient? Hardly!

1.04.03 2008.02.29 spike
Implementing nested functions.
Not there yet!  Now keeping good track of nested scopes though.
Some progress on array literals.

1.04.02 2008.02.20 spike
Pulled 'scope' class into a separate module.
Changed functions to compile into classes, derived from func_

1.04.01 2008.02.18 spike
Able to set and get named properties of an object using '.' and []
Method calls work, for common case of o.m(...)
Crude Object constructor working.
Added reserved-for-future keywords to scanner.
Got minimal Date constructor working, and getTime method, so
can run the Ron's Benchmarks for Tak, Sieve and Fib.

1.03.11 2008.02.18 spike
(Finally) - compiles and runs autotest.js correctly.
External variables (undefined) and functions (alert).
Basic parameter passing.

1.03.10
interim check-in.

1.03.09
interim check-in.

1.03.8 2008.02.12 spike
Recovering from disk failure.

1.03.7	2008.02.06 spike -------
More benchmarks from the web.
Added correct operator-precedence.

1.03.6	2008.02.05 spike -------
Simple factorial runs. Working on compiling a
benchmark from the web: Fannkuch.js

1.03.5	2008.02.05 spike -------
Handle transcription of single-quoted strings to C++ syntax.
But still a fake - js needs UTF-16 strings.

1.03.4	2008.02.04 spike -------
Compiles hello.js to hello.cpp. First program to compile & run!

1.03.3	2008.02.02 spike -------
...Nothing happened here. Move along.

1.03.2	2008.02.01 spike -------
Validate predefined token names in Debug build.
Builds tree for utils.js
AST printer working 'OK'

1.03.1	2008.02.01 spike -------
First Parsing - parses utils.js!

1.02.0	spike -------
Error reporting framework.

1.01.1	spike -------
Oops: Added stringtab.* and jsrt.h to source code control.

1.01.0	spike -------
intern token names, keywords get their own lexical codes.
We now have enough information to start parsing!

1.00.1	spike -------
most token types scanned, including regex literals.

1.00.0	spike -------
very first basic version - tokenizer.

*/