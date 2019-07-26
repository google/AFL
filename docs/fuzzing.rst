.. _fuzzing-with-afl:

=====================
Fuzzing with afl-fuzz
=====================

To operate correctly, the fuzzer requires one or more starting file that
contains a good example of the input data normally expected by the targeted
application. There are two basic rules:

  - Keep the files small. Under 1 kB is ideal, although not strictly necessary.
    For a discussion of why size matters, see :ref:`performance-tips`.

  - Use multiple test cases only if they are functionally different from
    each other. There is no point in using fifty different vacation photos
    to fuzz an image library.

You can find many good examples of starting files in the testcases/ subdirectory
that comes with this tool.

PS. If a large corpus of data is available for screening, you may want to use
the afl-cmin utility to identify a subset of functionally distinct files that
exercise different code paths in the target binary.


Fuzzing binaries
================

The fuzzing process itself is carried out by the `afl-fuzz` utility. This
program requires a read-only directory with initial test cases, a separate place
to store its findings, plus a path to the binary to test.

For target binaries that accept input directly from stdin, the usual syntax is:

.. code-block:: console

  $ ./afl-fuzz -i testcase_dir -o findings_dir /path/to/program [...params...]


For programs that take input from a file, use '@@' to mark the location in
the target's command line where the input file name should be placed. The
fuzzer will substitute this for you:

.. code-block:: console

  $ ./afl-fuzz -i testcase_dir -o findings_dir /path/to/program @@


You can also use the `-f` option to have the mutated data written to a specific
file. This is useful if the program expects a particular file extension or so.

Non-instrumented binaries can be fuzzed in the QEMU mode (add `-Q` in the
command line) or in a traditional, blind-fuzzer mode (specify `-n`).

You can use `-t` and `-m` to override the default timeout and memory limit for
the executed process; rare examples of targets that may need these settings
touched include compilers and video decoders.

Tips for optimizing fuzzing performance are discussed in :ref:`performance-tips`.

Note that afl-fuzz starts by performing an array of deterministic fuzzing
steps, which can take several days, but tend to produce neat test cases. If you
want quick & dirty results right away - akin to zzuf and other traditional
fuzzers -- add the `-d` option to the command line.

.. _interpreting-output:

Interpreting output
===================

See :ref:`status-screen` for information on how to interpret the displayed stats
and monitor the health of the process. Be sure to consult this section
especially if any UI elements are highlighted in red.

The fuzzing process will continue until you press Ctrl-C. At minimum, you want
to allow the fuzzer to complete one queue cycle, which may take anywhere from a
couple of hours to a week or so.

There are three subdirectories created within the output directory and updated
in real time:

  - queue/   - test cases for every distinctive execution path, plus all the
               starting files given by the user. This is the synthesized corpus
               mentioned in section 2.
               Before using this corpus for any other purposes, you can shrink
               it to a smaller size using the afl-cmin tool. The tool will find
               a smaller subset of files offering equivalent edge coverage.

  - crashes/ - unique test cases that cause the tested program to receive a
               fatal signal (e.g., SIGSEGV, SIGILL, SIGABRT). The entries are
               grouped by the received signal.

  - hangs/   - unique test cases that cause the tested program to time out. The
               default time limit before something is classified as a hang is
               the larger of 1 second and the value of the -t parameter.
               The value can be fine-tuned by setting AFL_HANG_TMOUT, but this
               is rarely necessary.

Crashes and hangs are considered "unique" if the associated execution paths
involve any state transitions not seen in previously-recorded faults. If a
single bug can be reached in multiple ways, there will be some count inflation
early in the process, but this should quickly taper off.

The file names for crashes and hangs are correlated with parent, non-faulting
queue entries. This should help with debugging.

When you can't reproduce a crash found by afl-fuzz, the most likely cause is
that you are not setting the same memory limit as used by the tool. Try:

.. code-block:: console

  $ LIMIT_MB=50
  $ ( ulimit -Sv $[LIMIT_MB << 10]; /path/to/tested_binary ... )


Change `LIMIT_MB` to match the `-m` parameter passed to afl-fuzz. On OpenBSD,
also change `-Sv` to `-Sd`.

Any existing output directory can be also used to resume aborted jobs; try:

.. code-block:: console

  $ ./afl-fuzz -i- -o existing_output_dir [...etc...]


