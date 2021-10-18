#!/bin/bash
directions=(pull push) # Scheduling directions
threads=(single multi) # Single and multithreaded version of applications
graphs=(web-NotreDame Slashdot0811 CA-AstroPh Amazon0302) # The input graphs to run benchmark applications on
app_path=~/workloads/applications # Root path of applications 
dataset_path=~/workloads/datasets # Root path of reordered graph datasets
original_path=~/materials/sample_edgelists/original # Root path of original graph datasets
gem5=~/gem5/build/X86/gem5.opt # gem5 executable path
config=~/gem5/configs/example/se.py # gem5 configuration path
# gem5 parameters' path
params="--cpu-type=TimingSimpleCPU --num-cpus=1 --sys-clock=2GHz --caches --cacheline_size=64 --num-dirs=4 --mem-size=4GB --l1i_size=16kB --l1i_assoc=4 --l1d_size=16kB --l1d_assoc=8 --l2cache --num-l2caches=1 --l2_size=2MB --l2_assoc=8 --ruby --topology=Crossbar"
output="statistics_gem5.json" # Output path for application runtimes
temp="temp.txt" # temp file
truncate -s 0 $output # empties output file path
# Function runs applications on original graphs (unordered)
function original() {
	applications=(pagerank cc)
	suffix='.sg'
	if [ $1 = "weighted" ]
	then 
		applications=(sssp);
		suffix='.wsg'
	fi
	if [ $1 = 'maintain' ]
	then
		applications=(bfs)
	fi
	for app in "${applications[@]}"; do
		echo -e "\n\t${app}: {" >> $output;
		for g in "${graphs[@]}"; do
			echo -e "\n\t\t${g}: {" >> $output;
			g_name="${g}${suffix}"
			for t in "${threads[@]}"; do
				echo -e "\n\t\t\t${t}: {" >> $output;
				for d in "${directions[@]}"; do
					truncate -s 0 $temp
					$gem5 $config $params -c $app_path/$t/$d/$app -o $original_path/$g_name;
					runtime=$(<"log.txt");
					echo -e $runtime >> $temp;
					python3 average.py;
					avg=$(<$temp)
					echo -e "\t\t\t\t${d} runtime: $avg ms\n" >> $output;
				done
				echo -e "\n\t\t\t}" >> $output
			done
			echo -e "\n\t\t}" >> $output
		done
		echo -e "\n\t}" >> $output
	done
}
# Function runs applications on degree ordered graphs
function deg() {
	applications=(pagerank cc)
	suffix='.sg'
	dp='unweighted'
	deg_variants=(indegree outdegree)
	if [ $1 = "weighted" ]
	then 
		dp='weighted'
		applications=(sssp);
		suffix='.wsg'
	fi
	if [ $1 = 'maintain' ]
	then
		applications=(bfs)
	fi
	for app in "${applications[@]}"; do
		echo -e "\n\t${app}: {" >> $output;
		for g in "${graphs[@]}"; do
			echo -e "\n\t\t${g}: {" >> $output;
			g_name="${g}${suffix}"
			if [ $1 = 'maintain' ]
			then
				g_name="maintained/${g_name}"
			fi
			for t in "${threads[@]}"; do
				echo -e "\n\t\t\t${t}: {" >> $output;
				for v in "${deg_variants[@]}"; do
					echo -e "\n\t\t\t\t${v}: {" >> $output;
					for d in "${directions[@]}"; do
						truncate -s 0 $temp
						$gem5 $config $params -c $app_path/$t/$d/$app -o $dataset_path/$dp/deg/$v/$g_name;
						runtime=$(<"log.txt");
						echo -e $runtime >> $temp;
						python3 average.py;
						avg=$(<$temp);
						echo -e "\n\t\t\t\t\t${d} runtime: $avg ms\n " >> $output;
					done
					echo -e "\n\t\t\t\t}" >> $output;
				done
				echo -e "\n\t\t\t}" >> $output;
			done
			echo -e "\n\t\t}" >> $output;
		done
		echo -e "\n\t}" >> $output;
	done
}

