import sys
import subprocess
import argparse
parser = argparse.ArgumentParser(description="Process input arguments")
parser.add_argument('input', help='input graph file path')
parser.add_argument('output', help='output graph file path')
parser.add_argument('-p', '--program', help='block reordering program path', default='br/br')
parser.add_argument('-m', '--maintain', help='maintain initial al vertex', action='store_true')
parser.add_argument('-w', '--weighted', help='input is weighted graph', action='store_true')
args = parser.parse_args()
program = args.program
el = [] # Edgelist
with open(args.input, "r") as fin: # Read input graph
	line = fin.readline()
	while line:
		splited = line.split()
		src, dst = int(splited[0]), int(splited[1])
		w = (splited[2]) if args.weighted else None
		el.append([src, dst, w] if args.weighted else [src, dst])
		line = fin.readline()

mapping = {} # New mapping of a Vertex
if args.weighted: # If graph is weighted
	with open("br/input.el", "w") as fout:
		for s, d, _ in el:
			fout.write("{} {}\n".format(s, d))
	subprocess.run([program,  "br/input.el", "br/output.el"]) # Reorder input graph
	with open("br/new_order.el", "r") as fin, open(args.output, "w") as fout: # Get vertex mapping and write output
		in_line = fin.readline()
		node = 0
		zeros_map=None
		mapped_to_zero=None
		while in_line:
			nodeMap = int(in_line)
			if zeros_map is None:
				zeros_map=nodeMap
			if nodeMap==0:
				mapped_to_zero=node
			mapping[node] = nodeMap
			node += 1
			in_line = fin.readline()
		# print("Number of unique after reordering: ", node)
		if args.maintain: # Make 0's mapping 0
			mapping[mapped_to_zero]=zeros_map
			mapping[0]=0

		for s, d, w in el: # Write reordered graph to output path
			fout.write("{} {} {}\n".format(mapping.get(s, s), mapping.get(d, d), w))


else: # If graph is unweighted
	subprocess.run([args.program, args.input, "br/output.el" if args.maintain else args.output]) # if maintain write to temp file else to graph output path
#	subprocess.run([args.program, "64", "5324800", args.input, "output.el" if])
	if args.maintain: # If need to maintain initial Vertex 0 from reshuffling
		with open("br/new_order.el", "r") as fin, open(args.output, "w") as fout: # Get vertex mapping and write output
			in_line = fin.readline()
			node = 0
			zeros_map=None
			mapped_to_zero=None
			while in_line:
				nodeMap = int(in_line)
				if zeros_map is None:
					zeros_map = nodeMap
				if nodeMap == 0:
					mapped_to_zero = node
				mapping[node]=nodeMap
				node += 1
				in_line = fin.readline()
			print("Number of unique after reordering: ", node)
			mapping[0] = 0
			mapping[mapped_to_zero]=zeros_map
			for s,d in el: # Write reordered graph to output
				fout.write("{} {}\n".format(mapping.get(s, s), mapping.get(d, d)))
