import sys
import argparse
from collections import defaultdict

parser = argparse.ArgumentParser(description='Process input arguments for clustering')
parser.add_argument('input', help='input graph file path')
parser.add_argument('output', help='output graph file path')
parser.add_argument('-a', '--ascending', help='order graph in ascending order', action='store_true')
parser.add_argument('-m', '--maintain', help='maintain initial vertex ID 0', action='store_true')
parser.add_argument('-w', '--weighted', help='input graph is weighted', action='store_true')
parser.add_argument('-i', '--indegree', help='use indegree for ordering', action='store_true')
parser.add_argument('-s', '--sort', help='perform hub sorting else clustering', action='store_true')
args = parser.parse_args()


edge_weights = {} # holds an edge weight
edges_by_node = defaultdict(list) # stores either in-neighbour or outneighour depending on the arg (indegree, outdegree) to use
edge_list = [] # read input graph edges
vertices = set() # vertex set

with open(args.input, 'r') as fin: # read input graph
    line = fin.readline()
    if args.weighted:
        while line:
            src, dst, w = line.split()
            src, dst, w = int(src), int(dst), int(w)
            edge_list.append([src, dst])
            if args.indegree:
                edges_by_node[dst].append(src)
            else:
                edges_by_node[src].append(dst)
            edge_weights[(src, dst)] = w
            vertices.add(src)
            vertices.add(dst)
            line = fin.readline()
    else:
        while line:
            src, dst = line.split()
            src, dst = int(src), int(dst)
            edge_list.append([src, dst])
            if args.indegree:
                edges_by_node[dst].append(src)
            else:
                edges_by_node[src].append(dst)
            vertices.add(src)
            vertices.add(dst)
            line = fin.readline()
n = len(vertices)
edge_count = len(edge_list)
avg = edge_count / n
hub_nodes = set()
for src, dst in edge_list: # identify hub nodes
    edges_src = edges_by_node.get(src)
    edges_dst = edges_by_node.get(dst)
    if edges_src and len(edges_src) >= avg:
        hub_nodes.add(src)
    if edges_dst and len(edges_dst) >= avg:
        hub_nodes.add(dst)

non_hubs = vertices - hub_nodes
count = 1 if args.maintain else 0
mapping = {} # mapping for hub nodes
if args.sort: # if nodes are to be sorted based on their degrees
    hub_nodes_sorted = sorted(hub_nodes, key=lambda x: len(edges_by_node[x]), reverse=False if args.ascending else True)
    for n in hub_nodes_sorted:
        if args.maintain and n == 0:
            mapping[n] = n
        else:
            mapping[n] = count
            count += 1
else: # if nodes need not be to sorted
    for n in hub_nodes:
        if args.maintain and n == 0:
            mapping[n] = n
        else:
            mapping[n] = count
            count += 1
for n in non_hubs: # mapping for non-hub nodes
    if args.maintain and n == 0:
        mapping[n] = n
    else:
        mapping[n] = count
        count += 1
# either hub sorted or hub clustered edge list depending on (sorted, clustered) argument
hub_ordered = [[mapping[s], mapping[d], edge_weights.get((s, d))] if args.weighted else [mapping[s], mapping[d]] for
               s, d
               in edge_list]
print("Done hub ordering")
# write new order
with open('cluster/new_order.el', 'w') as fout:
	for i in range(len(mapping)):
		fout.write('{}\n'.format(mapping[i]))
# write ordered graph into output file
with open(args.output, 'w') as fout:
    if args.weighted:
        for src, dst, w in hub_ordered:
            fout.write('{} {} {}\n'.format(src, dst, w))
    else:
        for src, dst in hub_ordered:
            fout.write('{} {}\n'.format(src, dst))
print("Writing done")