If you have gnuplot installed, you can also generate some pretty graphs for any
active fuzzing task using afl-plot. For an example of how this looks like,
see [http://lcamtuf.coredump.cx/afl/plot/](http://lcamtuf.coredump.cx/afl/plot/).


Parallelized fuzzing
====================

Every instance of afl-fuzz takes up roughly one core. This means that on
multi-core systems, parallelization is necessary to fully utilize the hardware.
For tips on how to fuzz a common target on multiple cores or multiple networked
machines, please refer to :ref:`parallel-fuzzing`.

The parallel fuzzing mode also offers a simple way for interfacing AFL to other
fuzzers, to symbolic or concolic execution engines, and so forth; again, see the
last section of :ref:`parallel-fuzzing` for tips.


Fuzzer dictionaries
===================

By default, afl-fuzz mutation engine is optimized for compact data formats -
say, images, multimedia, compressed data, regular expression syntax, or shell
scripts. It is somewhat less suited for languages with particularly verbose and
redundant verbiage - notably including HTML, SQL, or JavaScript.

To avoid the hassle of building syntax-aware tools, afl-fuzz provides a way to
seed the fuzzing process with an optional dictionary of language keywords,
magic headers, or other special tokens associated with the targeted data type
-- and use that to reconstruct the underlying grammar on the go:

  [http://lcamtuf.blogspot.com/2015/01/afl-fuzz-making-up-grammar-with.html](http://lcamtuf.blogspot.com/2015/01/afl-fuzz-making-up-grammar-with.html)

To use this feature, you first need to create a dictionary in one of the two
formats discussed in dictionaries/README.dictionaries; and then point the fuzzer
to it via the -x option in the command line.

(Several common dictionaries are already provided in that subdirectory, too.)

There is no way to provide more structured descriptions of the underlying
syntax, but the fuzzer will likely figure out some of this based on the
instrumentation feedback alone. This actually works in practice, say:

  [http://lcamtuf.blogspot.com/2015/04/finding-bugs-in-sqlite-easy-way.html](http://lcamtuf.blogspot.com/2015/04/finding-bugs-in-sqlite-easy-way.html)

PS. Even when no explicit dictionary is given, afl-fuzz will try to extract
existing syntax tokens in the input corpus by watching the instrumentation
very closely during deterministic byte flips. This works for some types of
parsers and grammars, but isn't nearly as good as the -x mode.

If a dictionary is really hard to come by, another option is to let AFL run
for a while, and then use the token capture library that comes as a companion
utility with AFL. For that, see libtokencap/README.tokencap.

.. _crash-triage:

Crash triage
============

The coverage-based grouping of crashes usually produces a small data set that
can be quickly triaged manually or with a very simple GDB or Valgrind script.
Every crash is also traceable to its parent non-crashing test case in the
queue, making it easier to diagnose faults.

Having said that, it's important to acknowledge that some fuzzing crashes can be
difficult to quickly evaluate for exploitability without a lot of debugging and
code analysis work. To assist with this task, afl-fuzz supports a very unique
"crash exploration" mode enabled with the -C flag.

In this mode, the fuzzer takes one or more crashing test cases as the input,
and uses its feedback-driven fuzzing strategies to very quickly enumerate all
code paths that can be reached in the program while keeping it in the
crashing state.

Mutations that do not result in a crash are rejected; so are any changes that
do not affect the execution path.

The output is a small corpus of files that can be very rapidly examined to see
what degree of control the attacker has over the faulting address, or whether
it is possible to get past an initial out-of-bounds read - and see what lies
beneath.

Oh, one more thing: for test case minimization, give afl-tmin a try. The tool
can be operated in a very simple way:

.. code-block:: console

  $ ./afl-tmin -i test_case -o minimized_result -- /path/to/program [...]


The tool works with crashing and non-crashing test cases alike. In the crash
mode, it will happily accept instrumented and non-instrumented binaries. In the
non-crashing mode, the minimizer relies on standard AFL instrumentation to make
the file simpler without altering the execution path.

The minimizer accepts the `-m`, `-t`, `-f` and `@@` syntax in a manner
compatible with afl-fuzz.

Another recent addition to AFL is the afl-analyze tool. It takes an input
file, attempts to sequentially flip bytes, and observes the behavior of the
tested program. It then color-codes the input based on which sections appear to
be critical, and which are not; while not bulletproof, it can often offer quick
insights into complex file formats. More info about its operation can be found
near the end of :ref:`technical-details`.

.. _beyond-crashes:

Going beyond crashes
====================

Fuzzing is a wonderful and underutilized technique for discovering non-crashing
design and implementation errors, too. Quite a few interesting bugs have been
found by modifying the target programs to call abort() when, say:

  - Two bignum libraries produce different outputs when given the same
    fuzzer-generated input,

  - An image library produces different outputs when asked to decode the same
    input image several times in a row,

  - A serialization / deserialization library fails to produce stable outputs
    when iteratively serializing and deserializing fuzzer-supplied data,

  - A compression library produces an output inconsistent with the input file
    when asked to compress and then decompress a particular blob.

Implementing these or similar sanity checks usually takes very little time;
if you are the maintainer of a particular package, you can make this code
conditional with `#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION` (a flag also
shared with libfuzzer) or `#ifdef __AFL_COMPILER` (this one is just for AFL).
