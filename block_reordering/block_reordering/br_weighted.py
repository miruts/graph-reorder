import sys
import subprocess
if len(sys.argv) < 4:
	quit()
input_path, output_path = sys.argv[1], sys.argv[2]
program = sys.argv[3]
el = []
output_wel = []
vertices = set()
with open(input_path, "r") as fin:
	line = fin.readline()
	while line:
		src, dst, w = line.split()
		src, dst, w = int(src), int(dst), int(w)
		el.append([src, dst, w])
		vertices.add(src)
		vertices.add(dst)
		line = fin.readline()

with open("input.el", "w") as fout:
	for s, d, _ in el:
		fout.write("{} {}\n".format(s, d))
subprocess.run([program, "input.el", "output.el"])
mapping = {}
with open("new_order.el", "r") as fin, open(output_path, "w") as fout:
	in_line = fin.readline()
	node = 0
	while in_line:
		nodeMap = int(in_line)
		mapping[node] = nodeMap
		node += 1
		in_line = fin.readline()
	for s, d, w in el:
		fout.write("{} {} {}\n".format(mapping.get(s, s), mapping.get(d, d), w))


