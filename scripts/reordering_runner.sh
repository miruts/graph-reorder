#!/bin/bash
el=.el
wel=.wel
graphs=(Pokec soc-LiveJournal1 roadNet-PA) # Graphs reordering is applied on without extension
input=~/materials/sample_edgelists/original # Original graph path
output=~/workloads/shuffled/datasets # Reordered graph output path

# Function to remove temp file
rmvr() {
	rm $1/new_order.el
}
# Function runs ph reordering  on both weighted, unweighted and initial vertex maintained & unmaintained and also converts each to sg or wsg format
function ph() {
	for i in  "${graphs[@]}"; do
		elloc="${i}.el"
		welloc="${i}.wel"
		python3 ph/ph.py -p=ph/ph $input/$elloc $output/unweighted/ph/$elloc; rmvr ph;
		python3 ph/ph.py -m -p=ph/ph $input/$elloc $output/unweighted/ph/maintained/$elloc; rmvr ph;
		python3 ph/ph.py -m -w -p=ph/ph $input/$welloc $output/weighted/ph/$welloc; rmvr ph;
		./converter -f $output/unweighted/ph/$elloc -b $output/unweighted/ph/${i}.sg;
		./converter -f $output/weighted/ph/$welloc -w -b $output/weighted/ph/${i}.wsg;
		./converter -f $output/unweighted/ph/maintained/$elloc -b $output/unweighted/ph/maintained/${i}.sg
	done
}
# Function runs block reordering on both weighted, unweighted and initial vertex maintained and unmaintained and also converts each to sg or wsg format
function br (){
	for i in "${graphs[@]}"; do
		elloc="${i}.el";
		welloc="${i}.wel";
		python3 br/br.py -p=br/br $input/$elloc $output/unweighted/br/$elloc; rmvr br;
		python3 br/br.py -m -p=br/br $input/$elloc $output/unweighted/br/maintained/$elloc; rmvr br;
		python3 br/br.py -m -w -p=br/br $input/$welloc $output/weighted/br/$welloc; rmvr br;
		./converter -f $output/unweighted/br/$elloc -b $output/unweighted/br/${i}.sg;
		./converter -f $output/unweighted/br/maintained/$elloc -b $output/unweighted/br/maintained/${i}.sg;
		./converter -f $output/weighted/br/$welloc -w -b $output/weighted/br/${i}.wsg;
	done
}
# Function runs rabbit reordering on both weighted, unweighted and initial vertex maintained and unmaintained and also converts each to sg or wsg format
function rabbit() {
	for i in "${graphs[@]}"; do
		elloc="${i}.el"
		welloc="${i}.wel"
		python3 rabbit/rabbit.py $input/$elloc $output/unweighted/rabbit/$elloc; rmvr;
		python3 rabbit/rabbit.py -m $input/$elloc $output/unweighted/rabbit/maintained/$elloc; rmvr;
		python3 rabbit/rabbit.py -m -w $input/$welloc $output/weighted/rabbit/$welloc; rmvr;
		./converter -f $output/unweighted/rabbit/$elloc -b $output/unweighted/rabbit/${i}.sg;
		./converter -f $output/unweighted/rabbit/maintained/$elloc -b $output/unweighted/rabbit/maintained/${i}.sg
		./converter -f $output/weighted/rabbit/$welloc -w -b $output/weighted/rabbit/${i}.wsg;
	done
}
# Function runs GOrder reordering on both weighted, unweighted and initial vertex maintained and unmaintained and also converts each to sg or wsg format
function gorder (){
	for i in "${graphs[@]}"; do
		elloc="${i}.el";
		gorder/Gorder $input/$elloc; rmvr;
		mv $input/${i}_Gorder.txt $output/unweighted/gorder/$elloc; rmvr;
		./converter -f $output/unweighted/gorder/$elloc -b $output/unweighted/gorder/${i}.sg;
	done
}
# Function runs clustering on both weighted, unweighted, sorted, clustered, and initial vertex maintained and unmaintained and also converts each to sg or wsg format
function cl () {
	for i in "${graphs[@]}"; do
		welloc="${i}.wel"
		elloc="${i}.el"
		python3 cluster/hub.py -m -w -s $input/$welloc $output/weighted/cl/sort/outdegree/$welloc; rmvr;
		python3 cluster/hub.py -m -w -i -s $input/$welloc $output/weighted/cl/sort/indegree/$welloc; rmvr;
		python3 cluster/hub.py -m -w $input/$welloc $output/weighted/cl/cluster/outdegree/$welloc; rmvr;
		python3 cluster/hub.py -m -w -i $input/$welloc $output/weighted/cl/cluster/indegree/$welloc; rmvr;
		python3 cluster/hub.py -s $input/${elloc} $output/unweighted/cl/sort/outdegree/$elloc; rmvr;
		python3 cluster/hub.py -i -s $input/${elloc} $output/unweighted/cl/sort/indegree/$elloc; rmvr;
		python3 cluster/hub.py $input/${elloc} $output/unweighted/cl/cluster/outdegree/$elloc; rmvr;
		python3 cluster/hub.py -i $input/${elloc} $output/unweighted/cl/cluster/indegree/$elloc; rmvr;

		python3 cluster/hub.py -m -s $input/$elloc $output/unweighted/cl/sort/outdegree/maintained/$elloc; rmvr;
		python3 cluster/hub.py -m -i -s $input/$elloc $output/unweighted/cl/sort/indegree/maintained/$elloc; rmvr;
		python3 cluster/hub.py -m $input/$elloc $output/unweighted/cl/cluster/outdegree/maintained/$elloc; rmvr;
		python3 cluster/hub.py -m -i $input/$elloc $output/unweighted/cl/cluster/indegree/maintained/$elloc; rmvr;
		./converter -f $output/weighted/cl/sort/outdegree/$welloc -w -b $output/weighted/cl/sort/outdegree/${i}.wsg
		./converter -f $output/weighted/cl/sort/indegree/$welloc -w -b $output/weighted/cl/sort/indegree/${i}.wsg
		./converter -f $output/unweighted/cl/sort/outdegree/$elloc -b $output/unweighted/cl/sort/outdegree/${i}.sg
		./converter -f $output/unweighted/cl/sort/indegree/$elloc  -b $output/unweighted/cl/sort/indegree/${i}.sg

		./converter -f $output/weighted/cl/cluster/outdegree/$welloc -w -b $output/weighted/cl/cluster/outdegree/${i}.wsg
		./converter -f $output/weighted/cl/cluster/indegree/$welloc -w -b $output/weighted/cl/cluster/indegree/${i}.wsg
		./converter -f $output/unweighted/cl/cluster/outdegree/$elloc -b $output/unweighted/cl/cluster/outdegree/${i}.sg
		./converter -f $output/unweighted/cl/cluster/indegree/$elloc -b $output/unweighted/cl/cluster/indegree/${i}.sg

		./converter -f $output/unweighted/cl/sort/outdegree/maintained/$elloc -b $output/unweighted/cl/sort/outdegree/maintained/${i}.sg
		./converter -f $output/unweighted/cl/sort/indegree/maintained/$elloc  -b $output/unweighted/cl/sort/indegree/maintained/${i}.sg
		./converter -f $output/unweighted/cl/cluster/outdegree/maintained/$elloc -b $output/unweighted/cl/cluster/outdegree/maintained/${i}.sg
		./converter -f $output/unweighted/cl/cluster/indegree/maintained/$elloc -b $output/unweighted/cl/cluster/indegree/maintained/${i}.sg
	done
}
# Function run degree based sorting on both weighted, unweighted, indegree, outdegree based, and initial vertex maintained and unmaintained and also converts each to sg or wsg format
function deg() {
	for i in "${graphs[@]}"; do
		welloc="${i}.wel"
		elloc="${i}.el"
		python3 degree/degree.py -m -w $input/$welloc $output/weighted/deg/outdegree/$welloc; rmvr;
		python3 degree/degree.py -m -w -i $input/$welloc $output/weighted/deg/indegree/$welloc; rmvr;
		python3 degree/degree.py  $input/${elloc} $output/unweighted/deg/outdegree/${elloc};rmvr;
		python3 degree/degree.py -i $input/${elloc} $output/unweighted/deg/indegree/${elloc};rmvr;
		python3 degree/degree.py -m  $input/${elloc} $output/unweighted/deg/outdegree/maintained/${elloc};rmvr;
		python3 degree/degree.py -m -i $input/${elloc} $output/unweighted/deg/indegree/maintained/${elloc};rmvr;
		./converter -f $output/weighted/deg/outdegree/$welloc -w -b $output/weighted/deg/outdegree/${i}.wsg;
		./converter -f $output/weighted/deg/indegree/$welloc -w -b $output/weighted/deg/indegree/${i}.wsg
		./converter -f $output/unweighted/deg/outdegree/$elloc -b $output/unweighted/deg/outdegree/${i}.sg
		./converter -f $output/unweighted/deg/indegree/$elloc -b $output/unweighted/deg/indegree/${i}.sg
		./converter -f $output/unweighted/deg/outdegree/maintained/$elloc -b $output/unweighted/deg/outdegree/maintained/${i}.sg
		./converter -f $output/unweighted/deg/indegree/maintained/$elloc -b $output/unweighted/deg/indegree/maintained/${i}.sg
	done
}
# Invocations
ph
br
rabbit
gorder
deg
cl