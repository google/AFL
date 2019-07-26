====
Tips
====

.. _performance-tips:

Performance Tips
================

  This file provides tips for troubleshooting slow or wasteful fuzzing jobs.

1) Keep your test cases small
-----------------------------

This is probably the single most important step to take! Large test cases do
not merely take more time and memory to be parsed by the tested binary, but
also make the fuzzing process dramatically less efficient in several other
ways.

To illustrate, let's say that you're randomly flipping bits in a file, one bit
at a time. Let's assume that if you flip bit #47, you will hit a security bug;
flipping any other bit just results in an invalid document.

Now, if your starting test case is 100 bytes long, you will have a 71% chance of
triggering the bug within the first 1,000 execs - not bad! But if the test case
is 1 kB long, the probability that we will randomly hit the right pattern in
the same timeframe goes down to 11%. And if it has 10 kB of non-essential
cruft, the odds plunge to 1%.

On top of that, with larger inputs, the binary may be now running 5-10x times
slower than before - so the overall drop in fuzzing efficiency may be easily
as high as 500x or so.

In practice, this means that you shouldn't fuzz image parsers with your
vacation photos. Generate a tiny 16x16 picture instead, and run it through
jpegtran or pngcrunch for good measure. The same goes for most other types
of documents.

There's plenty of small starting test cases in `../testcases/*` - try them out
or submit new ones!

If you want to start with a larger, third-party corpus, run afl-cmin with an
aggressive timeout on that data set first.

2) Use a simpler target
-----------------------

Consider using a simpler target binary in your fuzzing work. For example, for
image formats, bundled utilities such as djpeg, readpng, or gifhisto are
considerably (10-20x) faster than the convert tool from ImageMagick - all while
exercising roughly the same library-level image parsing code.

Even if you don't have a lightweight harness for a particular target, remember
that you can always use another, related library to generate a corpus that will
be then manually fed to a more resource-hungry program later on.

3) Use LLVM instrumentation
---------------------------

When fuzzing slow targets, you can gain 2x performance improvement by using
the LLVM-based instrumentation mode described in llvm_mode/README.llvm. Note
that this mode requires the use of clang and will not work with GCC.

The LLVM mode also offers a "persistent", in-process fuzzing mode that can
work well for certain types of self-contained libraries, and for fast targets,
can offer performance gains up to 5-10x; and a "deferred fork server" mode
that can offer huge benefits for programs with high startup overhead. Both
modes require you to edit the source code of the fuzzed program, but the
changes often amount to just strategically placing a single line or two.

4) Profile and optimize the binary
----------------------------------

Check for any parameters or settings that obviously improve performance. For
example, the djpeg utility that comes with IJG jpeg and libjpeg-turbo can be
called with:

.. code-block:: console

  -dct fast -nosmooth -onepass -dither none -scale 1/4

...and that will speed things up. There is a corresponding drop in the quality
of decoded images, but it's probably not something you care about.

In some programs, it is possible to disable output altogether, or at least use
an output format that is computationally inexpensive. For example, with image
transcoding tools, converting to a BMP file will be a lot faster than to PNG.

With some laid-back parsers, enabling "strict" mode (i.e., bailing out after
first error) may result in smaller files and improved run time without
sacrificing coverage; for example, for sqlite, you may want to specify -bail.

If the program is still too slow, you can use strace -tt or an equivalent
profiling tool to see if the targeted binary is doing anything silly.
Sometimes, you can speed things up simply by specifying /dev/null as the
config file, or disabling some compile-time features that aren't really needed
for the job (try ./configure --help). One of the notoriously resource-consuming
things would be calling other utilities via exec*(), popen(), system(), or
equivalent calls; for example, tar can invoke external decompression tools
when it decides that the input file is a compressed archive.

