/*
 * Copyright (c) 1999-2012 Mark D. Hill and David A. Wood
 * Copyright (c) 2013 Advanced Micro Devices, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __MEM_RUBY_STRUCTURES_SPM_HH__
#define __MEM_RUBY_STRUCTURES_SPM_HH__

#include <string>
#include <unordered_map>
#include <vector>

#include "base/statistics.hh"
#include "mem/protocol/CacheRequestType.hh"
#include "mem/protocol/CacheResourceType.hh"
#include "mem/protocol/RubyRequest.hh"
#include "mem/ruby/common/DataBlock.hh"
#include "mem/ruby/slicc_interface/AbstractCacheEntry.hh"
#include "mem/ruby/slicc_interface/RubySlicc_ComponentMapping.hh"
#include "mem/ruby/structures/spm/AbstractReplacementPolicySPM.hh"
#include "mem/ruby/structures/BankedArray.hh"
#include "mem/ruby/system/CacheRecorder.hh"
#include "params/RubySPM.hh"
#include "sim/sim_object.hh"

class ScratchpadMemory : public SimObject
{
  public:
    typedef RubySPMParams Params;
    ScratchpadMemory(const Params *p);
    ~ScratchpadMemory();

    void init();

    // Public Methods
    // perform a cache access and see if we hit or not.  Return true on a hit.
    bool tryCacheAccess(Addr address, RubyRequestType type,
                        DataBlock*& data_ptr);

    // similar to above, but doesn't require full access check
    bool testCacheAccess(Addr address, RubyRequestType type,
                         DataBlock*& data_ptr);

    // tests to see if an address is present in the cache
    bool isTagPresent(Addr address) const;

    
    //Abraham - adding source & destination scratchpads
    void atomicExecution(PacketPtr pkt, DataBlock& entry, int offset);
    int aggregate(const DataBlock& entry_ntk, DataBlock& entry_spm);
    //void setDataSPM(const DataBlock& blk_ntk, DataBlock& blk_spm);
    //void setDataSPM(int aggrValue, DataBlock& blk);
    void setDataSPM(PacketPtr pkt, DataBlock& blk, int offset, bool isAtomic=false);
    void isVertexActiveSPM(PacketPtr pkt, DataBlock& blk, int offset);
    void setSparseActiveVertex(PacketPtr pkt, uint8_t success);
    
    int maxNumAccel();
    int hashDest(Addr addr);

    // Returns true if there is:
    //   a) a tag match on this address or there is
    //   b) an unused line in the same cache "way"
    bool cacheAvail(Addr address) const;
    mutable bool flag = false;
    // find an unused entry and sets the tag appropriate for the address
    AbstractCacheEntry* allocate(Addr address,
                                 AbstractCacheEntry* new_entry, bool touch);
    AbstractCacheEntry* allocate(Addr address, AbstractCacheEntry* new_entry)
    {
        return allocate(address, new_entry, true);
    }
    void allocateVoid(Addr address, AbstractCacheEntry* new_entry)
    {
        allocate(address, new_entry, true);
    }

    // Explicitly free up this address
    void deallocate(Addr address);

    // Returns with the physical address of the conflicting cache line
    Addr cacheProbe(Addr address) const;
    
    // Returns true if there is a replaceable cache line.
    // Returns false if all cachelines are frequent
    bool isVictimPresent(Addr address) const;
    
    // looks an address up in the cache
    AbstractCacheEntry* lookup(Addr address);
    const AbstractCacheEntry* lookup(Addr address) const;

    Cycles getTagLatency() const { return tagArray.getLatency(); }
    Cycles getDataLatency() const { return dataArray.getLatency(); }

    bool isBlockInvalid(int64_t cache_set, int64_t loc);
    bool isBlockNotBusy(int64_t cache_set, int64_t loc);

    // Hook for checkpointing the contents of the cache
    void recordCacheContents(int cntrl, CacheRecorder* tr) const;

    // Set this address to most recently used
    void incFrequency(Addr address);
    // Set this entry to most recently used
    void incFrequency(const AbstractCacheEntry *e);

    // Functions for locking and unlocking cache lines corresponding to the
    // provided address.  These are required for supporting atomic memory
    // accesses.  These are to be used when only the address of the cache entry
    // is available.  In case the entry itself is available. use the functions
    // provided by the AbstractCacheEntry class.
    void setLocked (Addr addr, int context);
    void clearLocked (Addr addr);
    bool isLocked (Addr addr, int context);

    // Print cache contents
    void print(std::ostream& out) const;
    void printData(std::ostream& out) const;

    void regStats();
    bool checkResourceAvailable(CacheResourceType res, Addr addr);
    void recordRequestType(CacheRequestType requestType, Addr addr);
    
    //Abraham - adding source & destination scratchpads
    //setting values specific to the scratchpad implementations
    void setKey (Addr key);
    void setOppType (char oppType);
    void setHashType (char hashType);
    void setKeyType (char keyType);
    void incrNumAccelCompleted();

    //getting values specific to the scratchpad implementations
    Addr getKey () const;
    char getOppType () const;
    char getHashType () const;
    char getKeyType () const;
    Addr getKeyAddr () const;
    Addr getValueAddr () const;
    Addr getOppTypeAddr () const;
    Addr getHashTypeAddr () const;
    Addr getKeyTypeAddr () const;
    Addr getSpillStartAddr () const;
    Addr getMaxSpillAddr () const;
    Addr getNeedSecHashingAddr () const;
    Addr getMapCompleteAddr () const;
    int getNumAccelCompleted () const;
    int getNumAssoc () const;
    int getNumSets () const;
    int getNumEntries () const;


    std::vector<std::pair<char, int>> latency_per_app{std::make_pair('0', 1),
                                                     std::make_pair('1', 3),
                                                     std::make_pair('2', 1),
                                                     std::make_pair('3', 2),
                                                     std::make_pair('4', 3),
                                                     std::make_pair('5', 1),
                                                     std::make_pair('6', 2),
                                                     std::make_pair('7', 1),
                                                     std::make_pair('8', 1),
                                                     std::make_pair('a', 3),
                                                     std::make_pair('b', 1),
                                                     std::make_pair('c', 2),
                                                     
                                                     // Nicholas - adding Graphit-supported atomic operations
                                                     // Should match Sequencer.hh::latency_per_app
                                                     std::make_pair('d', 1), // ATOMIC_SUM: int (32-bit)
                                                     std::make_pair('e', 3), // ATOMIC_SUM: float
                                                     std::make_pair('f', 3), // ATOMIC_SUM: double
                                                     // Nicholas - TODO: are these (V) latencies reasonable?
                                                     std::make_pair('g', 1), // ATOMIC_MIN: int (32-bit)
                                                     std::make_pair('h', 1), // ATOMIC_MIN: float
                                                     std::make_pair('i', 1), // ATOMIC_MIN: double
                                                     std::make_pair('j', 1)  // CAS       : 32-bit
                                                     };

    
 
  public:
   //Abraham for pisc power and energy evaluation 
    Stats::Scalar m_atomic_execution;
    Stats::Scalar m_demand_hits;
    Stats::Scalar m_demand_misses;
    Stats::Formula m_demand_accesses;

    Stats::Scalar m_sw_prefetches;
    Stats::Scalar m_hw_prefetches;
    Stats::Formula m_prefetches;

    Stats::Vector m_accessModeType;

    Stats::Scalar numDataArrayReads;
    Stats::Scalar numDataArrayWrites;
    Stats::Scalar numTagArrayReads;
    Stats::Scalar numTagArrayWrites;

    Stats::Scalar numTagArrayStalls;
    Stats::Scalar numDataArrayStalls;

    int getCacheSize() const { return m_spm_size; }
    int getNumBlocks() const { return m_spm_num_sets * m_spm_assoc; }
    Addr getAddressAtIdx(int idx) const;

    Cycles getAtomicOverhead();
    
    //Abraham - adding source & destination scratchpads
    //identify if this object is of type sspm or dspm
    bool m_is_vertex_only_cache;

  private:
  
        //Abraham - adding source & destination scratchpads
    int primeVersion() const;
    int64_t moduloHash(Addr addr) const;
    int64_t simpleXORHash(Addr addr) const;
    int64_t rotateXORHash(Addr addr) const;

    // convert a Address to its location in the cache
    int64_t addressToCacheSet(Addr address) const;

    // Given a cache tag: returns the index of the tag in a set.
    // returns -1 if the tag is not found.
    int findTagInSet(int64_t line, Addr tag) const;
    int findTagInSetIgnorePermissions(int64_t cacheSet, Addr tag) const;

    // Private copy constructor and assignment operator
    ScratchpadMemory(const ScratchpadMemory& obj);
    ScratchpadMemory& operator=(const ScratchpadMemory& obj);

  private:
    // Data Members (m_prefix)
    
    //Abraham - trying to read itercount
    System* system;

    //Abraham - adding source & destination scratchpads
    //key variable - we need to store key until the value 
    //arrive from the core
    Addr key;
    // type of operation - + -addition, i - increament, m - min, and x - max
    char oppType; 
    // type of hash function - m - modulo, x - xorrotate, s - simple xor, 
    //f - FNV-1a, 
    char hashType; 
    
    // type of key -- those that write a character at a time and those that
    //write the whole key at a time
    // p (partial) - word count, pvc, min_max, average, c (complete) - 
    //historgram, linear regression 
    char keyType; 
    
    //the largest prime number smaller than the number of sets 
    //in the scratchpads
    int primeNumSets; 
   
    //Number of accelerators that finish execution
    uint32_t numAccelCompleted; 
    
    //The following parameters are used to store addresses for key, value, 
    //type of opperation, type of hash function respectively
    Addr m_keyAddr; 
    Addr m_valueAddr;
    Addr m_oppTypeAddr;
    Addr m_hashTypeAddr;
    Addr m_keyTypeAddr;
    Addr m_spillStartAddr;
    Addr m_maxSpillAllocAddr;
    Addr m_mapCompleteAddr;
    Addr m_needSecHashingAddr;
    int m_numCPU;
    int m_block_size;
    //identify what kind of memory this object is  
    bool m_is_instruction_only_cache;
    
    // The first index is the key.
    // The second index is the the amount associativity.
    std::unordered_map<Addr, int> m_tag_index;
    std::vector<std::vector<AbstractCacheEntry*> > m_spm;

    AbstractReplacementPolicySPM *m_replacementPolicy_ptr;

    BankedArray dataArray;
    BankedArray tagArray;

    int maxChunkSize = 1024;
    int m_spm_size;
    int m_spm_num_sets;
    int m_spm_num_set_bits;
    int m_spm_assoc;
    int m_start_index_bit;
    bool m_resource_stalls;
};

std::ostream& operator<<(std::ostream& out, const ScratchpadMemory& obj);

#endif // __MEM_RUBY_STRUCTURES_SPM_HH__
