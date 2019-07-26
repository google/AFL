===============================
AFL (american fuzzy lop)
===============================

What is AFL?
============

American fuzzy lop is a security-oriented fuzzer that employs a novel type of
compile-time instrumentation and genetic algorithms to automatically discover
clean, interesting test cases that trigger new internal states in the targeted
binary. This substantially improves the functional coverage for the fuzzed code.
The compact synthesized corpora produced by the tool are also useful for seeding
other, more labor- or resource-intensive testing regimes down the road.

.. toctree::
  :maxdepth: 2

  quick_start
  motivation
  instrumenting
  fuzzing
  INSTALL
  user_guide
  notes_for_asan

  tips
  limitations

  about_afl
  related_projects


.. include:: ChangeLog