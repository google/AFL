==============
AFL User Guide
==============

.. _status-screen:

Understanding the status screen
===============================

  This document provides an overview of the status screen - plus tips for
  troubleshooting any warnings and red text shown in the UI.

0) A note about colors
----------------------

The status screen and error messages use colors to keep things readable and
attract your attention to the most important details. For example, red almost
always means "consult this doc" :-)

Unfortunately, the UI will render correctly only if your terminal is using
traditional un*x palette (white text on black background) or something close
to that.

If you are using inverse video, you may want to change your settings, say:

  - For GNOME Terminal, go to Edit > Profile preferences, select the "colors"
    tab, and from the list of built-in schemes, choose "white on black".

  - For the MacOS X Terminal app, open a new window using the "Pro" scheme via
    the Shell > New Window menu (or make "Pro" your default).

Alternatively, if you really like your current colors, you can edit config.h
to comment out USE_COLORS, then do 'make clean all'.

I'm not aware of any other simple way to make this work without causing
other side effects - sorry about that.

With that out of the way, let's talk about what's actually on the screen...

1) Process timing
-----------------

::

  +----------------------------------------------------+
  |        run time : 0 days, 8 hrs, 32 min, 43 sec    |
  |   last new path : 0 days, 0 hrs, 6 min, 40 sec     |
  | last uniq crash : none seen yet                    |
  |  last uniq hang : 0 days, 1 hrs, 24 min, 32 sec    |
  +----------------------------------------------------+

This section is fairly self-explanatory: it tells you how long the fuzzer has
been running and how much time has elapsed since its most recent finds. This is
broken down into "paths" (a shorthand for test cases that trigger new execution
patterns), crashes, and hangs.

When it comes to timing: there is no hard rule, but most fuzzing jobs should be
expected to run for days or weeks; in fact, for a moderately complex project, the
first pass will probably take a day or so. Every now and then, some jobs
will be allowed to run for months.

There's one important thing to watch out for: if the tool is not finding new
paths within several minutes of starting, you're probably not invoking the
target binary correctly and it never gets to parse the input files we're
throwing at it; another possible explanations are that the default memory limit
(-m) is too restrictive, and the program exits after failing to allocate a
buffer very early on; or that the input files are patently invalid and always
fail a basic header check.

If there are no new paths showing up for a while, you will eventually see a big
red warning in this section, too :-)

2) Overall results
------------------

::

  +-----------------------+
  |  cycles done : 0      |
  |  total paths : 2095   |
  | uniq crashes : 0      |
  |   uniq hangs : 19     |
  +-----------------------+

The first field in this section gives you the count of queue passes done so far
- that is, the number of times the fuzzer went over all the interesting test
cases discovered so far, fuzzed them, and looped back to the very beginning.
Every fuzzing session should be allowed to complete at least one cycle; and
ideally, should run much longer than that.

As noted earlier, the first pass can take a day or longer, so sit back and
relax. If you want to get broader but more shallow coverage right away, try
the -d option - it gives you a more familiar experience by skipping the
deterministic fuzzing steps. It is, however, inferior to the standard mode in
a couple of subtle ways.

To help make the call on when to hit Ctrl-C, the cycle counter is color-coded.
It is shown in magenta during the first pass, progresses to yellow if new finds
are still being made in subsequent rounds, then blue when that ends - and
finally, turns green after the fuzzer hasn't been seeing any action for a
longer while.

The remaining fields in this part of the screen should be pretty obvious:
there's the number of test cases ("paths") discovered so far, and the number of
unique faults. The test cases, crashes, and hangs can be explored in real-time
by browsing the output directory, as discussed in :ref:`interpreting-output`.

3) Cycle progress
-----------------

::

  +-------------------------------------+
  |  now processing : 1296 (61.86%)     |
  | paths timed out : 0 (0.00%)         |
  +-------------------------------------+

This box tells you how far along the fuzzer is with the current queue cycle: it
shows the ID of the test case it is currently working on, plus the number of
inputs it decided to ditch because they were persistently timing out.

The "*" suffix sometimes shown in the first line means that the currently
processed path is not "favored" (a property discussed later on, in section 6).

If you feel that the fuzzer is progressing too slowly, see the note about the
-d option in section 2 of this doc.

4) Map coverage
---------------

::

  +--------------------------------------+
  |    map density : 10.15% / 29.07%     |
  | count coverage : 4.03 bits/tuple     |
  +--------------------------------------+

The section provides some trivia about the coverage observed by the
instrumentation embedded in the target binary.

