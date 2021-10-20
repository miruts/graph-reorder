Contents of this folder:

  gem5:
    -Contains an older version of gem5 modified to implement OMEGA functionality

  benchmarks:
    -Contains the C++ source files for OMEGA-accelerated and baseline versions of GraphIt:
      -PageRank
      -Breadth-First Search (BFS)
      -Connected Components (CC)
      -Single-Source Shortest Path (SSSP)
      
    -Each algorithm's directory contains:
      -graphit_compiled: C++ source files for OMEGA-accelerated (acc) and baseline (std) implementations of the algorithm with different apply directions (SparsePush, DensePull, SparsePush-DensePull). Note that PageRank does not have a SparsePush-DensePull implementation (since the number of active vertices does not change, the Push-Pull just adds overhead).
      -executables_16  : Pre-compiled and tested executables for each C++ source file configured to run 16 threads (what we evaluate with)
      -executables_1   : Same as 'executables_16', but configured to run 1 thread
      
    -OMEGA_API.cpp and OMEGA_API.h: provide functions that we use to interact with OMEGA's hardware features
    
    -gem5_run_std.sh: runs gem5 in our baseline configuration
      -Usage: gem5_run_std.sh <exe_path> <input_args>
        -<exe_path>  : path to the executable to run
        -<input_args>: input arguments (e.g. path to an input graph)
    
    -gem5_run_acc.sh: runs gem5 in our OMEGA configuration
      -Usage: gem5_run_acc.sh <vertexline_size> <exe_path> <input_args>
        -<vertexline_size>: the number of bytes in each line of scratchpad memory; this should match the size of elements mapped to SPM
        
        
Building and running gem5:

  -This version of gem5 has compatibility issues with newer versions of Ubuntu, so we'll be using Docker to have an Ubuntu 16.04 environment with the required packages installed.
  
  -On poisonivy, running the command "docker images" should return something like:
   
        REPOSITORY        TAG       IMAGE ID       CREATED      SIZE
        aura_16.04_gem5   latest    ae02b88a2145   2 days ago   549MB
        
  -"ae02b88a2145" is the modified Ubuntu 16.04 image that we'll be using to create a Docker container
  -To run this image in a Docker container, I recommend using the command:
  
    docker run -u $UID:$GID --volume <path>:/volume_files --rm -it ae02b88a2145
    
  -Where <path> is the path to this folder (the one that contains "gem5" and "benchmarks")
  -After starting the container and running the command "ls" you should see something like:
  
    I have no name!@5debf2e92150:/$ ls
    bin  boot  dev	etc  home  lib	lib64  media  mnt  opt	proc  root  run  sbin  srv  sys  tmp  usr  var	volume_files
    I have no name!@5debf2e92150:/$ 

  -You can ignore the "I have no name"; the string after @ is the container ID and will vary
  -The directory "volume_files" is the directory that you specified via <path>. Any files contained in this directory will be accessible while in this Docker container and any changes to the directory contents will persist after the container is closed.
  
  -To build gem5, navigate to the gem5 directory and run the command:
  
    scons build/X86/gem5.opt PROTOCOL=MESI_Two_Level -j12
    
  -To run any of the pre-built executables, first navigate to the benchmarks directory
  
    -Example for a baseline app:
    
      ./gem5_run_std.sh pagerank/executables/std/pagerank_pull
  
    -Example for an OMEGA app:
    
      ./gem5_run_acc.sh 8 pagerank/executables/acc/pagerank_pull-omega
  
    -For the included OMEGA apps, the following vertexline_sizes (first argument for gem5_run_acc.sh) should be used:
      -PageRank: 8 (since it maps doubles to SPM)
      -BFS:      4 (since it maps ints to SPM)
      -CC:       4 (since it maps ints to SPM)
      -SSSP:     4 (since it maps ints to SPM)
  
  -To exit the Docker container, run: "exit"
  

Building OMEGA-accelerated Executables
  
  -I've included the C++ source files for the pre-built executables mostly as a reference. If you need to modify and build these executables (or some other application) yourself to run on OMEGA/this version of gem5, let me know and we can discuss the process. The build process is somewhat fussy and difficult to explain just in text.
