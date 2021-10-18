#!/bin/bash
unweighted_apps=(pagerank cc) # algorithms that run on unweighted graphs
directions=(pull push) # Scheduling directions
threads=(single multi) # Single and multithreaded version of applications
graphs=(web-NotreDame) # The input graphs to run benchmark applications on

input_path=~/materials/sample_edgelists/original # Root path of original graph datasets
app_path=~/workloads/applications # Root path of applications 
dataset_path=~/workloads/datasets # Root path of reordered graph datasets
output="statistics_shuffled.json" # Output path for application runtimes
truncate -s 0 $output # Deletes previous statistics
temp="temp.txt" # temp file

# Function runs applications with original input graph (unordered)
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
		for g in "${graphs[@]}"; do
			echo -e "\n\t\t${g}: {" >> $output;
			g_name="${g}${suffix}"
			for t in "${threads[@]}"; do
				echo -e "\n\t\t\t${t}: {" >> $output;
				for d in "${directions[@]}"; do
					truncate -s 0 $temp
					for i in {1..10}; do
						$app_path/$t/$d/$app $input_path/$g_name;
						runtime=$(<"log.txt");
						echo -e $runtime >> $temp;
					done
					#cat $temp
					python3 average.py;
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
# Function runs gorder reordered graphs on unweighted_apps (pagerank cc)
function gorder() {
	for app in "${unweighted_apps[@]}";do
		echo -e "\n\t${app}: {" >> $output;
		for g in "${graphs[@]}"; do
			g_name="${g}.sg"
			echo -e "\n\t\t${g}: {" >> $output
			for t in "${threads[@]}"; do
				echo -e "\n\t\t\t${t}: {" >> $output;
				for d in "${directions[@]}"; do
					truncate -s 0 $temp
					for i in {1..10}; do
						$app_path/$t/$d/$app $dataset_path/unweighted/gorder/$g_name;
						runtime=$(<"log.txt");
						echo -e $runtime >> $temp;
					done
					#cat $temp
					python3 average.py;
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
# Function runs applications on ph reordered graphs
function ph() {
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
					for i in {1..10}; do
						$app_path/$t/$d/$app $dataset_path/$dp/ph/$g_name;
						runtime=$(<"log.txt");
						echo -e $runtime >> $temp;
					done
					#cat $temp
					python3 average.py;
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
# Function runs applications on block reordered graphs
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
					for i in  {1..10}; do
						$app_path/$t/$d/$app $dataset_path/$dp/br/$g_name;
						runtime=$(<"log.txt");
						echo -e $runtime >> $temp;
					done
					#cat $temp
					python3 average.py;
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
# Function runs applications on rabbit reordered graphs
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
				for i in  {1..10}; do
					$app_path/$t/$d/$app $dataset_path/$dp/rabbit/$g_name;
					runtime=$(<"log.txt");
					echo -e $runtime >> $temp;
				done
				#cat $temp
				python3 average.py;
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
						for i in {1..10}; do
							$app_path/$t/$d/$app $dataset_path/$dp/deg/$v/$g_name;
							runtime=$(<"log.txt");
							echo -e $runtime >> $temp;
						done
						#cat $temp
						python3 average.py;
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
# Function runs applications on clustered graphs.
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
							for i in {1..10}; do
								$app_path/$t/$d/$app $dataset_path/$dp/cl/$v1/$v2/$g_name;
								runtime=$(<"log.txt");
								echo -e $runtime >> $temp;
							done
							#cat $temp
							python3 average.py;
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