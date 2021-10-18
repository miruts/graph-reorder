import sys
if len(sys.argv) < 3:
    quit()
"""
This script filters out vacant nodes. Length of a graph is usually assumed to be the value of the largest vertex ID,
but in some cases we might have unassigned vertex IDs between 0 and the maximum vertex ID. The script condenses the vertex ID to the real 
length of the nodes.
"""
input_path, output_path = sys.argv[1], sys.argv[2]
edge_list = []
vertices = set()
with open(input_path, 'r') as fin:
    line = fin.readline()
    while line:
        if line.startswith("#"):
            line = fin.readline()
            continue
        src, dst = line.split()
        src, dst = int(src), int(dst)
        edge_list.append([src, dst])
        vertices.add(src)
        vertices.add(dst)
        line = fin.readline()
print("Unique Vertices: ", len(vertices))
filtered = sorted(vertices)
mapping = {}
for i,v in enumerate(filtered):
	mapping[v] = i

filtered_edges = [[mapping[s], mapping[d]] for s, d in edge_list]
with open(output_path, 'w') as fout:
    for s, d in filtered_edges:
        fout.write("{} {}\n".format(s, d))
print("Length written: ", len(filtered_edges))
print("Filtering done")
