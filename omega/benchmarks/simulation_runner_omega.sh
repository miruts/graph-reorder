#!/bin/bash
unweighted_apps=(pagerank cc) # Apps that run on unweighted graphs
weighted_apps=(sssp bfs)

directions=(pull push push-pull) # Graphit scheduling options 
threads=(executables_1 executables_16) # Multi and single threaded versions
graphs=(web-NotreDame Amazon0302 Slashdot0811 CA-AstroPh) # Input graphs for applications
acc='acc'
runner=./gem5_run_${acc}.sh # Omega runner script on OMEGA_Accelerator
input_path=../filtered # Root path of the original or unordered graphs
app_path=. # Application root path
dataset_path=../datasets # Root path of the reordered graphs
output="statistics_omega.json" # Output path for algorithm runtimes
truncate -s 0 $output # Empty the output file
temp="log.txt" # temp file

# Function runs applications on original (unordered) graphs
function original() {
	applications=(pagerank cc bfs)
	suffix='.sg'
	if [ $1 = 'weighted' ]
	then
		applications=(sssp)
		suffix='.wsg'
	fi
	for app in "${applications[@]}"; do
		echo -e "\n\t${app}: {" >> $output;
		if [ $app = 'pagerank' ]
		then
			vls=8
		else
			vls=4
		fi
		for g in "${graphs[@]}"; do
			echo -e "\n\t\t${g}: {" >> $output;
			g_name="${g}${suffix}"
			for t in "${threads[@]}"; do
				echo -e "\n\t\t\t${t}: {" >> $output;
				for d in "${directions[@]}"; do
					truncate -s 0 $temp
					./$runner $vls $app_path/$app/$t/$acc/"${app}_${d}-omega" $input_path/$g_name;
					python average.py;
					avg=$(<$temp);
					echo -e  "\t\t\t\t${d} runtime: $avg ms\n" >> $output;
				done
				echo -e "\t\t\t}" >> $output
			done
			echo -e "\t\t}" >> $output
		done
		echo -e "\t}" >> $output
	done
}

# Function runs applications on Gorder reordered graphs
function gorder() {
	for app in "${unweighted_apps[@]}";do
		echo -e "\n\t${app}: {" >> $output;
		if [ $app = 'pagerank' ]
		then
			vls=8
		else
			vls=4
		fi
		for g in "${graphs[@]}"; do
			g_name="${g}.sg"
			echo -e "\n\t\t${g}: {" >> $output
			for t in "${threads[@]}"; do
				echo -e "\n\t\t\t${t}: {" >> $output;
				for d in "${directions[@]}"; do
					truncate -s 0 $temp
					./$runner $app_path/$app/$t/$acc/"${app}_${d}-omega" $dataset_path/unweighted/gorder/$g_name;
					#cat $temp
					python average.py;
					avg=$(<$temp);
					echo -e  "\t\t\t\t${d} runtime: $avg ms\n" >> $output;
				done
				echo -e "\t\t\t}" >> $output;
			done
			echo -e "\t\t}" >> $output;
		done
		echo -e "\t}" >> $output;
	done
}
# Function runs applications on PH reordered graphs
function ph() {
	ph_variants=(indegree outdegree)
	suffix='.sg'
	dp="unweighted"
	applications=(pagerank cc)
	if [ $1 = 'weighted' ]
	then 
		applications=(sssp)
		suffix='.wsg'
		dp="weighted"
	fi
	if [ $1 == 'maintain' ]
	then
		applications=(bfs)
	fi
	for app in "${applications[@]}"; do
		echo -e "\n\t${app}: {" >> $output;
		if [ $app = 'pagerank' ]
		then
			vls=8
		else
			vls=4
		fi
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
					./$runner $vls $app_path/$app/$t/$acc/"${app}_${d}-omega" $dataset_path/$dp/ph/$g_name
					#cat $temp
					python average.py;
					avg=$(<$temp);
					echo -e  "\t\t\t\t${d} runtime: $avg ms\n" >> $output;
				done
				echo -e "\t\t\t}" >> $output;
			done
			echo -e "\t\t}" >> $output;
		done
		echo -e "\t}" >> $output;
	done
}
# Function runs applications on Block reordered graphs
function br() {
	suffix='.sg'
	applications=(pagerank cc)
	dp='unweighted'
	if [ $1 = 'weighted' ]
	then
		applications=(sssp)
		suffix='.wsg'
		dp='weighted'
	fi
	if [ $1 = 'maintain' ]
	then
		applications=(bfs)
	fi
	for app in "${applications[@]}"; do
		echo -e "\n\t${app}: {" >> $output;
		if [ $app = 'pagerank' ]
			then
				vls=8
			else
				vls=4
			fi
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
					./$runner $vls $app_path/$app/$t/$acc/"${app}_${d}-omega" $dataset_path/$dp/br/$g_name
					python average.py;
					avg=$(<$temp)
					echo -e "\t\t\t\t${d}: $avg ms\n" >> $output;
				done
				echo -e "\t\t\t}" >> $output;
			done
			echo -e "\t\t}" >> $output;
		done
		echo -e "\t}" >> $output;
	done
}

