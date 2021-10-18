# Running Gorder Reordering #
**Gorder** - Sophisticated graph reordering algorithm that relabels vertices to maximize the overlap between neighbors of vertices with consecutive IDs
.

    usage: ./Gorder ~/path/to/input.el [-w [w]]
    
    positional arguments:
        input     input graph file
    optional arguments:
        w     window size in nodes, defaults to 5
E.g. ```./Gorder ~/path/to/web-NotreDame.el -w 8```

Output: ```filename```_Gorder.txt - reordered edgelist graph in the same directory as input graph file

PS. Gorder only accepts unweighted edgelist graphs & no duplicate edges are allowed