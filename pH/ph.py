import sys
import subprocess
import argparse
parser = argparse.ArgumentParser(description="Process input arguments")
parser.add_argument('input', help='input graph file path')
parser.add_argument('output', help='output graph file path')
parser.add_argument('-p', '--program', help='ph reordering program path', default='ph/ph')
parser.add_argument('-m', '--maintain', help='maintain initial al vertex', action='store_true')
parser.add_argument('-w', '--weighted', help='input is weighted graph', action='store_true')
args = parser.parse_args()
program = args.program
el = [] # stores graph edgelist
output_wel = []
vertices = set() # stores unique vertices
with open(args.input, "r") as fin: # read input graph
	line = fin.readline()
	while line:
		splited = line.split()
		src, dst = int(splited[0]), int(splited[1])
		w = (splited[2]) if args.weighted else None
		el.append([src, dst, w] if args.weighted else [src, dst])
		line = fin.readline()
mapping = {} # new mapping of a vertex
if args.weighted: # if initial vertex 0 needs to be maintained
	count = 1
	with open("ph/input.el", "w") as fout: # write unweighted edgelist to temp file
		for s, d, _ in el:
			fout.write("{} {}\n".format(s, d))
	subprocess.run([program, "ph/input.el", "ph/output.el"]) # reorder unweighted graph using ph
	with open("ph/new_order.el", "r") as fin, open(args.output, "w") as fout: # read vertex mapping and write to output
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
		# print("Number of vertices read after reordering: ", node)
		if args.maintain: # maintain vertex 0
			mapping[mapped_to_zero]=zeros_map
			mapping[0]=0
		for s, d, w in el: # write reordered graph to output file
			fout.write("{} {} {}\n".format(mapping.get(s, s), mapping.get(d, d), w))

else: # unweighted graph
	# if no need to maintain directly write to reordered graph output path else write to temp file
	subprocess.run([args.program, args.input, "ph/output.el" if args.maintain else args.output])

	if args.maintain: # read vertex mapping, maintain 0 and write reordered graph to output path
		with open("ph/new_order.el", "r") as fin, open(args.output, "w") as fout:
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
			mapping[0] = 0
			mapping[mapped_to_zero]=zeros_map
			for s,d in el: # write reordered graph to output path
				fout.write("{} {}\n".format(mapping.get(s, s), mapping.get(d, d)))
