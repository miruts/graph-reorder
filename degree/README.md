## Running Degree based reordering ##

    usage: python3 degree.py [-h] [-m] [-w] [-a] [-i] input output

    Process input arguments

    positional arguments:
    input            input graph file path
    output           output graph file path

    optional arguments:
    -h, --help       show this help message and exit
    -m, --maintain   maintain initial vertex ID 0
    -w, --weighted   input graph is weighted
    -a, --ascending  order graph in ascending order
    -i, --indegree   use indegree for ordering

E.g. ```python3 degree.py --maintain --indegree ~/path/to/input.el ~/path/to/output.el```
- if ```--maintain``` is specified it prevents vertex ID 0 from reshuffling
- if ```--indegree``` is specified it uses indegree based sorting otherwise outdegree based sorting
- if ```--ascending``` is specified it sorts vertices in ascending order otherwise in descending order
- ```--weighted``` is required for weighted input graphs
