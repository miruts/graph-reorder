Contents:
  - graphit             : the version of GraphIt used for my work with OMEGA (known to be compatible)
  - allocateStaticMem.sh: script that creates a dummy file used during compilation to enable OMEGA's memory-mapped IO
  - pthread.o           : object file used during compilation to enable multithreading on this version of gem5
  - test.sg, test.wsg   : tiny graphs that I use when testing the stability of OMEGA/baseline applications in gem5


Building OMEGA/baseline applications for this version of gem5:

  Run the "allocateStaticMem.sh" script (only required once per Docker session)

  To build either the OMEGA or baseline applications:
    
      g++ -o <executable path> -std=c++14 -static -Wl,--section-start=.testmem=0x300000 /tmp/omega_static.tmp -DOPENMP -fopenmp -I ../graphit/src/runtime_lib/ -O3 <C++ source path> OMEGA_API.cpp pthread.o

      (Note that "../graphit/src/runtime_lib/" may need to be adjusted for your directory structure)


Hack for fixing an occasional gem5 assertion:

  Since I've been working with OMEGA, there's been a bug related to incompatibilities(?) between this build system and gem5 that causes the simulator to throw an "!event->scheduled()" assertion occasionally. An example is below:

    gem5.opt: build/X86/sim/eventq_impl.hh:46: void EventQueue::schedule(Event*, Tick, bool): Assertion `!event->scheduled()' failed.
    Program aborted at tick 96054500
    Aborted (core dumped)

  Instead of fixing the bug (it seems like it'd be difficult to completely resolve), I've been using a hacky solution to get around it.
  If this assertion is thrown for an application, open the source and find the segment in "main" that looks like:

    // Should match the core count in gem5
    setWorkers(16);

  Now, add or remove duplicate calls to "setWorkers". For example:

    // Should match the core count in gem5
    setWorkers(16);
    setWorkers(16);
    setWorkers(16);
    setWorkers(16);

  Retest, and add or remove these calls until the simulation completes successfully. Once the executable works for one input it almost always works for every other input (I've only had 1 instance where a certain input still triggered the assert). In short, this process only needs to be repeated if you make changes to the source code.

  The "setWorkers" function doesn't seem to be significant (only 1 is required for proper multithreading). In the past, I found that adding cout/printf statements could also "fix" the executable. I've just found duplicating this function convenient. The actual cause of the bug seems to be related to code placement and instruction caching; adding these duplicate calls (or other additional code) shifts instructions around and coincidentally fixes or breaks the simulation run.

  Tip: when adjusting the number of "setWorkers" calls, run the executable in gem5 on a very small graph to save time (I've included test.sg and test.wsg for this purpose)
