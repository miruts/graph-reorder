/*
 * Copyright (c) 1999-2008 Mark D. Hill and David A. Wood
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

#ifndef __MEM_RUBY_SYSTEM_SEQUENCER_HH__
#define __MEM_RUBY_SYSTEM_SEQUENCER_HH__

#include <iostream>
#include <unordered_map>

#include "mem/protocol/MachineType.hh"
#include "mem/protocol/RubyRequestType.hh"
#include "mem/protocol/SequencerRequestType.hh"
#include "mem/ruby/common/Address.hh"
#include "mem/ruby/structures/CacheMemory.hh"
#include "mem/ruby/structures/spm/ScratchpadMemory.hh"
#include "mem/ruby/system/RubyPort.hh"
#include "params/RubySequencer.hh"

//Abraham - adding source & destination scratchpads
//These header files are included to get access to the threadcontext
//and cpu 
#include "cpu/base.hh"
#include "cpu/thread_context.hh"

// Nicholas - Adding async PISCs
// When defined, cores don't wait for PISC ops to complete before
// removing the launching MMIO store from the store queue
#define ASYNC_PISCS

struct SequencerRequest
{
    PacketPtr pkt;
    RubyRequestType m_type;
    Cycles issue_time;

    SequencerRequest(PacketPtr _pkt, RubyRequestType _m_type,
                     Cycles _issue_time)
        : pkt(_pkt), m_type(_m_type), issue_time(_issue_time)
    {}
};

std::ostream& operator<<(std::ostream& out, const SequencerRequest& obj);

class Sequencer : public RubyPort
{
  public:
    typedef RubySequencerParams Params;
    Sequencer(const Params *);
    ~Sequencer();

    // Public Methods
    void wakeup(); // Used only for deadlock detection
    void resetStats();
    void collateStats();
    void regStats();

    //Abraham - added to  handle different ways of calling from the protocol 
    
    void writeCallback(Addr address,
                       const DataBlock& data,
                       const bool externalHit = false,
                       const int offset = 0,
                       MachineID machID = {MachineType_NUM,0},
                       const MachineType mach = MachineType_NUM,
                       const Cycles initialRequestTime = Cycles(0),
                       const Cycles forwardRequestTime = Cycles(0),
                       const Cycles firstResponseTime = Cycles(0));

    
    void writeCallback(Addr address,
                       DataBlock& data,
                       const bool externalHit = false,
                       const int offset = 0,
                       MachineID machID = {MachineType_NUM,0},
                       const MachineType mach = MachineType_NUM,
                       const Cycles initialRequestTime = Cycles(0),
                       const Cycles forwardRequestTime = Cycles(0),
                       const Cycles firstResponseTime = Cycles(0));

    
    //Abraham - added
    void readCallback(Addr address,
                      const DataBlock& data,
                      const bool externalHit = false,
                      const int offset = 0,
                       MachineID machID = {MachineType_NUM,0},
                      const MachineType mach = MachineType_NUM,
                      const Cycles initialRequestTime = Cycles(0),
                      const Cycles forwardRequestTime = Cycles(0),
                      const Cycles firstResponseTime = Cycles(0));

   
    void readCallback(Addr address,
                      DataBlock& data,
                      const bool externalHit = false,
                      const int offset = 0,
                       MachineID machID = {MachineType_NUM,0},
                      const MachineType mach = MachineType_NUM,
                      const Cycles initialRequestTime = Cycles(0),
                      const Cycles forwardRequestTime = Cycles(0),
                      const Cycles firstResponseTime = Cycles(0));

    RequestStatus makeRequest(PacketPtr pkt);
    bool empty() const;
    int outstandingCount() const { return m_outstanding_count; }

    bool isDeadlockEventScheduled() const
    { return deadlockCheckEvent.scheduled(); }

    void descheduleDeadlockEvent()
    { deschedule(deadlockCheckEvent); }

    void print(std::ostream& out) const;
    void checkCoherence(Addr address);

    void markRemoved();
    void evictionCallback(Addr address);
    void invalidateSC(Addr address);

    void recordRequestType(SequencerRequestType requestType);
    Stats::Histogram& getOutstandReqHist() { return m_outstandReqHist; }

    Stats::Histogram& getLatencyHist() { return m_latencyHist; }
    Stats::Histogram& getTypeLatencyHist(uint32_t t)
    { return *m_typeLatencyHist[t]; }

    Stats::Histogram& getHitLatencyHist() { return m_hitLatencyHist; }
    Stats::Histogram& getHitTypeLatencyHist(uint32_t t)
    { return *m_hitTypeLatencyHist[t]; }

    Stats::Histogram& getHitMachLatencyHist(uint32_t t)
    { return *m_hitMachLatencyHist[t]; }

    Stats::Histogram& getHitTypeMachLatencyHist(uint32_t r, uint32_t t)
    { return *m_hitTypeMachLatencyHist[r][t]; }

    Stats::Histogram& getMissLatencyHist()
    { return m_missLatencyHist; }
    Stats::Histogram& getMissTypeLatencyHist(uint32_t t)
    { return *m_missTypeLatencyHist[t]; }

    Stats::Histogram& getMissMachLatencyHist(uint32_t t) const
    { return *m_missMachLatencyHist[t]; }

    Stats::Histogram&
    getMissTypeMachLatencyHist(uint32_t r, uint32_t t) const
    { return *m_missTypeMachLatencyHist[r][t]; }

    Stats::Histogram& getIssueToInitialDelayHist(uint32_t t) const
    { return *m_IssueToInitialDelayHist[t]; }

    Stats::Histogram&
    getInitialToForwardDelayHist(const MachineType t) const
    { return *m_InitialToForwardDelayHist[t]; }

    Stats::Histogram&
    getForwardRequestToFirstResponseHist(const MachineType t) const
    { return *m_ForwardToFirstResponseDelayHist[t]; }

    Stats::Histogram&
    getFirstResponseToCompletionDelayHist(const MachineType t) const
    { return *m_FirstResponseToCompletionDelayHist[t]; }

    Stats::Counter getIncompleteTimes(const MachineType t) const
    { return m_IncompleteTimes[t]; }

  private:
    
    
    void addToRetryActiveVertexList(PacketPtr pkt)
    {
        if (std::find(retryActiveVertexList.begin(), retryActiveVertexList.end(), pkt) !=
               retryActiveVertexList.end()) return;
        retryActiveVertexList.push_back(pkt);
        
        #ifdef ASYNC_PISCS
        // Nicholas - Adding async PISCs
        // Count retryActiveVertexList entries as outstanding requests
        m_outstanding_count++;
        assert(m_outstanding_count <= m_max_outstanding_requests);
        #endif
    }

   
    void addReadOnlyCachedData(PacketPtr pkt, int contextId);
    //void invalidateReadOnlyCachedData(); 
    uint8_t* lookupReadOnlyCachedData (PacketPtr pkt, int contextId); 
      
    //Abraham - vertex related functions
    bool isEdgePrefetchSize(PacketPtr pkt);
    bool isOutEdgeActiveList(PacketPtr pkt);
    bool isEdgeInAddrRange(PacketPtr pkt, int addrRange);
    bool isOutEdgeLocal(PacketPtr pkt);
    bool isVertexStartAddr(PacketPtr pkt);
    bool isVertexLocal(PacketPtr pkt);
    bool isVertexInAddrRange(PacketPtr pkt, int addrRange);
    bool isVertexInAddrRange_old(PacketPtr pkt, int addrRange);
    void writeActiveVertex(PacketPtr pkt); 
    
    unsigned  setEntry(unsigned int vertexId);
    unsigned destSPM(unsigned int vertexId);
    
    //int dest(PacketPtr pkt);
    //uint64_t determineSet(PacketPtr pkt);
    //int determineOffset(PacketPtr pkt);
    
    void issueRequest(PacketPtr pkt, RubyRequestType type);

    void hitCallback(SequencerRequest* request, DataBlock& data,
                     int offset,
                     MachineID machID,
                     bool llscSuccess,
                     const MachineType mach, const bool externalHit,
                     const Cycles initialRequestTime,
                     const Cycles forwardRequestTime,
                     const Cycles firstResponseTime);

    void recordMissLatency(const Cycles t, const RubyRequestType type,
                           const MachineType respondingMach,
                           bool isExternalHit, Cycles issuedTime,
                           Cycles initialRequestTime,
                           Cycles forwardRequestTime, Cycles firstResponseTime,
                           Cycles completionTime);

    RequestStatus insertRequest(PacketPtr pkt, RubyRequestType request_type);
    bool handleLlsc(Addr address, SequencerRequest* request);

    // Private copy constructor and assignment operator
    Sequencer(const Sequencer& obj);
    Sequencer& operator=(const Sequencer& obj);

  private:
    
    std::vector<PacketPtr> retryActiveVertexList;
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
                                                     
                                                     // Nicholas - Adding Graphit-supported atomic operations
                                                     // Should match ScratchpadMemory.hh::latency_per_app
                                                     std::make_pair('d', 1), // ATOMIC_SUM: int (32-bit)
                                                     std::make_pair('e', 3), // ATOMIC_SUM: float
                                                     std::make_pair('f', 3), // ATOMIC_SUM: double
                                                     // Nicholas - TODO: are these (V) latencies reasonable?
                                                     std::make_pair('g', 1), // ATOMIC_MIN: int (32-bit)
                                                     std::make_pair('h', 1), // ATOMIC_MIN: float
                                                     std::make_pair('i', 1), // ATOMIC_MIN: double
                                                     std::make_pair('j', 1)  // CAS       : 32-bit
                                                     };

    
    //std::vector<std::pair<Addr, uint8_t*>> readOnlyCachedData;
    
    int m_max_outstanding_requests;
    Cycles m_deadlock_threshold;

    CacheMemory* m_dataCache_ptr;
    CacheMemory* m_instCache_ptr;
    //Abraham - added
    int m_numCPU;
    ScratchpadMemory* m_vertexCache_ptr;
    
    ThreadContext* ctx;

//Scratchpad configuration parameters
/*
    Addr* vertexStartAddr1 = system->vertexStartAddr1;
    Addr* vertexStartAddr2 = system->vertexStartAddr2;
    Addr* vertexStartAddr3 = system->vertexStartAddr3;
    
    Addr* vertexStartAddr4 = system->vertexStartAddr4;
    Addr* dataTypeSize1 = system->dataTypeSize1;
    Addr* dataTypeSize2 = system->dataTypeSize2;
    Addr* dataTypeSize3 = system->dataTypeSize3;
    Addr* dataTypeSize4 = system->dataTypeSize4;
    Addr* enableTrackedAddr1 = system->enableTrackedAddr1;
    Addr* enableTrackedAddr2 = system->enableTrackedAddr2;
    Addr* enableTrackedAddr3 = system->enableTrackedAddr3;
    Addr* enableTrackedAddr4 = system->enableTrackedAddr4;
    int* numVertex = system->numVertex;
    int* stride1 = system->stride1;
    int* stride2 = system->stride2;
    int* stride3 = system->stride3;
    int* stride4 = system->stride4;
   
    int* numOutEdges = system->numOutEdges;

    uint64_t* atomicOldValue1 = system->atomicOldValue1;
    uint64_t* atomicOldValue2 = system->atomicOldValue2;
    unsigned int* enableAtomic = system->enableAtomic;
    uint64_t* atomicDataMapped1 = system->atomicDataMapped1;
    uint64_t* atomicDataMapped2 = system->atomicDataMapped2;
    uint64_t* atomicDataMappedAddr1 = system->atomicDataMappedAddr1;
    uint64_t* atomicDataMappedAddr2 = system->atomicDataMappedAddr2;

    bool* isSparse = system->isSparse ;
    bool* isCopy = system ->isCopy;

    char* atomicOppType1 = system->atomicOppType1;
    char* atomicOppType2 = system->atomicOppType2;
    bool* noStatusRead = system->noStatusRead;
    unsigned* numTrackedAtomic = system->numTrackedAtomic;
    
    uint64_t* chunkSize = system->chunkSize;
   

*/
    //bool enableVertexCaching = false;
    //bool validVertexCaching = false;


    Addr configVertexStartAddr1;
    Addr configVertexStartAddr2;
    Addr configVertexStartAddr3;
    Addr configVertexStartAddr4;
    Addr configEnableTrackedAddr1;
    Addr configEnableTrackedAddr2;
    Addr configEnableTrackedAddr3;
    Addr configEnableTrackedAddr4;
    Addr configNumVertex;
    Addr configDataTypeSize1;
    Addr configDataTypeSize2;
    Addr configDataTypeSize3;
    Addr configDataTypeSize4;
    Addr configStride1;
    Addr configStride2;
    Addr configStride3;
    Addr configStride4;
  
    //For custom prefetcher implementation
    Addr configEnablePrefetch;
    Addr configPrefetchAddr;
    Addr configPrefetchSize;
    
    //For PISC implementation 
    Addr configOldValueAddr1;
    Addr configOldValueAddr2;
    Addr configAtomicStatusAddr1;
    Addr configAtomicStatusAddr2;
    Addr configEnableAtomicAddr; 
    Addr configAtomicOppTypeAddr1;
    Addr configAtomicOppTypeAddr2;
    Addr configNoStatusReadAddr;
    Addr configNumMappedVertices;
    Addr configNumOutEdge;
    Addr configNumTrackedAtomic;
    Addr configIsSparse; 
    Addr configIsCopy; 
    
    Addr configSrcInfo; 
    Addr configAddInfo; 
    Addr configDestInfo; 
    Addr configEdgeIndex; 
    Addr configChunkSize; 
    
    Addr configActiveVerticesTotal;
    Addr configActiveVerticesPerSpm;

    // Nicholas - Adding PISC support for Graphit-supported CAS operations
    Addr configCasCompareInfo;

    // Nicholas - Adding optional hardware deduplication
    Addr configDedupEnabled;

    // Nicholas - Adding profiling interface
    Addr configClearProfile;
    Addr configStartProfile;
    Addr configStopProfile;
    Addr configDumpProfile;
    //
    Addr configRangeStartProfile0;
    Addr configRangeBytesProfile0;
    Addr configRangeStartProfile1;
    Addr configRangeBytesProfile1;
    Addr configRangeStartProfile2;
    Addr configRangeBytesProfile2;
    //
    Addr configRangeStartProfile3;
    Addr configRangeBytesProfile3;
    Addr configRangeStartProfile4;
    Addr configRangeBytesProfile4;
    Addr configRangeStartProfile5;
    Addr configRangeBytesProfile5;
    //
    Addr configRangeStartProfile6;
    Addr configRangeBytesProfile6;
    Addr configRangeStartProfile7;
    Addr configRangeBytesProfile7;
    Addr configRangeStartProfile8;
    Addr configRangeBytesProfile8;
 
    // Nicholas - Adding outstanding PISC op tracking
    Addr configOutstandingPISCOps;

    int enablePrefetch;
    Addr prefetchAddr; 
    int prefetchSize;


    //uint64_t srcInfo;
    uint8_t srcInfo[8] = {0};
    int addInfo;
    unsigned int  edgeIndex;

    // Nicholas - Adding PISC support for Graphit-supported CAS operations
    // If the target address contains this value, the swap will take place
    // Note that GraphIt [currently] only uses 32-bit CAS operations
    uint8_t casCompareInfo[4] = {0};

   
    // The cache access latency for top-level caches (L0/L1). These are
    // currently assessed at the beginning of each memory access through the
    // sequencer.
    // TODO: Migrate these latencies into top-level cache controllers.
    Cycles m_data_cache_hit_latency;
    Cycles m_inst_cache_hit_latency;
    Cycles m_vertex_cache_hit_latency;

    typedef std::unordered_map<Addr, SequencerRequest*> RequestTable;
    RequestTable m_writeRequestTable;
    RequestTable m_readRequestTable;
    // Global outstanding request count, across all request tables
    int m_outstanding_count;

    #ifdef ASYNC_PISCS
    // Nicholas - Adding async PISCs
    #define ASYNC_PISC_LIMIT 16
    // Requires that Addr is uint64_t
    #define ASYNC_PISC_PADDR_START 0xFFFFFFFFFFFF0000
    unsigned int localOutstandingPISCOps = 0;
    std::vector<Addr> availablePISCPAddrs;
    #endif

    bool m_deadlock_check_scheduled;

    //Abraham - counting accesses to read-only local buffer
    Stats::Scalar m_local_vertex_buffer_access;
    
    //! Counters for recording aliasing information.
    Stats::Scalar m_store_waiting_on_load;
    Stats::Scalar m_store_waiting_on_store;
    Stats::Scalar m_load_waiting_on_store;
    Stats::Scalar m_load_waiting_on_load;

    bool m_usingNetworkTester;

    //! Histogram for number of outstanding requests per cycle.
    Stats::Histogram m_outstandReqHist;

    //! Histogram for holding latency profile of all requests.
    Stats::Histogram m_latencyHist;
    std::vector<Stats::Histogram *> m_typeLatencyHist;

    //! Histogram for holding latency profile of all requests that
    //! hit in the controller connected to this sequencer.
    Stats::Histogram m_hitLatencyHist;
    std::vector<Stats::Histogram *> m_hitTypeLatencyHist;

    //! Histograms for profiling the latencies for requests that
    //! did not required external messages.
    std::vector<Stats::Histogram *> m_hitMachLatencyHist;
    std::vector< std::vector<Stats::Histogram *> > m_hitTypeMachLatencyHist;

    //! Histogram for holding latency profile of all requests that
    //! miss in the controller connected to this sequencer.
    Stats::Histogram m_missLatencyHist;
    std::vector<Stats::Histogram *> m_missTypeLatencyHist;

    //! Histograms for profiling the latencies for requests that
    //! required external messages.
    std::vector<Stats::Histogram *> m_missMachLatencyHist;
    std::vector< std::vector<Stats::Histogram *> > m_missTypeMachLatencyHist;

    //! Histograms for recording the breakdown of miss latency
    std::vector<Stats::Histogram *> m_IssueToInitialDelayHist;
    std::vector<Stats::Histogram *> m_InitialToForwardDelayHist;
    std::vector<Stats::Histogram *> m_ForwardToFirstResponseDelayHist;
    std::vector<Stats::Histogram *> m_FirstResponseToCompletionDelayHist;
    std::vector<Stats::Counter> m_IncompleteTimes;


    class SequencerWakeupEvent : public Event
    {
      private:
        Sequencer *m_sequencer_ptr;

      public:
        SequencerWakeupEvent(Sequencer *_seq) : m_sequencer_ptr(_seq) {}
        void process() { m_sequencer_ptr->wakeup(); }
        const char *description() const { return "Sequencer deadlock check"; }
    };

    SequencerWakeupEvent deadlockCheckEvent;
};

inline std::ostream&
operator<<(std::ostream& out, const Sequencer& obj)
{
    obj.print(out);
    out << std::flush;
    return out;
}

#endif // __MEM_RUBY_SYSTEM_SEQUENCER_HH__
