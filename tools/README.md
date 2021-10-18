## Using gaps converter tool ##
The Gaps converter tool is used to convert graphs to different formats, and it could also be used to generate random edge weights.

## Using shuffle_el.py script ##
The shuffle_el script is used to shuffle Vertex IDs in random fashion.

    # Add weights to produce a weighted edgelist
    ./converter -f CA-AstroPh.el -w -e CA-AstroPh.wel
    
    # Convert weighted edgelist to binary format
    ./converter -f CA-AstroPh.wel -w -b CA-AstroPh.wsg

    # From unweighted edgelist to weighted binary
    ./converter -f CA-AstroPh.el -w -b CA-AstroPh.wsg
## Using sorter.py ##
this sorter.py script is used to sort a graph nodes in descending order of outdegrees, and indegrees in their respective columns(useful for testing and debugging)
column 1                           column 2
```Vertex ID``` ```outdegree```    ```Vertex ID``` ```indegree```

usage: ```python3 sorter.py input.el sorted_output.el```

## Using filter.py script ##
This filter.py script is used to pack vertex ID so that each number between 0 and length of nodes is a valid vertex ID. i.e converts node IDs [0,1,4,5] -> [0,1,2,3] 
usage: ```python3 filter.py input.el filtered.el```