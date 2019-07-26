.. _instrumenting:

==============================
Instrumenting programs for AFL
==============================

When source code is available, instrumentation can be injected by a companion
tool that works as a drop-in replacement for gcc or clang in any standard build
process for third-party code.

The instrumentation has a fairly modest performance impact; in conjunction with
other optimizations implemented by afl-fuzz, most programs can be fuzzed as fast
or even faster than possible with traditional tools.

The correct way to recompile the target program may vary depending on the
specifics of the build process, but a nearly-universal approach would be:

.. code-block:: console

  $ CC=/path/to/afl/afl-gcc ./configure
  $ make clean all


For C++ programs, you'd would also want to set `CXX=/path/to/afl/afl-g++`.

The clang wrappers (afl-clang and afl-clang++) can be used in the same way;
clang users may also opt to leverage a higher-performance instrumentation mode,
as described in llvm_mode/README.llvm.

When testing libraries, you need to find or write a simple program that reads
data from stdin or from a file and passes it to the tested library. In such a
case, it is essential to link this executable against a static version of the
instrumented library, or to make sure that the correct .so file is loaded at
runtime (usually by setting `LD_LIBRARY_PATH`). The simplest option is a static
build, usually possible via:

.. code-block:: console

  $ CC=/path/to/afl/afl-gcc ./configure --disable-shared


Setting `AFL_HARDEN=1` when calling `make` will cause the CC wrapper to
automatically enable code hardening options that make it easier to detect
simple memory bugs. Libdislocator, a helper library included with AFL (see
libdislocator/README.dislocator) can help uncover heap corruption issues, too.

.. note::
  ASAN users are advised to review :ref:`asan-notes` for important caveats.


Instrumenting binary-only apps
==============================

When source code is *NOT* available, the fuzzer offers experimental support for
fast, on-the-fly instrumentation of black-box binaries. This is accomplished
with a version of QEMU running in the lesser-known "user space emulation" mode.

QEMU is a project separate from AFL, but you can conveniently build the
feature by doing:

.. code-block:: console

  $ cd qemu_mode
  $ ./build_qemu_support.sh


For additional instructions and caveats, see `qemu_mode/README.qemu`.

The mode is approximately 2-5x slower than compile-time instrumentation, is
less conductive to parallelization, and may have some other quirks.
