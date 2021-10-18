#ifndef _OMEGA_API_H_
#define _OMEGA_API_H_

#include "intrinsics.h"

//#define VERTEXLINE_SIZE 8 // Should match gem5's vertexline_size
#define CACHELINE_SIZE 64 // Should match gem5's cacheline_size
#define VERTEXLINE_SIZE 8

#ifndef VERTEXLINE_SIZE
  #error Must define VERTEXLINE_SIZE
#endif

// Options for enableAtomic()
#define OMEGA_ATOMIC_SUM_INT    'd'
#define OMEGA_ATOMIC_SUM_FLOAT  'e'
#define OMEGA_ATOMIC_SUM_DOUBLE 'f'
#define OMEGA_ATOMIC_MIN_INT    'g'
#define OMEGA_ATOMIC_MIN_FLOAT  'h'
#define OMEGA_ATOMIC_MIN_DOUBLE 'i'
#define OMEGA_ATOMIC_CAS_32     'j'

// Only one instance of this class should exist at any given time
class OMEGA_API {
public:
  //OMEGA_API();
  ~OMEGA_API();

  // Must be called before using the API
  // Could be done in the constructor, but setWorkers() must be called first
  // to get parallelized initialization of 'mapped_next'. This will often be
  // global, so the constructor would likely be called before setWorkers().
  void init();

  template <typename T> void map(T* new_data, long new_data_count);

  template <typename T> void unmap(T* data_dst);

  // Could be useful to unmap mapped data without copying back
  // (Not needed for GraphIt mapping thus far)
  //void dump();

  // Could be useful to create an array in-place in SPM without copying data in
  // (Not needed for GraphIt mapping thus far; how would unmapping be handled?)
  //template <typename T> T* allocate(long new_data_count);

  void enableAtomic(char op);
  void disableAtomic();

  // Note that writeX and cas32 assume that dst_ID < the max mapped ID

  // Used for performing OMEGA-accelerated writeAdd and writeMin
  // Graphit only generates writeAdd and writeMin ops for int, float, and double targets
  inline void writeX(int    val, NodeID dst_ID);
  inline void writeX(float  val, NodeID dst_ID);
  inline void writeX(double val, NodeID dst_ID);

  // Used for performing OMEGA-accelerated 32-bit compare_and_swap
  // GraphIt only generates compare_and_swap ops for int and float targets
  inline void cas32(int   val, int   dst_cmp_val, NodeID dst_ID);
  inline void cas32(float val, float dst_cmp_val, NodeID dst_ID);

  void mapSpareActive(uintE *outEdges, long outEdgeCount);
  void unmapSparseActive();
  inline void setSparseOffset(uintT offset);
  inline bool enableDeduplication();
  inline bool disableDeduplication();

  inline void waitForPISCs();

  // const might not work here, will have to check...
  inline volatile bool* getMappedNext()  { return mapped_next; }
  inline uint64_t       getActiveCount() { return *total_active_vertices_vaddr; }

  inline const int  mappedCount()    { return curr_data_count;                       }
  inline const bool allInSPM()       { return curr_fits_in_spm;                      }
  inline const bool inSPM(NodeID ID) { return (ID < curr_data_count ? true : false); }
  inline const int  grainSize()      { return curr_grain_size;                       }

private:
  // These are defined here so the atomic func definitions can use them
  #define VERTEX_DATA_SECTION 0x300008
  volatile void*     src_info         = (void*)    (VERTEX_DATA_SECTION + 0x110);
  volatile uint32_t* cas_compare_info = (uint32_t*)(VERTEX_DATA_SECTION + 0x148);
  volatile uint64_t* dedup_enabled    = (uint64_t*)(VERTEX_DATA_SECTION + 0x150);
  volatile NodeID*   dst_info         = (NodeID*)  (VERTEX_DATA_SECTION + 0x118);
  volatile unsigned int* edge_info    = (uintE*)(VERTEX_DATA_SECTION + 0x128);
  volatile uint64_t* total_active_vertices_vaddr = (uint64_t*)(VERTEX_DATA_SECTION + 0x140);
  volatile uint64_t* outstanding_pisc_ops = (uint64_t*)(VERTEX_DATA_SECTION + 0x208);
  #undef  VERTEX_DATA_SECTION