# Function runs applications on clustred graphs
function cl() {
	cl_variants=(sort cluster)
	cl_variants2=(indegree outdegree)
	applications=(pagerank cc)
	dp='unweighted'
	suffix='.sg'
	if [ $1 = "weighted" ]
	then
		applications=(sssp);
		suffix='.wsg'
		dp='weighted'
	fi
	if [ $1 = 'maintain' ]
	then
		applications=(bfs)
	fi
	echo -e "\nClustering: {" >> $output
	for app in "${applications[@]}"; do
		echo -e "\n\t${app}: {" >> $output
		for g in "${graphs[@]}"; do
			echo -e "\n\t\t${g}: {" >> $output;
			g_name="${g}${suffix}"
			if [ $1 = 'maintain' ]
			then
				g_name="maintained/${g_name}"
			fi
			for t in "${threads[@]}"; do
				echo -e "\n\t\t\t${t}: {" >> $output;
				for type in "${cl_variants[@]}"; do
					echo -e "\n\t\t\t\t${tyep}: {" >> $output;
					for type2 in "${cl_variants2[@]}"; do
						echo -e "\n\t\t\t\t\t${type2}: {" >> $output;
						for d in "${directions[@]}"; do
							truncate -s 0 $temp
							$gem5 $config $params -c $app_path/$t/$d/$app -o $dataset_path/$dp/cl/$type/$type2/$g_name;
							runtime=$(<"log.txt");
							echo -e $runtime >> $temp;
							python3 average.py;
							avg=$(<$temp);
							echo -e "\n\t\t\t\t\t\t${d} runtime: $avg ms\n " >> $output;
						done
						echo -e "\n\t\t\t\t\t}" >> $output;
					done
					echo -e "\n\t\t\t\t}" >> $output;
				done
				echo -e "\n\t\t\t}" >> $output;
			done
			echo -e "\n\t\t}" >> $output;
		done
		echo -e "\n\t}" >> $output;
	done
}

# Function runs applications on ph reordered graphs
function ph(){
	applications=(pagerank cc)
	suffix='.sg'
	dp='unweighted'
	if [ $1 = "weighted" ]
	then 
		applications=(sssp);
		suffix='.wsg'
		dp='weighted'
	fi
	if [ $1 = 'maintain' ]
	then
		applications=(bfs)
	fi
	for app in "${applications[@]}"; do
		echo -e "\n\t${app}: {" >> $output;
		for g in "${graphs[@]}"; do
			g_name="${g}${suffix}"
			echo -e "\n\t\t${g}: {" >> $output;
			if [ $1 = 'maintain' ]
			then
				g_name="maintained/${g_name}"
			fi
			for t in "${threads[@]}"; do
				echo -e "\n\t\t\t${t}: {" >> $output;
				for d in "${directions[@]}"; do
					truncate -s 0 $temp
					$gem5 $config $params -c $app_path/$t/$d/$app -o $dataset_path/$dp/ph/$g_name;
					runtime=$(<"log.txt");
					echo -e $runtime >> $temp;
					python3 average.py;
					avg=$(<$temp);
					echo -e  "\t\t\t\t${d} runtime: $avg ms" >> $output;
				done
				echo -e "\n\t\t\t}" >> $output;
			done
			echo -e "\n\t\t}" >> $output;
		done
		echo -e "\n\t}" >> $output;
	done
}
# Function runs applications on block reordered graphs.
function br() {
	applications=(pagerank cc)
	suffix='.sg'
	dp='unweighted'
	if [ $1 = "weighted" ]
	then 
		applications=(sssp);
		suffix='.wsg'
		dp='weighted'
	fi
	if [ $1 = 'maintain' ]
	then
		applications=(bfs)
	fi
	for app in "${applications[@]}"; do
		echo -e "\n\t${app}: {" >> $output;
		for g in "${graphs[@]}"; do
			g_name="${g}.el"
			echo -e "\n\t\t${g}: {" >> $output;
			if [ $1 = 'maintain' ]
			then
				g_name="maintained/${g_name}"
			fi
			for t in "${threads[@]}"; do
				echo -e "\n\t\t\t${t}: {" >> $output;
				for d in "${directions[@]}"; do
					truncate -s 0 $temp;
					$gem5 $config $params -c $app_path/$t/$d/$app -o $dataset_path/$dp/br/$g_name;
					runtime=$(<"log.txt");
					echo -e $runtime >> $temp;
					python3 average.py;
					avg=$(<$temp)
					echo -e "\t\t\t\t${d} runtime: $avg seconds" >> $output;
				done
				echo -e "\n\t\t\t}" >> $output;
			done
			echo -e "\n\t\t}" >> $output;
		done
		echo -e "\n\t}" >> $output;
	done
}