The first line in the box tells you how many branch tuples we have already
hit, in proportion to how much the bitmap can hold. The number on the left
describes the current input; the one on the right is the value for the entire
input corpus.

Be wary of extremes:

  - Absolute numbers below 200 or so suggest one of three things: that the
    program is extremely simple; that it is not instrumented properly (e.g.,
    due to being linked against a non-instrumented copy of the target
    library); or that it is bailing out prematurely on your input test cases.
    The fuzzer will try to mark this in pink, just to make you aware.

  - Percentages over 70% may very rarely happen with very complex programs
    that make heavy use of template-generated code.

    Because high bitmap density makes it harder for the fuzzer to reliably
    discern new program states, I recommend recompiling the binary with
    AFL_INST_RATIO=10 or so and trying again (see env_variables.txt).

    The fuzzer will flag high percentages in red. Chances are, you will never
    see that unless you're fuzzing extremely hairy software (say, v8, perl,
    ffmpeg).

The other line deals with the variability in tuple hit counts seen in the
binary. In essence, if every taken branch is always taken a fixed number of
times for all the inputs we have tried, this will read "1.00". As we manage
to trigger other hit counts for every branch, the needle will start to move
toward "8.00" (every bit in the 8-bit map hit), but will probably never
reach that extreme.

Together, the values can be useful for comparing the coverage of several
different fuzzing jobs that rely on the same instrumented binary.

5) Stage progress
-----------------

::

  +-------------------------------------+
  |  now trying : interest 32/8         |
  | stage execs : 3996/34.4k (11.62%)   |
  | total execs : 27.4M                 |
  |  exec speed : 891.7/sec             |
  +-------------------------------------+

This part gives you an in-depth peek at what the fuzzer is actually doing right
now. It tells you about the current stage, which can be any of:

  - calibration - a pre-fuzzing stage where the execution path is examined
    to detect anomalies, establish baseline execution speed, and so on. Executed
    very briefly whenever a new find is being made.

  - trim L/S - another pre-fuzzing stage where the test case is trimmed to the
    shortest form that still produces the same execution path. The length (L)
    and stepover (S) are chosen in general relationship to file size.

  - bitflip L/S - deterministic bit flips. There are L bits toggled at any given
    time, walking the input file with S-bit increments. The current L/S variants
    are: 1/1, 2/1, 4/1, 8/8, 16/8, 32/8.

  - arith L/8 - deterministic arithmetics. The fuzzer tries to subtract or add
    small integers to 8-, 16-, and 32-bit values. The stepover is always 8 bits.

  - interest L/8 - deterministic value overwrite. The fuzzer has a list of known
    "interesting" 8-, 16-, and 32-bit values to try. The stepover is 8 bits.

  - extras - deterministic injection of dictionary terms. This can be shown as
    "user" or "auto", depending on whether the fuzzer is using a user-supplied
    dictionary (-x) or an auto-created one. You will also see "over" or "insert",
    depending on whether the dictionary words overwrite existing data or are
    inserted by offsetting the remaining data to accommodate their length.

  - havoc - a sort-of-fixed-length cycle with stacked random tweaks. The
    operations attempted during this stage include bit flips, overwrites with
    random and "interesting" integers, block deletion, block duplication, plus
    assorted dictionary-related operations (if a dictionary is supplied in the
    first place).

  - splice - a last-resort strategy that kicks in after the first full queue
    cycle with no new paths. It is equivalent to 'havoc', except that it first
    splices together two random inputs from the queue at some arbitrarily
    selected midpoint.

  - sync - a stage used only when -M or -S is set (see :ref:`parallel-fuzzing`).
    No real fuzzing is involved, but the tool scans the output from other
    fuzzers and imports test cases as necessary. The first time this is done,
    it may take several minutes or so.

The remaining fields should be fairly self-evident: there's the exec count
progress indicator for the current stage, a global exec counter, and a
benchmark for the current program execution speed. This may fluctuate from
one test case to another, but the benchmark should be ideally over 500 execs/sec
most of the time - and if it stays below 100, the job will probably take very
long.

The fuzzer will explicitly warn you about slow targets, too. If this happens,
see :ref:`performance-tips` for ideas on how to speed things up.

6) Findings in depth
--------------------

::

  +--------------------------------------+
  | favored paths : 879 (41.96%)         |
  |  new edges on : 423 (20.19%)         |
  | total crashes : 0 (0 unique)         |
  |  total tmouts : 24 (19 unique)       |
  +--------------------------------------+