Some programs may also intentionally call `sleep()`, `usleep()`, or
`nanosleep()`; vim is a good example of that. Other programs may attempt
`fsync()` and so on. There are third-party libraries that make it easy to get
rid of such code, e.g.:

  https://launchpad.net/libeatmydata

In programs that are slow due to unavoidable initialization overhead, you may
want to try the LLVM deferred forkserver mode (see llvm_mode/README.llvm),
which can give you speed gains up to 10x, as mentioned above.

Last but not least, if you are using ASAN and the performance is unacceptable,
consider turning it off for now, and manually examining the generated corpus
with an ASAN-enabled binary later on.

5) Instrument just what you need
--------------------------------

Instrument just the libraries you actually want to stress-test right now, one
at a time. Let the program use system-wide, non-instrumented libraries for
any functionality you don't actually want to fuzz. For example, in most
cases, it doesn't make to instrument libgmp just because you're testing a
crypto app that relies on it for bignum math.

Beware of programs that come with oddball third-party libraries bundled with
their source code (Spidermonkey is a good example of this). Check ./configure
options to use non-instrumented system-wide copies instead.

6) Parallelize your fuzzers
---------------------------

The fuzzer is designed to need ~1 core per job. This means that on a, say,
4-core system, you can easily run four parallel fuzzing jobs with relatively
little performance hit. For tips on how to do that, see :ref:`parallel-fuzzing`.

The `afl-gotcpu` utility can help you understand if you still have idle CPU
capacity on your system. (It won't tell you about memory bandwidth, cache
misses, or similar factors, but they are less likely to be a concern.)

7) Keep memory use and timeouts in check
----------------------------------------

If you have increased the -m or -t limits more than truly necessary, consider
dialing them back down.

For programs that are nominally very fast, but get sluggish for some inputs,
you can also try setting -t values that are more punishing than what afl-fuzz
dares to use on its own. On fast and idle machines, going down to -t 5 may be
a viable plan.

The -m parameter is worth looking at, too. Some programs can end up spending
a fair amount of time allocating and initializing megabytes of memory when
presented with pathological inputs. Low -m values can make them give up sooner
and not waste CPU time.

8) Check OS configuration
-------------------------

There are several OS-level factors that may affect fuzzing speed:

  - High system load. Use idle machines where possible. Kill any non-essential
    CPU hogs (idle browser windows, media players, complex screensavers, etc).

  - Network filesystems, either used for fuzzer input / output, or accessed by
    the fuzzed binary to read configuration files (pay special attention to the
    home directory - many programs search it for dot-files).

  - On-demand CPU scaling. The Linux 'ondemand' governor performs its analysis
    on a particular schedule and is known to underestimate the needs of
    short-lived processes spawned by afl-fuzz (or any other fuzzer). On Linux,
    this can be fixed with:

    .. code-block:: console

      cd /sys/devices/system/cpu
      echo performance | tee cpu*/cpufreq/scaling_governor

    On other systems, the impact of CPU scaling will be different; when fuzzing,
    use OS-specific tools to find out if all cores are running at full speed.

  - Transparent huge pages. Some allocators, such as jemalloc, can incur a
    heavy fuzzing penalty when transparent huge pages (THP) are enabled in the
    kernel. You can disable this via:

    .. code-block:: console

      echo never > /sys/kernel/mm/transparent_hugepage/enabled

  - Suboptimal scheduling strategies. The significance of this will vary from
    one target to another, but on Linux, you may want to make sure that the
    following options are set:

    .. code-block:: console

      echo 1 >/proc/sys/kernel/sched_child_runs_first
      echo 1 >/proc/sys/kernel/sched_autogroup_enabled

    Setting a different scheduling policy for the fuzzer process - say
    SCHED_RR - can usually speed things up, too, but needs to be done with
    care.

9) If all other options fail, use -d
------------------------------------

For programs that are genuinely slow, in cases where you really can't escape
using huge input files, or when you simply want to get quick and dirty results
early on, you can always resort to the `-d` mode.

