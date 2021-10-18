#!/bin/bash
apps=(pagerank bfs cc sssp) # benchmark applications
directions=(pull push) # scheduling options
path=~/workloads/applications/ # application base locations
for a in "${apps[@]}"; do
	for d in "${directions[@]}"; do
		# convert graphit to C++
		python3 graphitc.py -f ~/materials/graphit_schedules/${d}.gt -a graphit_apps/${a}_benchmark_m.gt -o ${a}_benchmark_${d}.cpp;
		# single-threaded executable
		g++ -std=c++14 -I ../../src/runtime_lib/ -O3 ${a}_benchmark_${d}.cpp graphit_apps/logger.cpp -o ${path}single/$d/$a;
		# multi-threaded executable
		g++ -std=c++14 -I ../../src/runtime_lib/ -O3 -DOPENMP -fopenmp ${a}_benchmark_${d}.cpp graphit_apps/logger.cpp -o ${path}multi/$d/$a;
	done
done