This gives you several metrics that are of interest mostly to complete nerds.
The section includes the number of paths that the fuzzer likes the most based
on a minimization algorithm baked into the code (these will get considerably
more air time), and the number of test cases that actually resulted in better
edge coverage (versus just pushing the branch hit counters up). There are also
additional, more detailed counters for crashes and timeouts.

Note that the timeout counter is somewhat different from the hang counter; this
one includes all test cases that exceeded the timeout, even if they did not
exceed it by a margin sufficient to be classified as hangs.

7) Fuzzing strategy yields
--------------------------

::

  +-----------------------------------------------------+
  |   bit flips : 57/289k, 18/289k, 18/288k             |
  |  byte flips : 0/36.2k, 4/35.7k, 7/34.6k             |
  | arithmetics : 53/2.54M, 0/537k, 0/55.2k             |
  |  known ints : 8/322k, 12/1.32M, 10/1.70M            |
  |  dictionary : 9/52k, 1/53k, 1/24k                   |
  |       havoc : 1903/20.0M, 0/0                       |
  |        trim : 20.31%/9201, 17.05%                   |
  +-----------------------------------------------------+

This is just another nerd-targeted section keeping track of how many paths we
have netted, in proportion to the number of execs attempted, for each of the
fuzzing strategies discussed earlier on. This serves to convincingly validate
assumptions about the usefulness of the various approaches taken by afl-fuzz.

The trim strategy stats in this section are a bit different than the rest.
The first number in this line shows the ratio of bytes removed from the input
files; the second one corresponds to the number of execs needed to achieve this
goal. Finally, the third number shows the proportion of bytes that, although
not possible to remove, were deemed to have no effect and were excluded from
some of the more expensive deterministic fuzzing steps.

8) Path geometry
----------------

::

  +---------------------+
  |    levels : 5       |
  |   pending : 1570    |
  |  pend fav : 583     |
  | own finds : 0       |
  |  imported : 0       |
  | stability : 100.00% |
  +---------------------+

The first field in this section tracks the path depth reached through the
guided fuzzing process. In essence: the initial test cases supplied by the
user are considered "level 1". The test cases that can be derived from that
through traditional fuzzing are considered "level 2"; the ones derived by
using these as inputs to subsequent fuzzing rounds are "level 3"; and so forth.
The maximum depth is therefore a rough proxy for how much value you're getting
out of the instrumentation-guided approach taken by afl-fuzz.

The next field shows you the number of inputs that have not gone through any
fuzzing yet. The same stat is also given for "favored" entries that the fuzzer
really wants to get to in this queue cycle (the non-favored entries may have to
wait a couple of cycles to get their chance).

Next, we have the number of new paths found during this fuzzing section and
imported from other fuzzer instances when doing parallelized fuzzing; and the
extent to which identical inputs appear to sometimes produce variable behavior
in the tested binary.

That last bit is actually fairly interesting: it measures the consistency of
observed traces. If a program always behaves the same for the same input data,
it will earn a score of 100%. When the value is lower but still shown in purple,
the fuzzing process is unlikely to be negatively affected. If it goes into red,
you may be in trouble, since AFL will have difficulty discerning between
meaningful and "phantom" effects of tweaking the input file.

Now, most targets will just get a 100% score, but when you see lower figures,
there are several things to look at:

  - The use of uninitialized memory in conjunction with some intrinsic sources
    of entropy in the tested binary. Harmless to AFL, but could be indicative
    of a security bug.

  - Attempts to manipulate persistent resources, such as left over temporary
    files or shared memory objects. This is usually harmless, but you may want
    to double-check to make sure the program isn't bailing out prematurely.
    Running out of disk space, SHM handles, or other global resources can
    trigger this, too.

  - Hitting some functionality that is actually designed to behave randomly.
    Generally harmless. For example, when fuzzing sqlite, an input like
    'select random();' will trigger a variable execution path.

  - Multiple threads executing at once in semi-random order. This is harmless
    when the 'stability' metric stays over 90% or so, but can become an issue
    if not. Here's what to try:

    - Use afl-clang-fast from llvm_mode/ - it uses a thread-local tracking
      model that is less prone to concurrency issues,

    - See if the target can be compiled or run without threads. Common
      ./configure options include --without-threads, --disable-pthreads, or
      --disable-openmp.

    - Replace pthreads with GNU Pth (https://www.gnu.org/software/pth/), which
      allows you to use a deterministic scheduler.

  - In persistent mode, minor drops in the "stability" metric can be normal,
    because not all the code behaves identically when re-entered; but major
    dips may signify that the code within __AFL_LOOP() is not behaving
    correctly on subsequent iterations (e.g., due to incomplete clean-up or
    reinitialization of the state) and that most of the fuzzing effort goes
    to waste.

The paths where variable behavior is detected are marked with a matching entry
in the <out_dir>/queue/.state/variable_behavior/ directory, so you can look
them up easily.

9) CPU load
-----------

