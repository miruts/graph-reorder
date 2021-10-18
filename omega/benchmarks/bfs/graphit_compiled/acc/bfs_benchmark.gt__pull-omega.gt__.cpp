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
int  * __restrict parent;
template <typename TO_FUNC , typename APPLY_FUNC> VertexSubset<NodeID>* edgeset_apply_pull_parallel_from_vertexset_to_filter_func_with_frontier(Graph & g , VertexSubset<NodeID>* from_vertexset, TO_FUNC to_func, APPLY_FUNC apply_func) 
{ 
    int64_t numVertices = g.num_nodes(), numEdges = g.num_edges();
  VertexSubset<NodeID> *next_frontier = new VertexSubset<NodeID>(g.num_nodes(), 0);
  bool * next = newA(bool, g.num_nodes());
  ligra::parallel_for_lambda((int)0, (int)numVertices, [&] (int i) { next[i] = 0; });
  from_vertexset->toDense();
  ligra::parallel_for_lambda((NodeID)0, (NodeID)g.num_nodes(), [&] (NodeID d) {
    if (to_func(d)){ 
      for(NodeID s : g.in_neigh(d)){
        if (from_vertexset->bool_map_[s] ) { 
          if( apply_func ( s , d ) ) { 
            next[d] = 1; 
            if (!to_func(d)) break; 
          }
        }
      } //end of loop on in neighbors
    } //end of to filtering 
  },GRAIN); //end of outer for loop
  next_frontier->num_vertices_ = sequence::sum(next, numVertices);
  next_frontier->bool_map_ = next;
  next_frontier->is_dense = true;
  return next_frontier;
} //end of edgeset apply function 
struct parent_generated_vector_op_apply_func_0
{
void operator() (NodeID v) 
  {
    parent[v] =  -(1) ;
  };
};
struct updateEdge
{
bool operator() (NodeID src, NodeID dst) 
  {
    bool output1 ;
    ((volatile int*)parent)[dst] = src;
    output1 = (bool) 1;
    return output1;
  };
};
struct toFilter
{
bool operator() (NodeID v) 
  {
    bool output ;
    output = (((volatile int*)parent)[v]) == ( -(1) );
    return output;
  };
};
struct reset
{
void operator() (NodeID v) 
  {
    ((volatile int*)parent)[v] =  -(1) ;
  };
};
int main(int argc, char * argv[])
{
  // Should match the core count in gem5
  setWorkers(16);
  setWorkers(16);
  setWorkers(16);

  // Initialize OMEGA control registers and mapped arrays
  omega.init();

  edges = builtin_loadEdgesFromFile ( argv_safe((1) , argv, argc)) ;
  parent = new int [ builtin_getVertices(edges) ];
  ligra::parallel_for_lambda((int)0, (int)builtin_getVertices(edges) , [&] (int vertexsetapply_iter) {
    parent_generated_vector_op_apply_func_0()(vertexsetapply_iter);
  });;
  for ( int trail = (0) ; trail < (1) ; trail++ )
  {
    startTimer() ;
    omegaMap(parent, builtin_getVertices(edges));
    ligra::parallel_for_lambda((int)0, (int)builtin_getVertices(edges) , [&] (int vertexsetapply_iter) {
      reset()(vertexsetapply_iter);
    },GRAIN);;
    VertexSubset<int> *  frontier = new VertexSubset<int> ( builtin_getVertices(edges)  , (0) );
    builtin_addVertex(frontier, (0) ) ;
    ((volatile int*)parent)[(0) ] = (0) ;
    while ( (builtin_getVertexSetSize(frontier) ) != ((0) ))
    {
      frontier = edgeset_apply_pull_parallel_from_vertexset_to_filter_func_with_frontier(edges, frontier, toFilter(), updateEdge()); 
    }
    omegaUnmap(parent);
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

