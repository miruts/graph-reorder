import sys
import subprocess
import argparse
parser = argparse.ArgumentParser(description="Process input arguments")
parser.add_argument('input', help='input grpah file path')
parser.add_argument('output',help='output graph file path')
parser.add_argument('-p','--program', default='rabbit/reorder', help='path to rabbit executable')
parser.add_argument('-m', '--maintain', help='maintain inital vertex', action='store_true')
parser.add_argument('-w', '--weighted', help='input is weighted graph', action='store_true')
args = parser.parse_args()
program = 'rabbit/rabbit'
el = [] # stores input graph edgelist
vertices = set()
with open(args.input, "r") as fin: # read input graph
	line = fin.readline()
	while line:
		splited = line.split()
		src, dst = int(splited[0]), int(splited[1])
		w = int(splited[2]) if args.weighted else None
		el.append([src, dst, w] if args.weighted else [src, dst])
		vertices.add(src)
		vertices.add(dst)
		line = fin.readline()
mapping = {} # stores mapping of a vertex
if args.weighted: # if input is weighted graph
	with open('input.el', 'w') as fout: # write unweighted graph to temp file
		for s,d,_ in el:
			fout.write('{} {}\n'.format(s,d))
	with open('rabbit_output.txt', 'w') as fout: # run rabbit reorderer and write new mapping to temp file
		subprocess.run([program, './input.el'], stdout=fout)
	with open('rabbit_output.txt', 'r') as fin, open(args.output, 'w') as fout: # read vertex mapping and write to output file
		in_line = fin.readline()
		node = 0
		zeros_map =None
		mapped_to_zero = None
		while in_line:
			nodeMap = int(in_line)
			if  zeros_map is None:
				zeros_map = nodeMap
			if nodeMap==0:
				mapped_to_zero=node
			mapping[node]=nodeMap
			node += 1
			in_line = fin.readline()
		if args.maintain: # maintain 0
			mapping[mapped_to_zero] = zeros_map
			mapping[0]=0
		for s,d,w in el: # write reordered graph to output file
			fout.write('{} {} {}\n'.format(mapping.get(s,s), mapping.get(d,d), w))
else: # if input graph is unweighted
	with open('rabbit_output.txt', 'w') as fout: # run rabbit reorderer and store new order to temp file
		subprocess.run([program, args.input], stdout=fout)
	with open('rabbit_output.txt', 'r') as fin, open(args.output, 'w') as fout: # read new vertex mapping and write to output file
		in_line = fin.readline()
		node = 0
		zeros_map = None
		mapped_to_zero = None
		while in_line:
			nodeMap = int(in_line)
			if zeros_map is None:
				zeros_map = nodeMap
			if nodeMap == 0:
				mapped_to_zero = node
			mapping[node]=nodeMap
			node += 1
			in_line = fin.readline()
		if args.maintain: # if need to maintain vertex 0
			mapping[mapped_to_zero]=zeros_map
			mapping[0]=0
		for s,d in el: # write reordered graph to output path
			fout.write("{} {}\n".format(mapping.get(s,s), mapping.get(d,d)))