::

  [cpu: 25%]

This tiny widget shows the apparent CPU utilization on the local system. It is
calculated by taking the number of processes in the "runnable" state, and then
comparing it to the number of logical cores on the system.

If the value is shown in green, you are using fewer CPU cores than available on
your system and can probably parallelize to improve performance; for tips on
how to do that, see :ref:`parallel-fuzzing`.

If the value is shown in red, your CPU is *possibly* oversubscribed, and
running additional fuzzers may not give you any benefits.

Of course, this benchmark is very simplistic; it tells you how many processes
are ready to run, but not how resource-hungry they may be. It also doesn't
distinguish between physical cores, logical cores, and virtualized CPUs; the
performance characteristics of each of these will differ quite a bit.

If you want a more accurate measurement, you can run the afl-gotcpu utility
from the command line.

10) Addendum: status and plot files
-----------------------------------

For unattended operation, some of the key status screen information can be also
found in a machine-readable format in the fuzzer_stats file in the output
directory. This includes:

  - start_time     - unix time indicating the start time of afl-fuzz
  - last_update    - unix time corresponding to the last update of this file
  - fuzzer_pid     - PID of the fuzzer process
  - cycles_done    - queue cycles completed so far
  - execs_done     - number of execve() calls attempted
  - execs_per_sec  - current number of execs per second
  - paths_total    - total number of entries in the queue
  - paths_found    - number of entries discovered through local fuzzing
  - paths_imported - number of entries imported from other instances
  - max_depth      - number of levels in the generated data set
  - cur_path       - currently processed entry number
  - pending_favs   - number of favored entries still waiting to be fuzzed
  - pending_total  - number of all entries waiting to be fuzzed
  - stability      - percentage of bitmap bytes that behave consistently
  - variable_paths - number of test cases showing variable behavior
  - unique_crashes - number of unique crashes recorded
  - unique_hangs   - number of unique hangs encountered

Most of these map directly to the UI elements discussed earlier on.

On top of that, you can also find an entry called 'plot_data', containing a
plottable history for most of these fields. If you have gnuplot installed, you
can turn this into a nice progress report with the included 'afl-plot' tool.


Environmental variables
=======================

  This document discusses the environment variables used by American Fuzzy Lop
  to expose various exotic functions that may be (rarely) useful for power
  users or for some types of custom fuzzing setups. See README for the general
  instruction manual.

1) Settings for afl-gcc, afl-clang, and afl-as
----------------------------------------------

Because they can't directly accept command-line options, the compile-time
tools make fairly broad use of environmental variables:

  - Setting AFL_HARDEN automatically adds code hardening options when invoking
    the downstream compiler. This currently includes -D_FORTIFY_SOURCE=2 and
    -fstack-protector-all. The setting is useful for catching non-crashing
    memory bugs at the expense of a very slight (sub-5%) performance loss.

  - By default, the wrapper appends -O3 to optimize builds. Very rarely, this
    will cause problems in programs built with -Werror, simply because -O3
    enables more thorough code analysis and can spew out additional warnings.
    To disable optimizations, set AFL_DONT_OPTIMIZE.

  - Setting AFL_USE_ASAN automatically enables ASAN, provided that your
    compiler supports that. Note that fuzzing with ASAN is mildly challenging
    - see :ref:`asan-notes`.

    (You can also enable MSAN via AFL_USE_MSAN; ASAN and MSAN come with the
    same gotchas; the modes are mutually exclusive. UBSAN and other exotic
    sanitizers are not officially supported yet, but are easy to get to work
    by hand.)

  - Setting AFL_CC, AFL_CXX, and AFL_AS lets you use alternate downstream
    compilation tools, rather than the default 'clang', 'gcc', or 'as' binaries
    in your $PATH.

  - AFL_PATH can be used to point afl-gcc to an alternate location of afl-as.
    One possible use of this is experimental/clang_asm_normalize/, which lets
    you instrument hand-written assembly when compiling clang code by plugging
    a normalizer into the chain. (There is no equivalent feature for GCC.)

  - Setting AFL_INST_RATIO to a percentage between 0 and 100% controls the
    probability of instrumenting every branch. This is (very rarely) useful
    when dealing with exceptionally complex programs that saturate the output
    bitmap. Examples include v8, ffmpeg, and perl.

    (If this ever happens, afl-fuzz will warn you ahead of the time by
    displaying the "bitmap density" field in fiery red.)

    Setting AFL_INST_RATIO to 0 is a valid choice. This will instrument only
    the transitions between function entry points, but not individual branches.

  - AFL_NO_BUILTIN causes the compiler to generate code suitable for use with
    libtokencap.so (but perhaps running a bit slower than without the flag).

  - TMPDIR is used by afl-as for temporary files; if this variable is not set,
    the tool defaults to /tmp.

  - Setting AFL_KEEP_ASSEMBLY prevents afl-as from deleting instrumented
    assembly files. Useful for troubleshooting problems or understanding how
    the tool works. To get them in a predictable place, try something like:

    mkdir assembly_here
    TMPDIR=$PWD/assembly_here AFL_KEEP_ASSEMBLY=1 make clean all

  - Setting AFL_QUIET will prevent afl-cc and afl-as banners from being
    displayed during compilation, in case you find them distracting.

