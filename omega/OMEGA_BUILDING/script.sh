#!/bin/bash
path=../benchmarks
apps=(pagerank bfs cc sssp)
directions=(pull push push-pull)
for a in "${apps[@]}";do
	for d in "${directions[@]}"; do
		g++ -o $path/$a/executables_1/acc/${a}_${d}-omega -std=c++14 -static -Wl,--section-start=.testmem=0x300000 /tmp/omega_static.tmp -I graphit/src/runtime_lib/ -O3 $path/$a/graphit_compiled/acc/${a}_benchmark.gt__${d}-omega.gt__.cpp OMEGA_API.cpp pthread.o
		g++ -o $path/$a/executables_16/acc/${a}_${d}-omega -std=c++14 -static -Wl,--section-start=.testmem=0x300000 /tmp/omega_static.tmp -DOPENMP -fopenmp -I graphit/src/runtime_lib/ -O3 $path/$a/graphit_compiled/acc/${a}_benchmark.gt__${d}-omega.gt__.cpp OMEGA_API.cpp pthread.o
	done
done
