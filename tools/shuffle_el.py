import sys
import random

if len(sys.argv) != 3:
  print("Usage: python3 {} 'input edgelist path' 'output edgelist path'".format(sys.argv[0]))
  quit()
  
input_path = sys.argv[1]
output_path = sys.argv[2]

# Dictionary storing graph edges
# - Keys are source vertices
# - Values are lists of destination vertices (out-neighbors)
edges_by_src = {}

# Set used to collect vertex IDs (both source and destination) with no repeats
unique_verts = set()

# Import the edgelist input file
print("Reading input...")
with open(input_path, "r") as fin:
  # Read an input line
  line = fin.readline()
  
  # Loop until end of file reached
  while line:
    src, dst = line.split()
    src, dst = int(src), int(dst)
    
    # If this source vertex hasn't been encountered yet,
    if src not in edges_by_src:
      # Start an out-neighbor list for it
      edges_by_src[src] = []
    
    # Add this destination to the source's list of out-neighbors
    edges_by_src[src].append(dst)
  
    # Collect both imported vertices
    unique_verts.add(src)
    unique_verts.add(dst)
  
    # Read another input line
    line = fin.readline()
print("Done")

# Get the vertex IDs as a list, and make a copy
original_IDs = list(unique_verts)
shuffled_IDs = list(original_IDs)

# Shuffle the list copy
random.shuffle(shuffled_IDs)

# Create a dictionary that converts orignal IDs to shuffled IDs
# - Keys are original IDs
# - Values are shuffled (new) IDs
original_to_shuffled = dict(zip(original_IDs, shuffled_IDs))

# Output the original edges using the new, shuffled IDs
print("Writing output...")
with open(output_path, "w") as fout:
  for src, dst_list in edges_by_src.items():
    for dst in dst_list:
      fout.write("{} {}\n".format( original_to_shuffled[src], original_to_shuffled[dst] ))
print("Done")