2) Settings for afl-clang-fast
------------------------------

The native LLVM instrumentation helper accepts a subset of the settings
discussed in section #1, with the exception of:

  - AFL_AS, since this toolchain does not directly invoke GNU as.

  - TMPDIR and AFL_KEEP_ASSEMBLY, since no temporary assembly files are
    created.

Note that AFL_INST_RATIO will behave a bit differently than for afl-gcc,
because functions are *not* instrumented unconditionally - so low values
will have a more striking effect. For this tool, 0 is not a valid choice.

3) Settings for afl-fuzz
------------------------

The main fuzzer binary accepts several options that disable a couple of sanity
checks or alter some of the more exotic semantics of the tool:

  - Setting AFL_SKIP_CPUFREQ skips the check for CPU scaling policy. This is
    useful if you can't change the defaults (e.g., no root access to the
    system) and are OK with some performance loss.

  - Setting AFL_NO_FORKSRV disables the forkserver optimization, reverting to
    fork + execve() call for every tested input. This is useful mostly when
    working with unruly libraries that create threads or do other crazy
    things when initializing (before the instrumentation has a chance to run).

    Note that this setting inhibits some of the user-friendly diagnostics
    normally done when starting up the forkserver and causes a pretty
    significant performance drop.

  - AFL_EXIT_WHEN_DONE causes afl-fuzz to terminate when all existing paths
    have been fuzzed and there were no new finds for a while. This would be
    normally indicated by the cycle counter in the UI turning green. May be
    convenient for some types of automated jobs.

  - Setting AFL_NO_AFFINITY disables attempts to bind to a specific CPU core
    on Linux systems. This slows things down, but lets you run more instances
    of afl-fuzz than would be prudent (if you really want to).

  - AFL_SKIP_CRASHES causes AFL to tolerate crashing files in the input
    queue. This can help with rare situations where a program crashes only
    intermittently, but it's not really recommended under normal operating
    conditions.

  - Setting AFL_HANG_TMOUT allows you to specify a different timeout for
    deciding if a particular test case is a "hang". The default is 1 second
    or the value of the -t parameter, whichever is larger. Dialing the value
    down can be useful if you are very concerned about slow inputs, or if you
    don't want AFL to spend too much time classifying that stuff and just 
    rapidly put all timeouts in that bin.

  - AFL_NO_ARITH causes AFL to skip most of the deterministic arithmetics.
    This can be useful to speed up the fuzzing of text-based file formats.

  - AFL_SHUFFLE_QUEUE randomly reorders the input queue on startup. Requested
    by some users for unorthodox parallelized fuzzing setups, but not
    advisable otherwise.

  - When developing custom instrumentation on top of afl-fuzz, you can use
    AFL_SKIP_BIN_CHECK to inhibit the checks for non-instrumented binaries
    and shell scripts; and AFL_DUMB_FORKSRV in conjunction with the -n
    setting to instruct afl-fuzz to still follow the fork server protocol
    without expecting any instrumentation data in return.

  - When running in the -M or -S mode, setting AFL_IMPORT_FIRST causes the
    fuzzer to import test cases from other instances before doing anything
    else. This makes the "own finds" counter in the UI more accurate.
    Beyond counter aesthetics, not much else should change.

  - Setting AFL_POST_LIBRARY allows you to configure a postprocessor for
    mutated files - say, to fix up checksums. See experimental/post_library/
    for more.

  - AFL_FAST_CAL keeps the calibration stage about 2.5x faster (albeit less
    precise), which can help when starting a session against a slow target.

  - The CPU widget shown at the bottom of the screen is fairly simplistic and
    may complain of high load prematurely, especially on systems with low core
    counts. To avoid the alarming red color, you can set AFL_NO_CPU_RED.

  - In QEMU mode (-Q), AFL_PATH will be searched for afl-qemu-trace.

  - Setting AFL_PRELOAD causes AFL to set LD_PRELOAD for the target binary
    without disrupting the afl-fuzz process itself. This is useful, among other
    things, for bootstrapping libdislocator.so.

  - Setting AFL_NO_UI inhibits the UI altogether, and just periodically prints
    some basic stats. This behavior is also automatically triggered when the
    output from afl-fuzz is redirected to a file or to a pipe.

  - If you are Jakub, you may need AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES.
    Others need not apply.

  - Benchmarking only: AFL_BENCH_JUST_ONE causes the fuzzer to exit after
    processing the first queue entry; and AFL_BENCH_UNTIL_CRASH causes it to
    exit soon after the first crash is found.

