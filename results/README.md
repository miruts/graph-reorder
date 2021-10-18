## Performance changes after reordering ##
Hereby is is the runtimes of different graph algorithms before and after being reordered by different reordering techniques.
PS. Runtimes are averages over 10 different runs for native(baseline) execution, while a single run is reported on gem5 and omega environment, because it's deterministic.

### 1. Application runtimes on gem5 simulation environment (original vs reordered) ###
Link to file: [runtimes_gem5.xlsx](runtimes_gem5.xlsx)
This Excel book contains runtimes & comparisons of reordered graphs and original graphs. The parameters and configurations of the gem5 simulator can be found [here](../scripts/simulation_runner_gem5.sh)
### 2. Application runtimes on baseline (original vs reordered) ###
Link to file: [runtimes_native.xlsx](runtimes_native.xlsx)
This Excel book contains runtimes & comparisons of reordered graphs and original graphs.
### 3. Application runtimes on baseline (shuffled vs reordered) ###
Link to file: [runtimes_shuffled_native.xlsx](runtimes_shuffled_native.xlsx)
This Excel book contains runtimes & comparisions of reordered graphs and randomly shuffled graphs.
### 4. Application runtimes on omega (original vs reordered) ###
Link to file: [runtimes_omega.xlsx](runtimes_omega.xlsx)
This Excel book contains runtimes & comparisons of reordered graphs and original graphs.
Some fields are empty because gem5 throws assertion error when changing application code. and those those apps that threw assertion are empty.