The mode causes afl-fuzz to skip all the deterministic fuzzing steps, which
makes output a lot less neat and can ultimately make the testing a bit less
in-depth, but it will give you an experience more familiar from other fuzzing
tools.


AFL "Life Pro Tips"
===================

Bite-sized advice for those who understand the basics, but can't be bothered to
read or memorize every other piece of documentation for AFL.

.. tip::

  Get more bang for your buck by using fuzzing dictionaries.
  See `dictionaries/README.dictionaries` to learn how.

.. tip::

  You can get the most out of your hardware by parallelizing AFL jobs.
  See :ref:`parallel-fuzzing` for step-by-step tips.

.. tip::

  Improve the odds of spotting memory corruption bugs with libdislocator.so!
  It's easy. Consult libdislocator/README.dislocator for usage tips.

.. tip::

  Want to understand how your target parses a particular input file?
  Try the bundled afl-analyze tool; it's got colors and all!

.. tip::

  You can visually monitor the progress of your fuzzing jobs.
  Run the bundled afl-plot utility to generate browser-friendly graphs.

.. tip::

  Need to monitor AFL jobs programmatically? Check out the fuzzer_stats file
  in the AFL output dir or try afl-whatsup.

.. tip::

  Puzzled by something showing up in red or purple in the AFL UI?
  It could be important - consult :ref:`status-screen` right away!

.. tip::

  Know your target? Convert it to persistent mode for a huge performance gain!
  Consult section #5 in llvm_mode/README.llvm for tips.

.. tip::

  Using clang? Check out llvm_mode/ for a faster alternative to afl-gcc!

.. tip::

  Did you know that AFL can fuzz closed-source or cross-platform binaries?
  Check out qemu_mode/README.qemu for more.

.. tip::

  Did you know that afl-fuzz can minimize any test case for you?
  Try the bundled afl-tmin tool - and get small repro files fast!

.. tip::

  Not sure if a crash is exploitable? AFL can help you figure it out. Specify
  -C to enable the peruvian were-rabbit mode. See :ref:`crash-triage` for more.

.. tip::

  Trouble dealing with a machine uprising? Relax, we've all been there.
  Find essential survival tips at http://lcamtuf.coredump.cx/prep/.

.. tip::

  AFL-generated corpora can be used to power other testing processes.
  See :ref:`afl-approach` for inspiration - it tends to pay off!

.. tip::

  Want to automatically spot non-crashing memory handling bugs?
  Try running an AFL-generated corpus through ASAN, MSAN, or Valgrind.

.. tip::

  Good selection of input files is critical to a successful fuzzing job.
  See section :ref:`fuzzing-with-afl` in README (or :ref:`performance-tips`)
  for pro tips.

.. tip::

  You can improve the odds of automatically spotting stack corruption issues.
  Specify `AFL_HARDEN=1` in the environment to enable hardening flags.

.. tip::

  Bumping into problems with non-reproducible crashes? It happens, but usually
  isn't hard to diagnose. See the bottom of :ref:`interpreting-output` for tips.

.. tip::

  Fuzzing is not just about memory corruption issues in the codebase. Add some
  sanity-checking assert() / abort() statements to effortlessly catch logic bugs.

.. tip::

  Hey kid... pssst... want to figure out how AFL really works?
  Check out :ref:`technical-details` for all the gory details in one place!

.. tip::

  There's a ton of third-party helper tools designed to work with AFL!
  Be sure to check out :ref:`related-projects` before writing your own.

.. tip::

  Need to fuzz the command-line arguments of a particular program?
  You can find a simple solution in experimental/argv_fuzzing.

.. tip::

  Attacking a format that uses checksums? Remove the checksum-checking code or
  use a postprocessor! See experimental/post_library/ for more.

.. tip::

  Dealing with a very slow target or hoping for instant results? Specify -d
  when calling afl-fuzz!