4) Settings for afl-qemu-trace
------------------------------

The QEMU wrapper used to instrument binary-only code supports several settings:

  - It is possible to set AFL_INST_RATIO to skip the instrumentation on some
    of the basic blocks, which can be useful when dealing with very complex
    binaries.

  - Setting AFL_INST_LIBS causes the translator to also instrument the code
    inside any dynamically linked libraries (notably including glibc).

  - The underlying QEMU binary will recognize any standard "user space
    emulation" variables (e.g., QEMU_STACK_SIZE), but there should be no
    reason to touch them.

5) Settings for afl-cmin
------------------------

The corpus minimization script offers very little customization:

  - Setting AFL_PATH offers a way to specify the location of afl-showmap
    and afl-qemu-trace (the latter only in -Q mode).

  - AFL_KEEP_TRACES makes the tool keep traces and other metadata used for
    minimization and normally deleted at exit. The files can be found in the
    `<out_dir>/.traces/*`.

  - AFL_ALLOW_TMP permits this and some other scripts to run in /tmp. This is
    a modest security risk on multi-user systems with rogue users, but should
    be safe on dedicated fuzzing boxes.

6) Settings for afl-tmin
------------------------

Virtually nothing to play with. Well, in QEMU mode (-Q), AFL_PATH will be
searched for afl-qemu-trace. In addition to this, TMPDIR may be used if a
temporary file can't be created in the current working directory.

You can specify AFL_TMIN_EXACT if you want afl-tmin to require execution paths
to match when minimizing crashes. This will make minimization less useful, but
may prevent the tool from "jumping" from one crashing condition to another in
very buggy software. You probably want to combine it with the -e flag.

7) Settings for afl-analyze
---------------------------

You can set AFL_ANALYZE_HEX to get file offsets printed as hexadecimal instead
of decimal.

8) Settings for libdislocator.so
--------------------------------

The library honors three environmental variables:

  - AFL_LD_LIMIT_MB caps the size of the maximum heap usage permitted by the
    library, in megabytes. The default value is 1 GB. Once this is exceeded,
    allocations will return NULL.

  - AFL_LD_HARD_FAIL alters the behavior by calling abort() on excessive
    allocations, thus causing what AFL would perceive as a crash. Useful for
    programs that are supposed to maintain a specific memory footprint.

  - AFL_LD_VERBOSE causes the library to output some diagnostic messages
    that may be useful for pinpointing the cause of any observed issues.

  - AFL_LD_NO_CALLOC_OVER inhibits abort() on calloc() overflows. Most
    of the common allocators check for that internally and return NULL, so
    it's a security risk only in more exotic setups.

9) Settings for libtokencap.so
------------------------------

This library accepts AFL_TOKEN_FILE to indicate the location to which the
discovered tokens should be written.

10) Third-party variables set by afl-fuzz & other tools
-------------------------------------------------------

Several variables are not directly interpreted by afl-fuzz, but are set to
optimal values if not already present in the environment:

  - By default, LD_BIND_NOW is set to speed up fuzzing by forcing the
    linker to do all the work before the fork server kicks in. You can
    override this by setting LD_BIND_LAZY beforehand, but it is almost
    certainly pointless.

  - By default, ASAN_OPTIONS are set to:

    abort_on_error=1
    detect_leaks=0
    symbolize=0
    allocator_may_return_null=1

    If you want to set your own options, be sure to include abort_on_error=1 -
    otherwise, the fuzzer will not be able to detect crashes in the tested
    app. Similarly, include symbolize=0, since without it, AFL may have
    difficulty telling crashes and hangs apart.

  - In the same vein, by default, MSAN_OPTIONS are set to:

    exit_code=86 (required for legacy reasons)    
    abort_on_error=1
    symbolize=0
    msan_track_origins=0
    allocator_may_return_null=1

    Be sure to include the first one when customizing anything, since some
    MSAN versions don't call abort() on error, and we need a way to detect
    faults.

