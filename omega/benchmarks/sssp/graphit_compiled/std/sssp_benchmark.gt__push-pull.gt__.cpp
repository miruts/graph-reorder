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
WGraph edges;
int  * __restrict SP;
template <typename APPLY_FUNC , typename PUSH_APPLY_FUNC> VertexSubset<NodeID>* edgeset_apply_hybrid_dense_parallel_weighted_deduplicatied_from_vertexset_with_frontier(WGraph & g , VertexSubset<NodeID>* from_vertexset, APPLY_FUNC apply_func, PUSH_APPLY_FUNC push_apply_func) 
{ 
    int64_t numVertices = g.num_nodes(), numEdges = g.num_edges();
    from_vertexset->toSparse();
    long m = from_vertexset->size();
    // used to generate nonzero indices to get degrees
    uintT *degrees = newA(uintT, m);
    // We probably need this when we get something that doesn't have a dense set, not sure
    // We can also write our own, the eixsting one doesn't quite work for bitvectors
    //from_vertexset->toSparse();
    {
        ligra::parallel_for_lambda((long)0, (long)m, [&] (long i) {
            NodeID v = from_vertexset->dense_vertex_set_[i];
            degrees[i] = g.out_degree(v);
         });
    }
    uintT outDegrees = sequence::plusReduce(degrees, m);
    if (m + outDegrees > numEdges / 20) {
  VertexSubset<NodeID> *next_frontier = new VertexSubset<NodeID>(g.num_nodes(), 0);
  bool * next = newA(bool, g.num_nodes());
  ligra::parallel_for_lambda((int)0, (int)numVertices, [&] (int i) { next[i] = 0; });
    from_vertexset->toDense();
    ligra::parallel_for_lambda((NodeID)0, (NodeID)g.num_nodes(), [&] (NodeID d) {
      for(WNode s : g.in_neigh(d)){
        if (from_vertexset->bool_map_[s.v] ) { 
          if( apply_func ( s.v , d, s.w ) ) { 
            next[d] = 1; 
          }
        }
      } //end of loop on in neighbors
    },16); //end of outer for loop
  next_frontier->num_vertices_ = sequence::sum(next, numVertices);
  next_frontier->bool_map_ = next;
  next_frontier->is_dense = true;
  return next_frontier;
} else {
    if (g.get_flags_() == nullptr){
      g.set_flags_(new int[numVertices]());
      ligra::parallel_for_lambda(0, (int)numVertices, [&] (int i) { g.get_flags_()[i]=0; });
    }
    VertexSubset<NodeID> *next_frontier = new VertexSubset<NodeID>(g.num_nodes(), 0);
    if (numVertices != from_vertexset->getVerticesRange()) {
        cout << "edgeMap: Sizes Don't match" << endl;
        abort();
    }
    if (outDegrees == 0) return next_frontier;
    uintT *offsets = degrees;
    long outEdgeCount = sequence::plusScan(offsets, degrees, m);
    uintE *outEdges = newA(uintE, outEdgeCount);
      ligra::parallel_for_lambda((long)0, (long)m, [&] (long i) {
    NodeID s = from_vertexset->dense_vertex_set_[i];
    int j = 0;
    uintT offset = offsets[i];
        for(WNode d : g.out_neigh(s)){
          if( push_apply_func ( s , d.v, d.w ) && CAS(&(g.get_flags_()[d.v]), 0, 1)  ) { 
            outEdges[offset + j] = d.v; 
          } else { outEdges[offset + j] = UINT_E_MAX; }
          j++;
        } //end of for loop on neighbors
      },16);
  uintE *nextIndices = newA(uintE, outEdgeCount);
  long nextM = sequence::filter(outEdges, nextIndices, outEdgeCount, nonMaxF());
  free(outEdges);
  free(degrees);
  next_frontier->num_vertices_ = nextM;
  next_frontier->dense_vertex_set_ = nextIndices;
  ligra::parallel_for_lambda((int)0, (int)nextM, [&] (int i) {
     g.get_flags_()[nextIndices[i]] = 0;
  });
  return next_frontier;
} //end of else
} //end of edgeset apply function 
struct updateEdge_push_ver
{
bool operator() (NodeID src, NodeID dst, int weight) 
  {
    bool output4 ;
    bool SP_trackving_var_3 = (bool) 0;
    SP_trackving_var_3 = writeMin( &SP[dst], (SP[src] + weight) ); 
    output4 = SP_trackving_var_3;
    return output4;
  };
};
struct SP_generated_vector_op_apply_func_0
{
void operator() (NodeID v) 
  {
    SP[v] = (2147483647) ;
  };
};
struct updateEdge
{
bool operator() (NodeID src, NodeID dst, int weight) 
  {
    bool output2 ;
    bool SP_trackving_var_1 = (bool) 0;
    if ( ( SP[dst]) > ( (SP[src] + weight)) ) { 
      SP[dst]= (SP[src] + weight); 
      SP_trackving_var_1 = true ; 
    } 
    output2 = SP_trackving_var_1;
    return output2;
  };
};
struct reset
{
void operator() (NodeID v) 
  {
    SP[v] = (2147483647) ;
  };
};
int main(int argc, char * argv[])
{
  // Should match the core count in gem5
  setWorkers(16);

  edges = builtin_loadWeightedEdgesFromFile ( argv_safe((1) , argv, argc)) ;
  SP = new int [ builtin_getVertices(edges) ];
  ligra::parallel_for_lambda((int)0, (int)builtin_getVertices(edges) , [&] (int vertexsetapply_iter) {
    SP_generated_vector_op_apply_func_0()(vertexsetapply_iter);
  });;
  for ( int trail = (0) ; trail < (1) ; trail++ )
  {
    startTimer() ;
    ligra::parallel_for_lambda((int)0, (int)builtin_getVertices(edges) , [&] (int vertexsetapply_iter) {
      reset()(vertexsetapply_iter);
    });;
    int n = builtin_getVertices(edges) ;
    VertexSubset<int> *  frontier = new VertexSubset<int> ( builtin_getVertices(edges)  , (0) );
    builtin_addVertex(frontier, (0) ) ;
    SP[(0) ] = (0) ;
    int rounds = (0) ;
    while ( (builtin_getVertexSetSize(frontier) ) != ((0) ))
    {
      frontier = edgeset_apply_hybrid_dense_parallel_weighted_deduplicatied_from_vertexset_with_frontier(edges, frontier, updateEdge(), updateEdge_push_ver()); 
      rounds = (rounds + (1) );
      if ((rounds) == (n))
       { 
        std::cout << "negative cycle"<< std::endl;
        break;
       } 
    }
    float elapsed_time = stopTimer() ;
    std::cout << "elapsed time: "<< std::endl;
    std::cout << elapsed_time<< std::endl;
    std::cout << "rounds"<< std::endl;
    std::cout << rounds<< std::endl;
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

