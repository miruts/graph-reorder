#include "OMEGA_API.h"
#include <inttypes.h>
#include <stdexcept>
#include <iostream>

// ---------------------------Begin: Template instantiations-----------------------------

// GraphIt can generate arrays of type: int, uintE (unsigned int), uint64_t, float, double, bool
template void OMEGA_API::map(int*      new_data, long new_data_count);
template void OMEGA_API::map(uintE*    new_data, long new_data_count);
template void OMEGA_API::map(uint64_t* new_data, long new_data_count);
template void OMEGA_API::map(float*    new_data, long new_data_count);
template void OMEGA_API::map(double*   new_data, long new_data_count);
template void OMEGA_API::map(bool*     new_data, long new_data_count);

template void OMEGA_API::unmap(int*      data_dst);
template void OMEGA_API::unmap(uintE*    data_dst);
template void OMEGA_API::unmap(uint64_t* data_dst);
template void OMEGA_API::unmap(float*    data_dst);
template void OMEGA_API::unmap(double*   data_dst);
template void OMEGA_API::unmap(bool*     data_dst);

// ----------------------------End: Template instantiations------------------------------

// ---------------------------Begin: OMEGA Control Registers-----------------------------

// Mostly copied from accelerated Ligra

#define VERTEX_DATA_SECTION 0x300008

volatile uint64_t* datatype_size_vaddr1 = (uint64_t*)(VERTEX_DATA_SECTION );
volatile uint64_t* datatype_size_vaddr2 = (uint64_t*)(VERTEX_DATA_SECTION + 0x8);
volatile uint64_t* datatype_size_vaddr3 = (uint64_t*)(VERTEX_DATA_SECTION + 0x10);
volatile uint64_t* datatype_size_vaddr4 = (uint64_t*)(VERTEX_DATA_SECTION + 0x18);

volatile uint64_t* stride_vaddr1 = (uint64_t*)(VERTEX_DATA_SECTION + 0x20);
volatile uint64_t* stride_vaddr2 = (uint64_t*)(VERTEX_DATA_SECTION + 0x28);
volatile uint64_t* stride_vaddr3 = (uint64_t*)(VERTEX_DATA_SECTION + 0x30);
volatile uint64_t* stride_vaddr4 = (uint64_t*)(VERTEX_DATA_SECTION + 0x38);

volatile uint64_t* num_vertex = (uint64_t*)(VERTEX_DATA_SECTION + 0x40);

volatile uint64_t* mapped_vaddr1 = (uint64_t*)(VERTEX_DATA_SECTION + 0x50);
volatile uint64_t* mapped_vaddr2 = (uint64_t*)(VERTEX_DATA_SECTION + 0x58);
volatile uint64_t* mapped_vaddr3 = (uint64_t*)(VERTEX_DATA_SECTION + 0x60);
volatile uint64_t* mapped_vaddr4 = (uint64_t*)(VERTEX_DATA_SECTION + 0x68);

volatile uint64_t* enable_tracked_vaddr1 = (uint64_t*)(VERTEX_DATA_SECTION + 0x70);
volatile uint64_t* enable_tracked_vaddr2 = (uint64_t*)(VERTEX_DATA_SECTION + 0x78);
volatile uint64_t* enable_tracked_vaddr3 = (uint64_t*)(VERTEX_DATA_SECTION + 0x80);
volatile uint64_t* enable_tracked_vaddr4 = (uint64_t*)(VERTEX_DATA_SECTION + 0x88);

volatile uint64_t* enable_prefetch_vaddr = (uint64_t*)(VERTEX_DATA_SECTION + 0x90);
volatile uint64_t* prefetch_start_vaddr = (uint64_t*)(VERTEX_DATA_SECTION + 0x98);
volatile uint64_t* prefetch_size_vaddr = (uint64_t*)(VERTEX_DATA_SECTION + 0xa0);

volatile uint64_t* atomic_oldValue_vaddr1 = (uint64_t*)(VERTEX_DATA_SECTION + 0xa8);
volatile uint64_t* atomic_oldValue_vaddr2 = (uint64_t*)(VERTEX_DATA_SECTION + 0xb0);