# Function runs applications on Rabbit Reordered graphs
function rabbit() {
	suffix='.sg'
	applications=(pagerank cc)
	dp='unweighted'
	if [ $1 = 'weighted' ]
	then
		applications=(sssp)
		suffix='.wsg'
		dp='weighted'
	fi
	if [ $1 = 'maintain' ]
	then 
		applications=(bfs)
	fi
	for app in "${applications[@]}"; do
		echo -e "\n\t${app}: {" >> $output;
		if [ $app = 'pagerank' ]
			then
				vls=8
			else
				vls=4
			fi
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
					./$runner $vls $app_path/$app/$t/$acc/"${app}_${d}-omega"  $dataset_path/$dp/rabbit/$g_name;
					#cat $temp
					python average.py;
					avg=$(<$temp)
					echo -e "\t\t\t\t${d}: $avg ms\n" >> $output;
				done
				echo -e "\t\t\t}" >> $output;
			done
			echo -e "\t\t}" >> $output;
		done
		echo -e "\t}" >> $output;
	done
}

# Function runs applications on degree ordered graphs
function deg() {
	deg_variants=(indegree outdegree)
	suffix='.sg'
	dp="unweighted"
	applications=(pagerank cc)
	if [ $1 = 'weighted' ]
	then 
		applications=(sssp)
		suffix='.wsg'
		dp="weighted"
	fi
	if [ $1 == 'maintain' ]
	then
		applications=(bfs)
	fi
	for app in "${applications[@]}"; do
		echo -e "\n\t${app}: {" >> $output;
		if [ $app = 'pagerank' ]
			then
				vls=8
			else
				vls=4
			fi
		for g in "${graphs[@]}"; do
			g_name="${g}${suffix}"
			echo -e "\n\t\t${g}: {" >> $output;
			if [ $1 = 'maintain' ]
			then
				g_name="maintained/${g_name}"
			fi
			for v in "${deg_variants[@]}"; do
				echo -e "\t\t\t${v}: {" >> $output
				for t in "${threads[@]}"; do
					echo -e "\n\t\t\t\t${t}: {" >> $output;
					for d in "${directions[@]}"; do
						truncate -s 0 $temp
						./$runner $vls $app_path/$app/$t/$acc/"${app}_${d}-omega" $dataset_path/$dp/deg/$v/$g_name;
						#cat $temp
						python average.py;
						avg=$(<$temp);
						echo -e  "\t\t\t\t\t${d} runtime: $avg ms\n" >> $output;
					done
					echo -e "\t\t\t\t}" >> $output;
				done
				echo -e "\t\t\t}" >> $output;
			done
			echo -e "\t\t}" >> $output;
		done
		echo -e "\t}" >> $output;
	done
}

# Function runs applications on clutered graphs
function cl() {
	cl_variants=(sort cluster)
	cl_variants2=(indegree outdegree) 
	suffix='.sg'
	dp="unweighted"
	applications=(pagerank cc)
	if [ $1 = 'weighted' ]
	then 
		applications=(sssp)
		suffix='.wsg'
		dp="weighted"
	fi
	if [ $1 == 'maintain' ]
	then
		applications=(bfs)
	fi
	for app in "${applications[@]}"; do
		echo -e "\n\t${app}: {" >> $output;
		if [ $app = 'pagerank' ]
			then
				vls=8
			else
				vls=4
			fi
		for g in "${graphs[@]}"; do
			g_name="${g}${suffix}"
			echo -e "\n\t\t${g}: {" >> $output;
			if [ $1 = 'maintain' ]
			then
				g_name="maintained/${g_name}"
			fi
			for v1 in "${cl_variants[@]}"; do
				echo -e "\n\t\t\t${v1}: {" >> $output
				for v2 in "${cl_variants2[@]}";do
					echo -e "\n\t\t\t\t${v2}: {" >> $output
					for t in "${threads[@]}"; do
						echo -e "\n\t\t\t\t\t${t}: {" >> $output;
						for d in "${directions[@]}"; do
							truncate -s 0 $temp
							./$runner $vls $app_path/$app/$t/$acc/"${app}_${d}-omega" $$dataset_path/$dp/cl/$v1/$v2/$g_name;
							#cat $temp
							python average.py;
							avg=$(<$temp);
							echo -e  "\t\t\t\t\t\t\t${d} runtime: $avg ms\n" >> $output;
						done
						echo -e "\t\t\t\t\t}" >> $output;
					done
					echo -e "\t\t\t\t}" >> $output
				done
				echo -e "\t\t\t}" >> $output
			done
			echo -e "\t\t}" >> $output;
		done
		echo -e "\t}" >> $output;
	done
}
# Invocations
echo -e "\nOriginal ordering: {" >> $output
original 'unweighted'
original 'weighted'
echo -e "}" >> $output
echo -e "\npH Reordering: {" >> $output
ph 'unweighted'
ph 'maintain'
ph 'weighted'
echo -e "}" >> $output
echo -e "\nDegree ordering: {" >> $output
deg 'unweighted'
deg 'maintain'
deg 'weighted'
echo -e "}" >> $output
echo -e "\nClustering: {" >> $output
cl 'unweighted'
cl 'maintain'
cl 'weighted'
echo -e "}"
echo -e "\nBlock Reordering: {" >> $output
br 'unweighted'
br 'maintain'
br 'weighted'
echo -e "}" >> $output
echo -e "\nRabbit Reordering: {" >> $output
rabbit 'unweighted'
rabbit 'maintain'
rabbit 'weighted'
echo -e "}" >> $output
echo -e "\nGorder reordering: {" >> $output;
gorder
echo -e "}" >> $output
