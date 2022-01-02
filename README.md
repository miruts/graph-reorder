# Graph Reordering #
Graph Reordering improves the locality of Graph processing. It systematically reassigns Vertex ID of the nodes in the graph to increase the cache hit rate based on different heuristics and graph structure properties, like power-law distribution property, degrees, clusters, and symmetricity of real-world graphs. 
Provided below is how to use and run different reordering methods, test reordering methods, and how to run simulations on those reordered graphs.
## These Reordering methods include: ##
- Degree reordering
- Clustering
- [GOrder](https://github.com/datourat/Gorder)
- [pH](https://github.com/kartiklakhotia/graphReordering)
- [Block Reordering](https://github.com/kartiklakhotia/graphReordering)
- [Rabbit reordering](https://github.com/araij/rabbit_order)

## Reordering
Each Folder contains how to run or use reordering method, script, and tools

PS. reordering methods, or scripts should not be run in parallel because they use the same temp file for logging runtimes and could cause trouble.