.. _parallel-fuzzing:

Tips for parallel fuzzing
=========================

  This document talks about synchronizing afl-fuzz jobs on a single machine
  or across a fleet of systems. See README for the general instruction manual.

1) Introduction
---------------

Every copy of afl-fuzz will take up one CPU core. This means that on an
n-core system, you can almost always run around n concurrent fuzzing jobs with
virtually no performance hit (you can use the afl-gotcpu tool to make sure).

In fact, if you rely on just a single job on a multi-core system, you will
be underutilizing the hardware. So, parallelization is usually the right
way to go.

When targeting multiple unrelated binaries or using the tool in "dumb" (-n)
mode, it is perfectly fine to just start up several fully separate instances
of afl-fuzz. The picture gets more complicated when you want to have multiple
fuzzers hammering a common target: if a hard-to-hit but interesting test case
is synthesized by one fuzzer, the remaining instances will not be able to use
that input to guide their work.

To help with this problem, afl-fuzz offers a simple way to synchronize test
cases on the fly.

2) Single-system parallelization
--------------------------------

If you wish to parallelize a single job across multiple cores on a local
system, simply create a new, empty output directory ("sync dir") that will be
shared by all the instances of afl-fuzz; and then come up with a naming scheme
for every instance - say, "fuzzer01", "fuzzer02", etc. 

Run the first one ("master", -M) like this:

.. code-block:: console

  $ ./afl-fuzz -i testcase_dir -o sync_dir -M fuzzer01 [...other stuff...]

...and then, start up secondary (-S) instances like this:

.. code-block:: console

  $ ./afl-fuzz -i testcase_dir -o sync_dir -S fuzzer02 [...other stuff...]
  $ ./afl-fuzz -i testcase_dir -o sync_dir -S fuzzer03 [...other stuff...]

Each fuzzer will keep its state in a separate subdirectory, like so:

::

  /path/to/sync_dir/fuzzer01/

Each instance will also periodically rescan the top-level sync directory
for any test cases found by other fuzzers - and will incorporate them into
its own fuzzing when they are deemed interesting enough.

The difference between the -M and -S modes is that the master instance will
still perform deterministic checks; while the secondary instances will
proceed straight to random tweaks. If you don't want to do deterministic
fuzzing at all, it's OK to run all instances with -S. With very slow or complex
targets, or when running heavily parallelized jobs, this is usually a good plan.

Note that running multiple -M instances is wasteful, although there is an
experimental support for parallelizing the deterministic checks. To leverage
that, you need to create -M instances like so:

.. code-block:: console

  $ ./afl-fuzz -i testcase_dir -o sync_dir -M masterA:1/3 [...]
  $ ./afl-fuzz -i testcase_dir -o sync_dir -M masterB:2/3 [...]
  $ ./afl-fuzz -i testcase_dir -o sync_dir -M masterC:3/3 [...]

...where the first value after ':' is the sequential ID of a particular master
instance (starting at 1), and the second value is the total number of fuzzers to
distribute the deterministic fuzzing across. Note that if you boot up fewer
fuzzers than indicated by the second number passed to -M, you may end up with
poor coverage.

You can also monitor the progress of your jobs from the command line with the
provided afl-whatsup tool. When the instances are no longer finding new paths,
it's probably time to stop.

WARNING: Exercise caution when explicitly specifying the -f option. Each fuzzer
must use a separate temporary file; otherwise, things will go south. One safe
example may be:

.. code-block:: console

  $ ./afl-fuzz [...] -S fuzzer10 -f file10.txt ./fuzzed/binary @@
  $ ./afl-fuzz [...] -S fuzzer11 -f file11.txt ./fuzzed/binary @@
  $ ./afl-fuzz [...] -S fuzzer12 -f file12.txt ./fuzzed/binary @@

This is not a concern if you use @@ without -f and let afl-fuzz come up with the
file name.

3) Multi-system parallelization
-------------------------------

