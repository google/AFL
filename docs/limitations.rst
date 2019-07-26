===========
Limitations
===========

Here are some of the most important caveats for AFL:

  - AFL detects faults by checking for the first spawned process dying due to
    a signal (SIGSEGV, SIGABRT, etc). Programs that install custom handlers for
    these signals may need to have the relevant code commented out. In the same
    vein, faults in child processed spawned by the fuzzed target may evade
    detection unless you manually add some code to catch that.

  - As with any other brute-force tool, the fuzzer offers limited coverage if
    encryption, checksums, cryptographic signatures, or compression are used to
    wholly wrap the actual data format to be tested.

    To work around this, you can comment out the relevant checks (see
    `experimental/libpng_no_checksum/` for inspiration); if this is not
    possible, you can also write a postprocessor, as explained in
    `experimental/post_library/`.

  - There are some unfortunate trade-offs with ASAN and 64-bit binaries. This
    isn't due to any specific fault of afl-fuzz; see :ref:`asan-notes`
    for tips.

  - There is no direct support for fuzzing network services, background
    daemons, or interactive apps that require UI interaction to work. You may
    need to make simple code changes to make them behave in a more traditional
    way. Preeny may offer a relatively simple option, too - see:
    https://github.com/zardus/preeny

    Some useful tips for modifying network-based services can be also found at:
    https://www.fastly.com/blog/how-to-fuzz-server-american-fuzzy-lop

  - AFL doesn't output human-readable coverage data. If you want to monitor
    coverage, use afl-cov from Michael Rash: https://github.com/mrash/afl-cov

  - Occasionally, sentient machines rise against their creators. If this
    happens to you, please consult http://lcamtuf.coredump.cx/prep/.

Beyond this, see INSTALL for platform-specific tips.


Risks
-----

Please keep in mind that, similarly to many other computationally-intensive
tasks, fuzzing may put strain on your hardware and on the OS. In particular:

  - Your CPU will run hot and will need adequate cooling. In most cases, if
    cooling is insufficient or stops working properly, CPU speeds will be
    automatically throttled. That said, especially when fuzzing on less
    suitable hardware (laptops, smartphones, etc), it's not entirely impossible
    for something to blow up.

  - Targeted programs may end up erratically grabbing gigabytes of memory or
    filling up disk space with junk files. AFL tries to enforce basic memory
    limits, but can't prevent each and every possible mishap. The bottom line
    is that you shouldn't be fuzzing on systems where the prospect of data loss
    is not an acceptable risk.

  - Fuzzing involves billions of reads and writes to the filesystem. On modern
    systems, this will be usually heavily cached, resulting in fairly modest
    "physical" I/O - but there are many factors that may alter this equation.
    It is your responsibility to monitor for potential trouble; with very heavy
    I/O, the lifespan of many HDDs and SSDs may be reduced.

    A good way to monitor disk I/O on Linux is the 'iostat' command:

.. code-block:: console

  $ iostat -d 3 -x -k [...optional disk ID...]