# Function runs applications on rabbit reordered graphs
function rabbit() {
	applications=(pagerank cc)
	suffix='.sg'
	dp='unweighted'
	if [ $1 = "weighted" ]
	then 
		applications=(sssp);
		dp='weighted'
		suffix='.wsg'
	fi
	if [ $1 = 'maintain' ]
	then
		applications=(bfs)
	fi
	for app in "${applications[@]}"; do
		echo -e "\n\t${app}: {" >> $output;
		for g in "${graphs[@]}"; do
			g_name="${g}${suffix}"
			echo -e "\n\t\t${g}: {" >> $output;
			if [ $1 = 'maintain' ]
			then
				g_name="maintained/${g_name}"
			fi
			for t in "${threads[@]}"; do
				echo -e "\n\t\t\t${t}: {" >> $output;
				for d in "${directions[@]}"; do
					truncate -s 0 $temp
					$gem5 $config $params -c $app_path/$t/$d/$app -o $dataset_path/$dp/rabbit/$g_name;
					runtime=$(<"log.txt");
					echo -e $runtime >> $temp;
					python3 average.py;
					avg=$(<$temp)
					echo -e "\t\t\t\t${d} runtime: $avg seconds" >> $output;
				done
				echo -e "\n\t\t\t}" >> $output;
			done
			echo -e "\n\t\t}" >> $output;
		done
		echo -e "\n\t}" >> $output;
	done
}

# Function runs applications on GOrder reordered graphs
function gorder() {
	applications=(pagerank cc)
	suffix='.sg'
	for app in "${applications[@]}"; do
		echo -e "\n\t${app}: {" >> $output;
		for g in "${graphs[@]}"; do
			g_name="${g}.el"
			echo -e "\n\t\t${g}: {" >> $output;
			for t in "${threads[@]}"; do
				echo -e "\n\t\t\t${t}: {" >> $output;
				for d in "${directions[@]}"; do
					truncate -s o $temp;
					$gem5 $config $params -c $app_path/$t/$d/$app -o $dataset_path/unweighted/gorder/$g_name;
					runtime=$(<"log.txt");
					echo -e $runtime >> $temp;
					python3 average.py;
					avg=$(<$temp)
					echo -e "\t\t\t\t${d} runtime: $avg seconds" >> $output;
				done
				echo -e "\n\t\t\t}" >> $output;
			done
			echo -e "\n\t\t}" >> $output;
		done
		echo -e "\n\t}" >> $output;
	done
}
# Invocations
echo -e "\nOriginal Ordering: {" >> $output
original 'unweighted'
original 'maintain'
original 'weighted'
echo -e "\n}" >> $output
echo -e "\npH reeordering: {" >> $output
ph 'unweighted'
ph 'maintain'
ph 'weighted'
echo -e "\n}" >> $output
echo -e "\nBlock reordering: {" >> $output
br 'unweighted'
br 'maintain'
br 'weighted'
echo -e "\n}" >> $output
echo -e "\nRabbit reordering: {" >> $output
rabbit 'unweighted'
rabbit 'maintain'
rabbit 'weighted'
echo -e "\n}" >> $output
echo -e "\nDegree ordering: {" >> $output
deg 'unweighted'
deg 'maintain'
deg 'weighted'
echo -e "\n}" >> $output
echo -e "\nCluster reordering: {" >> $output
cl 'unweighted'
cl 'maintain'
cl 'weighted'
echo -e "\n}" >> $output
echo -e "\nGOrder reordering: {" >> $output
gorder
echo -e "\n}" >> $output