The basic operating principle for multi-system parallelization is similar to
the mechanism explained in section 2. The key difference is that you need to
write a simple script that performs two actions:

  - Uses SSH with authorized_keys to connect to every machine and retrieve
    a tar archive of the /path/to/sync_dir/<fuzzer_id>/queue/ directories for
    every <fuzzer_id> local to the machine. It's best to use a naming scheme
    that includes host name in the fuzzer ID, so that you can do something
    like:

    .. code-block:: bash

      for s in {1..10}; do
        ssh user@host${s} "tar -czf - sync/host${s}_fuzzid*/[qf]*" >host${s}.tgz
      done

  - Distributes and unpacks these files on all the remaining machines, e.g.:

    .. code-block:: bash

      for s in {1..10}; do
        for d in {1..10}; do
          test "$s" = "$d" && continue
          ssh user@host${d} 'tar -kxzf -' <host${s}.tgz
        done
      done

There is an example of such a script in experimental/distributed_fuzzing/;
you can also find a more featured, experimental tool developed by
Martijn Bogaard at:

  https://github.com/MartijnB/disfuzz-afl

Another client-server implementation from Richo Healey is:

  https://github.com/richo/roving

Note that these third-party tools are unsafe to run on systems exposed to the
Internet or to untrusted users.

When developing custom test case sync code, there are several optimizations
to keep in mind:

  - The synchronization does not have to happen very often; running the
    task every 30 minutes or so may be perfectly fine.

  - There is no need to synchronize crashes/ or hangs/; you only need to
    copy over queue/* (and ideally, also fuzzer_stats).

  - It is not necessary (and not advisable!) to overwrite existing files;
    the -k option in tar is a good way to avoid that.

  - There is no need to fetch directories for fuzzers that are not running
    locally on a particular machine, and were simply copied over onto that
    system during earlier runs.

  - For large fleets, you will want to consolidate tarballs for each host,
    as this will let you use n SSH connections for sync, rather than n*(n-1).

    You may also want to implement staged synchronization. For example, you
    could have 10 groups of systems, with group 1 pushing test cases only
    to group 2; group 2 pushing them only to group 3; and so on, with group
    eventually 10 feeding back to group 1.

    This arrangement would allow test interesting cases to propagate across
    the fleet without having to copy every fuzzer queue to every single host.

  - You do not want a "master" instance of afl-fuzz on every system; you should
    run them all with -S, and just designate a single process somewhere within
    the fleet to run with -M.

It is *not* advisable to skip the synchronization script and run the fuzzers
directly on a network filesystem; unexpected latency and unkillable processes
in I/O wait state can mess things up.

4) Remote monitoring and data collection
----------------------------------------

You can use screen, nohup, tmux, or something equivalent to run remote
instances of afl-fuzz. If you redirect the program's output to a file, it will
automatically switch from a fancy UI to more limited status reports. There is
also basic machine-readable information always written to the fuzzer_stats file
in the output directory. Locally, that information can be interpreted with
afl-whatsup.

In principle, you can use the status screen of the master (-M) instance to
monitor the overall fuzzing progress and decide when to stop. In this
mode, the most important signal is just that no new paths are being found
for a longer while. If you do not have a master instance, just pick any
single secondary instance to watch and go by that.

You can also rely on that instance's output directory to collect the
synthesized corpus that covers all the noteworthy paths discovered anywhere
within the fleet. Secondary (-S) instances do not require any special
monitoring, other than just making sure that they are up.

Keep in mind that crashing inputs are *not* automatically propagated to the
master instance, so you may still want to monitor for crashes fleet-wide
from within your synchronization or health checking scripts (see afl-whatsup).

5) Asymmetric setups
--------------------

It is perhaps worth noting that all of the following is permitted:

  - Running afl-fuzz with conjunction with other guided tools that can extend
    coverage (e.g., via concolic execution). Third-party tools simply need to
    follow the protocol described above for pulling new test cases from
    `out_dir/<fuzzer_id>/queue/*` and writing their own finds to sequentially
    numbered id:nnnnnn files in `out_dir/<ext_tool_id>/queue/*`.

  - Running some of the synchronized fuzzers with different (but related)
    target binaries. For example, simultaneously stress-testing several
    different JPEG parsers (say, IJG jpeg and libjpeg-turbo) while sharing
    the discovered test cases can have synergistic effects and improve the
    overall coverage.

    (In this case, running one -M instance per each binary is a good plan.)

  - Having some of the fuzzers invoke the binary in different ways.
    For example, 'djpeg' supports several DCT modes, configurable with
    a command-line flag, while 'dwebp' supports incremental and one-shot
    decoding. In some scenarios, going after multiple distinct modes and then
    pooling test cases will improve coverage.

  - Much less convincingly, running the synchronized fuzzers with different
    starting test cases (e.g., progressive and standard JPEG) or dictionaries.
    The synchronization mechanism ensures that the test sets will get fairly
    homogeneous over time, but it introduces some initial variability.
