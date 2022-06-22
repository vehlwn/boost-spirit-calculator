## Build

```bash
$ mkdir build
$ cd build
$ cmake -GNinja ..
$ cmake --build .
$ ./a
/////////////////////////////////////////////////////////

Expression parser...

/////////////////////////////////////////////////////////

Type an expression...or [q or Q] to quit

555 / (3 * 64 + 4 + 9) + 10 / 2
-------------------------
Parsing succeeded
555 3 64 * 4 + 9 + / 10 2 / +
Result: 7.70732
-------------------------
```
