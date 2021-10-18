# Running Clustering #
**Hub Sorting** - Relabels the hub vertices (defined as vertices with degree greater than average degree) in descending order of degrees, while retaining the vertex ID assignment for most non-hub vertices

**Hub Clustering** - Hub vertices are assigned a contiguous range of IDs, but doesn’t guarantee that the vertex IDs are assigned in descending order of degree

    usage: python3 hub.py [-h] [-a] [-m] [-w] [-i] [-s] input output

    Process input arguments for clustering

    positional arguments:
    input            input graph file path
    output           output graph file path

    optional arguments:
    -h, --help       show this help message and exit
    -a, --ascending  order graph in ascending order
    -m, --maintain   maintain initial vertex ID 0
    -w, --weighted   input graph is weighted
    -i, --indegree   use indegree for ordering
    -s, --sort       perform hub sorting else clustering
E.g. ```python3 hub.py --maintain --weighted ~/path/to/input.wel ~/path/to/output.wel```
- if ```--maintain``` is specified it prevents vertex ID 0 from reshuffling
- if ```--ascending``` is specified it sorts hub vertices in ascending order otherwise descending order
- if ```--indegree``` is specified it considers indegree of vertices otherwise outdegree
- if ```--sort``` is specified it performs hub sorting otherwise hub clustering
- ```--weighted``` is required for weighted graphs