  char* curr_data       = nullptr;
  long  curr_data_count = 0;
  int   curr_grain_size = 0;

  char* temp_data       = nullptr;
  bool* mapped_next     = nullptr;

  bool curr_fits_in_spm = false;
  long max_mapped_count = 0;
};

// Inline functions can't be defined in the .cpp
// Not using templates for these since templates defined in this header
// can't easily be limited to supporting only certain types.
inline void OMEGA_API::writeX(int val, NodeID dst_ID) {
  *(volatile int*)src_info = val;
  *dst_info                = dst_ID;
}

inline void OMEGA_API::writeX(float val, NodeID dst_ID) {
  *(volatile float*)src_info = val;
  *dst_info                  = dst_ID;
}

inline void OMEGA_API::writeX(double val, NodeID dst_ID) {
  *(volatile double*)src_info = val;
  *dst_info                   = dst_ID;
}

inline void OMEGA_API::cas32(int val, int dst_cmp_val, NodeID dst_ID) {
  *(volatile uint32_t*)src_info = reinterpret_cast<uint32_t &>(val);
  *cas_compare_info             = reinterpret_cast<uint32_t &>(dst_cmp_val);
  *dst_info                     = dst_ID;
}

inline void OMEGA_API::cas32(float val, float dst_cmp_val, NodeID dst_ID) {
  *(volatile uint32_t*)src_info = reinterpret_cast<uint32_t &>(val);
  *cas_compare_info             = reinterpret_cast<uint32_t &>(dst_cmp_val);
  *dst_info                     = dst_ID;
}

inline void OMEGA_API::setSparseOffset(uintT offset) {
  *edge_info = (unsigned int)offset;
}

inline bool OMEGA_API::enableDeduplication() {
  *dedup_enabled = true;
}

inline bool OMEGA_API::disableDeduplication() {
  *dedup_enabled = false;
}

inline void OMEGA_API::waitForPISCs() {
  // Loop until no outstanding PISC ops remain
  while (*outstanding_pisc_ops) {};
}

// Function wrappers for class methods since the GraphIt compiler doesn't
// natively support calling class methods (could be done as a variable as 
// a variable statement, but that's kind of clunky).

extern OMEGA_API omega;

inline void omegaInit() {
  omega.init();
}

template <typename T>
inline void omegaMap(T* new_data, long new_data_count) {
  omega.map(new_data, new_data_count);
}

template <typename T>
inline void omegaUnmap(T* data_dst) {
  omega.unmap(data_dst);
}

inline void omegaEnableAtomic(char op) {
  omega.enableAtomic(op);
}

inline void omegaDisableAtomic() {
  omega.disableAtomic();
}

template <typename T>
inline void omegaWriteX(T val, NodeID dst_ID) {
  omega.writeX(val, dst_ID);
}

template <typename T>
inline void omegaCas32(T val, T dst_cmp_val, NodeID dst_ID) {
  omega.cas32(val, dst_cmp_val, dst_ID);
}

inline void omegaMapSparseActive(uintE *outEdges, long outEdgeCount) {
  omega.mapSpareActive(outEdges, outEdgeCount);
}

inline void omegaUnmapSparseActive() {
  omega.unmapSparseActive();
}

inline void omegaSetSparseOffset(uintT offset) {
  omega.setSparseOffset(offset);
}

inline void omegaEnableDeduplication() {
  omega.enableDeduplication();
}

inline void omegaDisableDeduplication() {
  omega.disableDeduplication();
}

inline void omegaWaitForPISCs() {
  omega.waitForPISCs();
}

inline volatile bool* omegaGetMappedNext() {
  return omega.getMappedNext();
}

inline uint64_t omegaGetActiveCount() {
  return omega.getActiveCount();
}

inline const int omegaMappedCount() {
  return omega.mappedCount();
}

inline const bool omegaAllInSPM() {
  return omega.allInSPM();
}

inline const bool omegaInSPM(NodeID ID) {
  return omega.inSPM(ID);
}

inline const int omegaGrainSize() {
  return omega.grainSize();
}

#endif