volatile bool* atomic_status_vaddr1 = (bool*)(VERTEX_DATA_SECTION + 0xb8);
volatile bool* atomic_status_vaddr2 = (bool*)(VERTEX_DATA_SECTION + 0xc0);

volatile uint64_t* enable_atomic_vaddr = (uint64_t*)(VERTEX_DATA_SECTION + 0xc8);

volatile char* atomic_opp_type = (char*)(VERTEX_DATA_SECTION + 0xd0);
//volatile char* atomic_opp_type2 = (char*)(VERTEX_DATA_SECTION + 0xd8);

volatile uint64_t* no_status_read = (uint64_t*)(VERTEX_DATA_SECTION + 0xe0);

volatile unsigned int* num_vertex_mapped_vaddr = (unsigned int*)(VERTEX_DATA_SECTION + 0xe8);
volatile uint64_t* num_edges_mapped = (uint64_t*)(VERTEX_DATA_SECTION + 0xf0);
volatile uint64_t* num_tracked_atomic = (uint64_t*)(VERTEX_DATA_SECTION + 0xf8);

volatile uint64_t* is_sparse = (uint64_t*)(VERTEX_DATA_SECTION + 0x100);
volatile uint64_t* is_copy = (uint64_t*)(VERTEX_DATA_SECTION + 0x108);
//not the from 108 to 128, they are assigned for src_, dest_, add_info per app

// Defined in the class
//volatile void* src_info = (void*)(VERTEX_DATA_SECTION + 0x110);
//volatile NodeID* dst_info = (NodeID*)(VERTEX_DATA_SECTION + 0x118);

// add_info exists here

// Defined in the class
//volatile unsigned int* edge_info = (uintE*)(VERTEX_DATA_SECTION + 0x128);
volatile uint64_t* chunk_size_vaddr = (uint64_t*)(VERTEX_DATA_SECTION + 0x130);

volatile uint64_t* active_vertices_vaddr = (uint64_t*)(VERTEX_DATA_SECTION + 0x138);

// Defined in the class
//volatile uint64_t* total_active_vertices_vaddr = (uint64_t*)(VERTEX_DATA_SECTION + 0x140);

// Defined in the class
//volatile uint32_t* cas_compare_info = (uint32_t*)(VERTEX_DATA_SECTION + 0x148);

// Defined in the class
//volatile uint64_t* dedup_enabled = (uint64_t*)(VERTEX_DATA_SECTION + 0x150);

//volatile uint64_t* outstanding_pisc_ops = (uint64_t*)(VERTEX_DATA_SECTION + 0x208);

// ----------------------------End: OMEGA Control Registers------------------------------

// ---------------------------Begin: OMEGA_API Implementation----------------------------

// Just to be safe
#if VERTEXLINE_SIZE > 8 || VERTEXLINE_SIZE < 1
  #error VERTEXLINE_SIZE must be greater than 0B and not more than 8B!
#endif

OMEGA_API::~OMEGA_API() {
  // The user is responsible for unmapping (if they wish) before this deconstructor is called
  // Free dynamic arrays used by the instance
  if (temp_data != nullptr)   delete [] temp_data;
  if (mapped_next != nullptr) delete [] mapped_next;
}

void OMEGA_API::init() {
  max_mapped_count = *num_vertex_mapped_vaddr;

  // Allocate a temporary array for moving data to and from SPM
  // and the boolean array used for PISC frontier tracking
  temp_data   = new char [max_mapped_count * VERTEXLINE_SIZE];
  mapped_next = new bool [max_mapped_count];

  // Configure OMEGA
  *atomic_opp_type       = '1';
  *chunk_size_vaddr      = 1;                 // (vertex_id / chunk_size) % cpu_count
  *num_vertex            = 0;
  *datatype_size_vaddr1  = VERTEXLINE_SIZE;
  *datatype_size_vaddr3  = sizeof(bool);
  *stride_vaddr1         = VERTEXLINE_SIZE;
  *stride_vaddr3         = sizeof(bool);
  *mapped_vaddr1         = (uint64_t) temp_data;
  *mapped_vaddr3         = (uint64_t) mapped_next;
  *enable_prefetch_vaddr = 0;
  *enable_tracked_vaddr1 = 1;
  *enable_tracked_vaddr3 = 1;
  *is_sparse             = false;

  ligra::parallel_for_lambda((long)0, (long)max_mapped_count, [&] (long i) {
    mapped_next[i] = 0;
  }, CACHELINE_SIZE / sizeof(bool));
  //for (long i = 0; i < max_mapped_count; i++) { mapped_next[i] = 0; }
}

