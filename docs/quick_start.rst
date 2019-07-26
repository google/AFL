=====================
AFL Quick Start Guide
=====================

You should read :ref:`instrumenting` and :ref:`fuzzing-with-afl`. They're pretty
short. If you really can't, here's how to hit the ground running:

1) Compile AFL with `make`. If build fails, see :ref:`install` for tips.

2) Find or write a reasonably fast and simple program that takes data from
   a file or stdin, processes it in a test-worthy way, then exits cleanly.
   If testing a network service, modify it to run in the foreground and read
   from stdin. When fuzzing a format that uses checksums, comment out the
   checksum verification code, too.

   The program *must* crash properly when a fault is encountered. Watch out
   for custom SIGSEGV or SIGABRT handlers and background processes. For tips on
   detecting non-crashing flaws, see :ref:`beyond-crashes`.

3) Compile the program / library to be fuzzed using `afl-gcc`. A common way to
   do this would be:

   .. code-block:: console

     CC=/path/to/afl-gcc CXX=/path/to/afl-g++ ./configure --disable-shared
     make clean all

   If program build fails, ping <afl-users@googlegroups.com>.

4) Get a small but valid input file that makes sense to the program. When
   fuzzing verbose syntax (SQL, HTTP, etc), create a dictionary as described in
   dictionaries/README.dictionaries, too.

5) If the program reads from stdin, run `afl-fuzz` like so:

   .. code-block:: console

     ./afl-fuzz -i testcase_dir -o findings_dir -- \
        /path/to/tested/program [...program's cmdline...]

   If the program takes input from a file, you can put `@@` in the program's
   command line; AFL will put an auto-generated file name in there for you.

6) Investigate anything shown in red in the fuzzer UI by promptly consulting
   ":ref:`status-screen`".

That's it. Sit back, relax, and - time permitting - try to skim through the
following:

  - :ref:`motivation`         - A general introduction to AFL,
  - :ref:`performance-tips`   - Simple tips on how to fuzz more quickly,
  - :ref:`status-screen`      - An explanation of the tidbits shown in the UI,
  - :ref:`parallel-fuzzing`   - Advice on running AFL on multiple cores.
