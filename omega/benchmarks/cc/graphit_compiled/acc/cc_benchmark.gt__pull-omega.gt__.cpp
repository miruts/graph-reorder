#include <iostream> 
#include <vector>
#include <cstdio>
#include <algorithm>
#include "intrinsics.h"
#ifdef GEN_PYBIND_WRAPPERS
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
namespace py = pybind11;
#endif
#include "../../../OMEGA_API.h"

#define GRAIN (omegaGrainSize())

OMEGA_API omega;

Graph edges;
int  * __restrict IDs;
template <typename APPLY_FUNC > VertexSubset<NodeID>* edgeset_apply_pull_parallel_deduplicatied_from_vertexset_with_frontier(Graph & g , VertexSubset<NodeID>* from_vertexset, APPLY_FUNC apply_func) 
{ 
    int64_t numVertices = g.num_nodes(), numEdges = g.num_edges();
  VertexSubset<NodeID> *next_frontier = new VertexSubset<NodeID>(g.num_nodes(), 0);
  bool * next = newA(bool, g.num_nodes());
  ligra::parallel_for_lambda((int)0, (int)numVertices, [&] (int i) { next[i] = 0; });
  from_vertexset->toDense();
  ligra::parallel_for_lambda((NodeID)0, (NodeID)g.num_nodes(), [&] (NodeID d) {
    for(NodeID s : g.in_neigh(d)){
      if (from_vertexset->bool_map_[s] ) { 
        if( apply_func ( s , d ) ) { 
          next[d] = 1; 
        }
      }
    } //end of loop on in neighbors
  },GRAIN); //end of outer for loop
  next_frontier->num_vertices_ = sequence::sum(next, numVertices);
  next_frontier->bool_map_ = next;
  next_frontier->is_dense = true;
  return next_frontier;
} //end of edgeset apply function 
struct IDs_generated_vector_op_apply_func_0
{
void operator() (NodeID v) 
  {
    IDs[v] = (1) ;
  };
};
struct updateEdge
{
bool operator() (NodeID src, NodeID dst) 
  {
    bool output2 ;
    bool IDs_trackving_var_1 = (bool) 0;
    if ( ( ((volatile int*)IDs)[dst]) > ( ((volatile int*)IDs)[src]) ) { 
      ((volatile int*)IDs)[dst]= ((volatile int*)IDs)[src]; 
      IDs_trackving_var_1 = true ; 
    } 
    output2 = IDs_trackving_var_1;
    return output2;
  };
};
struct init
{
void operator() (NodeID v) 
  {
    ((volatile int*)IDs)[v] = v;
  };
};
int main(int argc, char * argv[])
{
  // Should match the core count in gem5
  setWorkers(16);
  setWorkers(16);

  // Initialize OMEGA control registers and mapped arrays
  omegaInit();

  edges = builtin_loadEdgesFromFile ( argv_safe((1) , argv, argc)) ;
  IDs = new int [ builtin_getVertices(edges) ];
  ligra::parallel_for_lambda((int)0, (int)builtin_getVertices(edges) , [&] (int vertexsetapply_iter) {
    IDs_generated_vector_op_apply_func_0()(vertexsetapply_iter);
  });;
  int n = builtin_getVertices(edges) ;
  for ( int trail = (0) ; trail < (1) ; trail++ )
  {
    startTimer() ;
    VertexSubset<int> *  frontier = new VertexSubset<int> ( builtin_getVertices(edges)  , n);
    ligra::parallel_for_lambda((int)0, (int)builtin_getVertices(edges) , [&] (int vertexsetapply_iter) {
      init()(vertexsetapply_iter);
    },GRAIN);;
    while ( (builtin_getVertexSetSize(frontier) ) != ((0) ))
    {
      omegaMap(IDs, builtin_getVertices(edges));
      frontier = edgeset_apply_pull_parallel_deduplicatied_from_vertexset_with_frontier(edges, frontier, updateEdge()); 
    }
    omegaUnmap(IDs);
    float elapsed_time = stopTimer() ;
    std::cout << "elapsed time: "<< std::endl;
    std::cout << elapsed_time<< std::endl;
	FILE * fp;
    fp = fopen("log.txt", "w");
    if (fp == NULL ) std::cout << "Error opening file\n";
    else fprintf(fp, "%f", elapsed_time);
    if (fp != NULL) fclose(fp);
  }
};
#ifdef GEN_PYBIND_WRAPPERS
PYBIND11_MODULE(, m) {
}
#endif