template <typename T>
void OMEGA_API::map(T* new_data, long new_data_count) {
  // Check that the size is safe here?

  // If this data is already mapped, do nothing
  // (Assumes that we'll never want to change the number of mapped elements for a given array)
  if (((char*)new_data) == curr_data) {
    return;
  }

  // If data is mapped (and it isn't new_data) print an error and exit
  // (this could be replaced with an exception)
  if ((char*)curr_data != nullptr) {
    std::cout << "OMEGA: Must unmap existing data before mapping new data!" << std::endl;
    exit(1);
  }

  // Cap the number of mapped elements, if required
  // Set the fits_in_spm flag accordingly
  if (new_data_count > max_mapped_count) {
    new_data_count   = max_mapped_count;
    curr_fits_in_spm = false;
  } else {
    curr_fits_in_spm = true;
  }

  // Compute the grain/chunk size to use for this data
  curr_grain_size = CACHELINE_SIZE / sizeof(T);

  // Configure OMEGA for the new data
  *chunk_size_vaddr      = curr_grain_size;   // Each core's SPM gets 'curr_grain_size' vertices mapped to it
  *num_vertex            = new_data_count;
  *datatype_size_vaddr1  = sizeof(T);
  *stride_vaddr1         = sizeof(T);
  *mapped_vaddr1 = (uint64_t) temp_data;

  // Copy data into SPM
  ligra::parallel_for_lambda((long)0, (long)new_data_count, [&] (long i) {
    ((volatile T*)temp_data)[i] = new_data[i];
  }, curr_grain_size);

  // Map the new data's pointer to SPM
  *mapped_vaddr1 = (uint64_t) new_data;

  // Update the current data pointer and element count
  curr_data       = (char*)new_data;
  curr_data_count = new_data_count;
}

template <typename T>
void OMEGA_API::unmap(T* data_dst) {
  // If no data is mapped, do nothing
  if (curr_data_count == 0) {
    return;
  }

  // Map the temporary array so that curr_data points to non-SPM memory
  *mapped_vaddr1 = (uint64_t) temp_data;

  // Copy from SPM using all threads
  // grain_size is set such that threads (hopefully) don't share cache lines
  ligra::parallel_for_lambda((long)0, (long)curr_data_count, [&] (long i) {
    data_dst[i] = ((volatile T*)temp_data)[i];
  }, curr_grain_size);

  // Now, no data is mapped to SPM
  curr_data       = nullptr;
  curr_data_count = 0;
  curr_grain_size = 0;        // Setting grain_size to 0, as this is the default for GraphIt's parallel_for_lambda
}

void OMEGA_API::enableAtomic(char op) {
  *atomic_opp_type     = op;
  *enable_atomic_vaddr = 1;
}

void OMEGA_API::disableAtomic() {
  *enable_atomic_vaddr = 0;
}

void OMEGA_API::mapSpareActive(uintE *outEdges, long outEdgeCount) {
  *datatype_size_vaddr3 = sizeof(uintE);
  *stride_vaddr3 = sizeof(uintE); 
  *mapped_vaddr3 = (int64_t)outEdges;
  *is_sparse = true;
  *num_edges_mapped = outEdgeCount;
  *enable_tracked_vaddr3 = 1;
}

void OMEGA_API::unmapSparseActive() {
  *enable_tracked_vaddr3 = 0;

  *datatype_size_vaddr3  = sizeof(bool);
  *stride_vaddr3         = sizeof(bool);
  *mapped_vaddr3         = (uint64_t) mapped_next;
  *enable_tracked_vaddr3 = 1;
  *is_sparse             = false;

  *enable_tracked_vaddr3 = 1;
}

// ----------------------------End: OMEGA_API Implementation-----------------------------
