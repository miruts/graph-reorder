'''
This script sorts an edgelist graph in descending order of outdegrees on the first column and descending order of indegrees in the second column
E.g
Vertex_ID 	 Vertex_Outdegree		Vertex_ID   Vertex_Indegree
0		 	 20						5 			16

'''
import sys
from collections import defaultdict
if len(sys.argv) < 3:
	quit()
input_path, output_path = sys.argv[1], sys.argv[2] # Input & output file paths
edges_by_src = defaultdict(list) # Out-neighbours of a vertex
edges_by_dst = defaultdict(list) # In-neighbours of a vertex
vertices = set() # Unique vertices
with open(input_path, "r") as fin:
	line = fin.readline()
	while line:
		splited = line.split()
		src, dst = int(splited[0]), int(splited[1])
		src, dst = int(src), int(dst)
		vertices.add(src)
		vertices.add(dst)
		edges_by_src[src].append(dst)
		edges_by_dst[dst].append(src)
		line = fin.readline()
print("Number of vertices read: ", len(vertices))
# Sort by outdegree - descending
src_sorted = sorted(vertices, key=lambda x: len(edges_by_src[x]), reverse=True)
# Sort by indegree - descending
dst_sorted = sorted(vertices, key=lambda x: len(edges_by_dst[x]), reverse=True)
# Write output
with open(output_path, "w") as fout:
	for o, i in zip(src_sorted, dst_sorted):
		fout.write("{} {}\t{} {}\n".format(o,len(edges_by_src[o]), i, len(edges_by_dst[i])))
print("Writing Done")
