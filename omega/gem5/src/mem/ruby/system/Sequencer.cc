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

#include "arch/x86/ldstflags.hh"
#include "base/misc.hh"
#include "base/str.hh"
#include "cpu/testers/rubytest/RubyTester.hh"
#include "debug/MemoryAccess.hh"
#include "debug/ProtocolTrace.hh"
#include "debug/RubySequencer.hh"
#include "debug/RubyStats.hh"
#include "mem/protocol/PrefetchBit.hh"
#include "mem/protocol/RubyAccessMode.hh"
#include "mem/ruby/profiler/Profiler.hh"
#include "mem/ruby/slicc_interface/RubyRequest.hh"
#include "mem/ruby/system/RubySystem.hh"
#include "mem/ruby/system/Sequencer.hh"
#include "mem/packet.hh"
#include "sim/system.hh"

#include "mem/page_table.hh"
#include "arch/generic/tlb.hh"

// Nicholas - Adding timing interface
#include "debug/NWTiming.hh"
// Nicholas - Temporary, print total counts for unhandled RubyRequestTypes
static unsigned long unhandledCounts[RubyRequestType_NUM] = {0};

using namespace std;

Sequencer *
RubySequencerParams::create()
{
    return new Sequencer(this);
}

Sequencer::Sequencer(const Params *p)
    : RubyPort(p), m_IncompleteTimes(MachineType_NUM), deadlockCheckEvent(this)
{
    m_outstanding_count = 0;

    m_instCache_ptr = p->icache;
    m_dataCache_ptr = p->dcache;
    m_vertexCache_ptr = p->vcache;
    m_data_cache_hit_latency = p->dcache_hit_latency;
    m_inst_cache_hit_latency = p->icache_hit_latency;
    m_vertex_cache_hit_latency = p->vertex_hit_latency;
    m_max_outstanding_requests = p->max_outstanding_requests;
    m_deadlock_threshold = p->deadlock_threshold;

    //Abraham - added
    m_numCPU = p->numCPU;

    configVertexStartAddr1 = p->configVertexStartAddr1;
    configVertexStartAddr2 = p->configVertexStartAddr2;
    configVertexStartAddr3 = p->configVertexStartAddr3;
    configVertexStartAddr4 = p->configVertexStartAddr4;
    configDataTypeSize1 = p->configDataTypeSize1;
    configDataTypeSize2 = p->configDataTypeSize2;
    configDataTypeSize3 = p->configDataTypeSize3;
    configDataTypeSize4 = p->configDataTypeSize4;
    configEnableTrackedAddr1 = p->configEnableTrackedAddr1;
    configEnableTrackedAddr2 = p->configEnableTrackedAddr2;
    configEnableTrackedAddr3 = p->configEnableTrackedAddr3;
    configEnableTrackedAddr4 = p->configEnableTrackedAddr4;

    configNumVertex = p->configNumVertex; 
    configNumOutEdge = p->configNumOutEdge; 
    configNumTrackedAtomic = p->configNumTrackedAtomic;
    configIsSparse = p->configIsSparse;
    configIsCopy = p->configIsCopy;
   
    configStride1 = p->configStride1;
    configStride2 = p->configStride2;
    configStride3 = p->configStride3;
    configStride4 = p->configStride4;

    configEnablePrefetch = p->configEnablePrefetch;
    configPrefetchAddr = p->configPrefetchAddr;
    configPrefetchSize = p->configPrefetchSize;

    configOldValueAddr1 = p->configOldValueAddr1;
    configOldValueAddr2 = p->configOldValueAddr2;
    configAtomicStatusAddr1 = p->configAtomicStatusAddr1;
    configAtomicStatusAddr2 = p->configAtomicStatusAddr2;
    configEnableAtomicAddr = p->configEnableAtomicAddr; 
    
    configAtomicOppTypeAddr1 = p->configAtomicOppTypeAddr1; 
    configAtomicOppTypeAddr2 = p->configAtomicOppTypeAddr2; 
    
    configNoStatusReadAddr = p->configNoStatusReadAddr;
    configNumMappedVertices = p->configNumMappedVertices;
    
    configSrcInfo = p->configSrcInfo;
    configAddInfo = p->configAddInfo;
    configDestInfo = p->configDestInfo;
    configEdgeIndex = p->configEdgeIndex;
    configChunkSize = p->configChunkSize;

    configActiveVerticesTotal = p->configActiveVerticesTotal;
    configActiveVerticesPerSpm = p->configActiveVerticesPerSpm;

    // Nicholas - Adding PISC support for Graphit-supported CAS operations
    configCasCompareInfo = p->configCasCompareInfo;

    // Nicholas - Adding optional hardware deduplication
    configDedupEnabled = p->configDedupEnabled;

    // Nicholas - Adding profiling interface
    configClearProfile = p->configClearProfile;
    configStartProfile = p->configStartProfile;
    configStopProfile  = p->configStopProfile;
    configDumpProfile  = p->configDumpProfile;
    //
    configRangeStartProfile0 = p->configRangeStartProfile0;
    configRangeBytesProfile0 = p->configRangeBytesProfile0;
    configRangeStartProfile1 = p->configRangeStartProfile1;
    configRangeBytesProfile1 = p->configRangeBytesProfile1;
    configRangeStartProfile2 = p->configRangeStartProfile2;
    configRangeBytesProfile2 = p->configRangeBytesProfile2;
    //
    configRangeStartProfile3 = p->configRangeStartProfile3;
    configRangeBytesProfile3 = p->configRangeBytesProfile3;
    configRangeStartProfile4 = p->configRangeStartProfile4;
    configRangeBytesProfile4 = p->configRangeBytesProfile4;
    configRangeStartProfile5 = p->configRangeStartProfile5;
    configRangeBytesProfile5 = p->configRangeBytesProfile5;
    //
    configRangeStartProfile6 = p->configRangeStartProfile6;
    configRangeBytesProfile6 = p->configRangeBytesProfile6;
    configRangeStartProfile7 = p->configRangeStartProfile7;
    configRangeBytesProfile7 = p->configRangeBytesProfile7;
    configRangeStartProfile8 = p->configRangeStartProfile8;
    configRangeBytesProfile8 = p->configRangeBytesProfile8;

    // Nicholas - Adding outstanding PISC op tracking
    configOutstandingPISCOps = p->configOutstandingPISCOps;

    #ifdef ASYNC_PISCS
    // Nicholas - Adding async PISCs
    assert(ASYNC_PISC_LIMIT >= m_max_outstanding_requests);
    // Create list of physical addresses to prevent "store_waiting_on_store" collisions for PISC ops
    for (int i = 0; i < ASYNC_PISC_LIMIT; i++) {
        availablePISCPAddrs.push_back(ASYNC_PISC_PADDR_START + i);
    }
    #endif

    assert(m_max_outstanding_requests > 0);
    assert(m_deadlock_threshold > 0);
    assert(m_instCache_ptr != NULL);
    assert(m_dataCache_ptr != NULL);
    assert(m_vertexCache_ptr != NULL);
    assert(m_data_cache_hit_latency > 0);
    assert(m_inst_cache_hit_latency > 0);

    m_usingNetworkTester = p->using_network_tester;
}

Sequencer::~Sequencer()
{
}

void
Sequencer::wakeup()
{
    assert(drainState() != DrainState::Draining);

    // Check for deadlock of any of the requests
    Cycles current_time = curCycle();

    // Check across all outstanding requests
    int total_outstanding = 0;

    RequestTable::iterator read = m_readRequestTable.begin();
    RequestTable::iterator read_end = m_readRequestTable.end();
    for (; read != read_end; ++read) {
        SequencerRequest* request = read->second;
       
       
       //DPRINTF(RubySequencer, "RRT p %#x v %#x size: %d\n", request->pkt->getAddr(), request->pkt->getVaddr(), m_readRequestTable.size());
		
			 if (current_time - request->issue_time < m_deadlock_threshold)
			 	continue;
			
        
        
        panic("Possible Deadlock detected. Aborting!\n"
              //"version: %d request.paddr: 0x%x request.vaddr: 0x%x m_readRequestTable: %d "
              "version: %d request.paddr: 0x%x request.vaddr: 0x%x m_readRequestTable: %d "
              "current time: %u issue_time: %d difference: %d\n", m_version,
              request->pkt->getAddr(), request->pkt->getVaddr(), m_readRequestTable.size(),
              current_time * clockPeriod(), request->issue_time * clockPeriod(),
              (current_time * clockPeriod()) - (request->issue_time * clockPeriod()));
						
    
    
    }

    RequestTable::iterator write = m_writeRequestTable.begin();
    RequestTable::iterator write_end = m_writeRequestTable.end();
    for (; write != write_end; ++write) {
        SequencerRequest* request = write->second;
        
				if (current_time - request->issue_time < m_deadlock_threshold)
				 	continue;
	/*  
    RequestTable::iterator read_tmp = m_readRequestTable.begin();
    RequestTable::iterator read_end_tmp = m_readRequestTable.end();

      for (; read_tmp != read_end_tmp; ++read_tmp) {
        SequencerRequest* request_tmp = read_tmp->second;
        
        DPRINTF(RubySequencer, "RRT p %#x v %#x size: %d\n", request_tmp->pkt->getAddr(), request_tmp->pkt->getVaddr(), m_readRequestTable.size());
      }
        DPRINTF(RubySequencer, "WRT p %#x v %#x size: %d\n", request->pkt->getAddr(), request->pkt->getVaddr(), m_writeRequestTable.size());
     */   
        panic("Possible Deadlock detected. Aborting!\n"
              "version: %d request.paddr: 0x%x request.vaddr: 0x%x m_writeRequestTable: %d "
              "current time: %u issue_time: %d difference: %d\n", m_version,
              request->pkt->getAddr(), request->pkt->getVaddr(), m_writeRequestTable.size(),
              current_time * clockPeriod(), request->issue_time * clockPeriod(),
              (current_time * clockPeriod()) - (request->issue_time * clockPeriod()));
							
    }

    total_outstanding += m_writeRequestTable.size();
    total_outstanding += m_readRequestTable.size();

    #ifdef ASYNC_PISCS
    // Nicholas - Adding async PISCs
    total_outstanding += retryActiveVertexList.size();
    #endif

    assert(m_outstanding_count == total_outstanding);

    if (m_outstanding_count > 0) {
        // If there are still outstanding requests, keep checking
        schedule(deadlockCheckEvent, clockEdge(m_deadlock_threshold));
    }
}

void Sequencer::resetStats()
{
    m_latencyHist.reset();
    m_hitLatencyHist.reset();
    m_missLatencyHist.reset();
    for (int i = 0; i < RubyRequestType_NUM; i++) {
        m_typeLatencyHist[i]->reset();
        m_hitTypeLatencyHist[i]->reset();
        m_missTypeLatencyHist[i]->reset();
        for (int j = 0; j < MachineType_NUM; j++) {
            m_hitTypeMachLatencyHist[i][j]->reset();
            m_missTypeMachLatencyHist[i][j]->reset();
        }
    }

    for (int i = 0; i < MachineType_NUM; i++) {
        m_missMachLatencyHist[i]->reset();
        m_hitMachLatencyHist[i]->reset();

        m_IssueToInitialDelayHist[i]->reset();
        m_InitialToForwardDelayHist[i]->reset();
        m_ForwardToFirstResponseDelayHist[i]->reset();
        m_FirstResponseToCompletionDelayHist[i]->reset();

        m_IncompleteTimes[i] = 0;
    }
}

// Insert the request on the correct request table.  Return true if
// the entry was already present.
RequestStatus
Sequencer::insertRequest(PacketPtr pkt, RubyRequestType request_type)
{
    assert(m_outstanding_count ==
        (m_writeRequestTable.size() + m_readRequestTable.size()
        #ifdef ASYNC_PISCS
        // Nicholas - Adding async PISCs
        + retryActiveVertexList.size()
        #endif
        ));

    // See if we should schedule a deadlock check
    if (!deadlockCheckEvent.scheduled() &&
        drainState() != DrainState::Draining) {
        schedule(deadlockCheckEvent, clockEdge(m_deadlock_threshold));
    }

    #ifdef ASYNC_PISCS
    // Nicholas - Adding async PISCs
    // Assuming that these high physical addresses are never used naturally
    assert(pkt->getAddr() < ASYNC_PISC_PADDR_START);
    #endif

    Addr line_addr = makeLineAddress(pkt->getAddr());
    
    //line adddress is the same as getAddr for scratchpad accesses
    if ((request_type == RubyRequestType_ST_Vertex_Local) ||
        (request_type == RubyRequestType_ST_Vertex_Remote) ||
        (request_type == RubyRequestType_Locked_Vertex_Read_Local) ||
        (request_type == RubyRequestType_Locked_Vertex_Read_Remote) ||
        (request_type == RubyRequestType_Locked_Vertex_Write_Local) ||
        (request_type == RubyRequestType_Locked_Vertex_Write_Remote) ||
        (request_type == RubyRequestType_LD_Vertex_Local) ||
        (request_type == RubyRequestType_PREFETCH_EDGE) ||
        (request_type == RubyRequestType_CHECK_ACTIVEVERTEX_Local) ||
        (request_type == RubyRequestType_CHECK_ACTIVEVERTEX_Remote) ||
        (request_type == RubyRequestType_LD_Vertex_Remote)) {
        //Abraham
        //Comment out the following line to check atomicity at a cache-line granularity
        line_addr = pkt->getAddr();
    }
    
    #ifdef ASYNC_PISCS
    // Nicholas - Adding async PISCs
    // Use an unused address to prevent "store waiting on store"
    if (pkt->getIsAtomic()) {
        assert(availablePISCPAddrs.size() > 0);
        line_addr = availablePISCPAddrs.back();
        assert(line_addr >= ASYNC_PISC_PADDR_START);
        availablePISCPAddrs.pop_back();

        // Update the PISC op packet's address
        pkt->setAddr(line_addr);
    }
    #endif

    // Create a default entry, mapping the address to NULL, the cast is
    // there to make gcc 4.4 happy
    RequestTable::value_type default_entry(line_addr,
                                           (SequencerRequest*) NULL);
   /* 
    //Abraham - attempting to fix deadlock issue based on https://groups.google.com/forum/#!topic/gem5-gpu-dev/RQv4SxIKv7g
    if (m_controller->isBlocked(line_addr) && 
        (request_type == RubyRequestType_IFETCH)) {
        return RequestStatus_Aliased;
    }
   */ 
    
    if ((request_type == RubyRequestType_ST) ||
        (request_type == RubyRequestType_ST_Vertex_Local) ||
        (request_type == RubyRequestType_ST_Vertex_Remote) ||
        (request_type == RubyRequestType_RMW_Read) ||
        (request_type == RubyRequestType_RMW_Write) ||
        (request_type == RubyRequestType_Load_Linked) ||
        (request_type == RubyRequestType_Store_Conditional) ||
        (request_type == RubyRequestType_Locked_RMW_Read) ||
        (request_type == RubyRequestType_Locked_Vertex_Read_Local) ||
        (request_type == RubyRequestType_Locked_Vertex_Read_Remote) ||
        (request_type == RubyRequestType_Locked_RMW_Write) ||
        (request_type == RubyRequestType_Locked_Vertex_Write_Local) ||
        (request_type == RubyRequestType_Locked_Vertex_Write_Remote) ||
        (request_type == RubyRequestType_PREFETCH_EDGE) ||
        (request_type == RubyRequestType_CHECK_ACTIVEVERTEX_Local) ||
        (request_type == RubyRequestType_CHECK_ACTIVEVERTEX_Remote) ||
        (request_type == RubyRequestType_FLUSH)) {

        // Check if there is any outstanding read request for the same
        // cache line.
        if (m_readRequestTable.count(line_addr) > 0) {
            m_store_waiting_on_load++;
            
            #ifdef ASYNC_PISCS
            // Nicholas - Adding async PISCs
            // Something has gone wrong if an async PISC op is forced to wait for a load
            assert(!pkt->getIsAtomic());
            #endif

            return RequestStatus_Aliased;
        }

        pair<RequestTable::iterator, bool> r =
            m_writeRequestTable.insert(default_entry);
        if (r.second) {
            RequestTable::iterator i = r.first;
            i->second = new SequencerRequest(pkt, request_type, curCycle());
            m_outstanding_count++;
        } else {
            // There is an outstanding write request for the cache line
            m_store_waiting_on_store++;

            #ifdef ASYNC_PISCS
            // Nicholas - Adding async PISCs
            // Something has gone wrong if an async PISC op is forced to wait for another store
            assert(!pkt->getIsAtomic());
            #endif

            return RequestStatus_Aliased;
        }
    } else {
        // Check if there is any outstanding write request for the same
        // cache line.
        if (m_writeRequestTable.count(line_addr) > 0) {
            m_load_waiting_on_store++;
            return RequestStatus_Aliased;
        }

        pair<RequestTable::iterator, bool> r =
            m_readRequestTable.insert(default_entry);

        if (r.second) {
            RequestTable::iterator i = r.first;
            i->second = new SequencerRequest(pkt, request_type, curCycle());
            m_outstanding_count++;
        } else {
            // There is an outstanding read request for the cache line
            m_load_waiting_on_load++;
            return RequestStatus_Aliased;
        }
    }

    m_outstandReqHist.sample(m_outstanding_count);
    assert(m_outstanding_count ==
        (m_writeRequestTable.size() + m_readRequestTable.size()
        #ifdef ASYNC_PISCS
        // Nicholas - Adding async PISCs
        + retryActiveVertexList.size()
        #endif
        ));
    
    DPRINTF(RubySequencer, "Request is ready vaddr :%x\n", line_addr);

    return RequestStatus_Ready;
}

void
Sequencer::markRemoved()
{
    m_outstanding_count--;
    assert(m_outstanding_count ==
        (m_writeRequestTable.size() + m_readRequestTable.size()
        #ifdef ASYNC_PISCS
        // Nicholas - Adding async PISCs
        + retryActiveVertexList.size()
        #endif
        ));
}

void
Sequencer::invalidateSC(Addr address)
{
    DPRINTF(RubySequencer, "InvalidateSC :%x\n", address);
    AbstractCacheEntry *e = m_dataCache_ptr->lookup(address);
    // The controller has lost the coherence permissions, hence the lock
    // on the cache line maintained by the cache should be cleared.
    if (e && e->isLocked(m_version)) {
        e->clearLocked();
    }
}

bool
Sequencer::handleLlsc(Addr address, SequencerRequest* request)
{
    DPRINTF(RubySequencer, "HandleLlsc is called:%x\n", address);
    AbstractCacheEntry *e = m_dataCache_ptr->lookup(address);
    if (!e)
        return true;

    // The success flag indicates whether the LLSC operation was successful.
    // LL ops will always succeed, but SC may fail if the cache line is no
    // longer locked.
    bool success = true;
    if (request->m_type == RubyRequestType_Store_Conditional) {
        DPRINTF(RubySequencer, "HandleLlsc Store_Conditional called:%x\n", address);
        if (!e->isLocked(m_version)) {
            //
            // For failed SC requests, indicate the failure to the cpu by
            // setting the extra data to zero.
            //
            request->pkt->req->setExtraData(0);
            success = false;
        } else {
            //
            // For successful SC requests, indicate the success to the cpu by
            // setting the extra data to one.
            //
            request->pkt->req->setExtraData(1);
        }
        //
        // Independent of success, all SC operations must clear the lock
        //
        e->clearLocked();
    } else if (request->m_type == RubyRequestType_Load_Linked) {
        DPRINTF(RubySequencer, "HandleLlsc  Load_Linked called:%x\n", address);
        //
        // Note: To fully follow Alpha LLSC semantics, should the LL clear any
        // previously locked cache lines?
        //
        e->setLocked(m_version);
    } else if (e->isLocked(m_version)) {
        DPRINTF(RubySequencer, "HandleLlsc  isLocked called:%x\n", address);
        //
        // Normal writes should clear the locked address
        //
        e->clearLocked();
    }
    return success;
}

void
Sequencer::recordMissLatency(const Cycles cycles, const RubyRequestType type,
                             const MachineType respondingMach,
                             bool isExternalHit, Cycles issuedTime,
                             Cycles initialRequestTime,
                             Cycles forwardRequestTime,
                             Cycles firstResponseTime, Cycles completionTime)
{
    m_latencyHist.sample(cycles);
    m_typeLatencyHist[type]->sample(cycles);

    if (isExternalHit) {
        m_missLatencyHist.sample(cycles);
        m_missTypeLatencyHist[type]->sample(cycles);

        if (respondingMach != MachineType_NUM) {
            m_missMachLatencyHist[respondingMach]->sample(cycles);
            m_missTypeMachLatencyHist[type][respondingMach]->sample(cycles);

            if ((issuedTime <= initialRequestTime) &&
                (initialRequestTime <= forwardRequestTime) &&
                (forwardRequestTime <= firstResponseTime) &&
                (firstResponseTime <= completionTime)) {

                m_IssueToInitialDelayHist[respondingMach]->sample(
                    initialRequestTime - issuedTime);
                m_InitialToForwardDelayHist[respondingMach]->sample(
                    forwardRequestTime - initialRequestTime);
                m_ForwardToFirstResponseDelayHist[respondingMach]->sample(
                    firstResponseTime - forwardRequestTime);
                m_FirstResponseToCompletionDelayHist[respondingMach]->sample(
                    completionTime - firstResponseTime);
            } else {
                m_IncompleteTimes[respondingMach]++;
            }
        }
    } else {
        m_hitLatencyHist.sample(cycles);
        m_hitTypeLatencyHist[type]->sample(cycles);

        if (respondingMach != MachineType_NUM) {
            m_hitMachLatencyHist[respondingMach]->sample(cycles);
            m_hitTypeMachLatencyHist[type][respondingMach]->sample(cycles);
        }
    }
}



void 
Sequencer::addReadOnlyCachedData(PacketPtr pkt, int contextId)
{
    
    uint8_t* dataPtr = new uint8_t[pkt->getSize()];
    memcpy(dataPtr,pkt->getConstPtr<uint8_t>(), pkt->getSize());

    Addr addr = pkt->getVaddr();

    // Nicholas - Switching to unordered_map for read-only cache data
    assert(system->readOnlyCachedData[contextId].find(addr) == system->readOnlyCachedData[contextId].end());
    system->readOnlyCachedData[contextId][addr] = dataPtr;
    /*
    system->readOnlyCachedData[contextId].push_back(std::make_pair(addr, dataPtr));
    */
    DPRINTF(RubySequencer, "Adding new cached data to read only cached vertex buffer: pkt value: %i\n", (int)*pkt->getConstPtr<uint8_t>());
    DPRINTF(RubySequencer, "Adding new cached data to read only cached vertex buffer: after copied to read only buffer data addd: %i\n", (int)*dataPtr);

}

uint8_t* 
Sequencer::lookupReadOnlyCachedData (PacketPtr pkt, int contextId) {
    if ((!system->readOnlyCachedData[contextId].empty())) {
            DPRINTF(RubySequencer, "Trying to read from read only cached vertex buffer\n");

            // Nicholas - Switching to unordered_map for read-only cache data
            auto i = system->readOnlyCachedData[contextId].find(pkt->getVaddr());
            if (i != system->readOnlyCachedData[contextId].end()) {
                DPRINTF(RubySequencer, "Reading cached data from read only cached vertex buffer: data addd: %i\n", *(int*)i->second);
                return i->second; 
            }
            /*
            for (auto i = system->readOnlyCachedData[contextId].begin(); i != system->readOnlyCachedData[contextId].end(); ++i) {
                
                if (i->first == pkt->getVaddr()) {
                    
                    DPRINTF(RubySequencer, "Reading cached data from read only cached vertex buffer: data addd: %i\n", *(int*)i->second);
                    return i->second; 
                }
            }
            */
    }

    return NULL;
}

//Abraham - handling nonpointer based datablock access
void
Sequencer::writeCallback(Addr address, const DataBlock& data,
                         const bool externalHit,
                         const int offset,
                         MachineID machID,
                         const MachineType mach,
                         const Cycles initialRequestTime,
                         const Cycles forwardRequestTime,
                         const Cycles firstResponseTime)
{

    writeCallback(address, const_cast<DataBlock&>(data), true, offset, machID);

}


void
Sequencer::writeCallback(Addr address, DataBlock& data,
                         const bool externalHit, 
                         const int offset,
                         MachineID machID,
                         const MachineType mach,
                         const Cycles initialRequestTime,
                         const Cycles forwardRequestTime,
                         const Cycles firstResponseTime)
{
    RequestTable::iterator i = m_writeRequestTable.find(address);
    assert(i != m_writeRequestTable.end());
    SequencerRequest* request = i->second;

    //Abraham - the folowing distiniction allows to block on word-level
    //address rather than cache-line-level address
    if((request->m_type == RubyRequestType_ST_Vertex_Local) ||
        (request->m_type == RubyRequestType_ST_Vertex_Remote) ||
        (request->m_type == RubyRequestType_Locked_Vertex_Read_Local) ||
        (request->m_type == RubyRequestType_Locked_Vertex_Read_Remote) ||
        (request->m_type == RubyRequestType_Locked_Vertex_Write_Local) ||
        (request->m_type == RubyRequestType_Locked_Vertex_Write_Remote) ||
        (request->m_type == RubyRequestType_CHECK_ACTIVEVERTEX_Local) ||
        (request->m_type == RubyRequestType_CHECK_ACTIVEVERTEX_Remote) ||
        (request->m_type == RubyRequestType_PREFETCH_EDGE)) {
        
        assert(m_writeRequestTable.count(address));
    
    }
    else {
        assert(address == makeLineAddress(address));
        assert(m_writeRequestTable.count(makeLineAddress(address)));
    }

    #ifdef ASYNC_PISCS
    // Nicholas - Adding async PISCs
    if (request->pkt->getIsAtomic()) {
        // Free the reserved address
        assert(address >= ASYNC_PISC_PADDR_START);
        availablePISCPAddrs.push_back(address);
        assert(availablePISCPAddrs.size() <= ASYNC_PISC_LIMIT);
    }
    #endif
    
    m_writeRequestTable.erase(i);
    markRemoved();

    assert((request->m_type == RubyRequestType_ST) ||
           (request->m_type == RubyRequestType_ST_Vertex_Local) ||
           (request->m_type == RubyRequestType_ST_Vertex_Remote) ||
           (request->m_type == RubyRequestType_ATOMIC) ||
           (request->m_type == RubyRequestType_RMW_Read) ||
           (request->m_type == RubyRequestType_RMW_Write) ||
           (request->m_type == RubyRequestType_Load_Linked) ||
           (request->m_type == RubyRequestType_Store_Conditional) ||
           (request->m_type == RubyRequestType_Locked_RMW_Read) ||
           (request->m_type == RubyRequestType_Locked_Vertex_Read_Local) ||
           (request->m_type == RubyRequestType_Locked_Vertex_Read_Remote) ||
           (request->m_type == RubyRequestType_Locked_RMW_Write) ||
           (request->m_type == RubyRequestType_Locked_Vertex_Write_Local) ||
           (request->m_type == RubyRequestType_Locked_Vertex_Write_Remote) ||
           (request->m_type == RubyRequestType_PREFETCH_EDGE) ||
           (request->m_type == RubyRequestType_CHECK_ACTIVEVERTEX_Local) ||
           (request->m_type == RubyRequestType_CHECK_ACTIVEVERTEX_Remote) ||
           (request->m_type == RubyRequestType_FLUSH));

    //
    // For Alpha, properly handle LL, SC, and write requests with respect to
    // locked cache blocks.
    //
    // Not valid for Network_test protocl
    //
    bool success = true;
    
    DPRINTF(RubySequencer, "From writeback Address %#x \n", address);
    //Abraham - the following lines don't get checked by in the !usingNetworkTester if clause
    if((request->m_type == RubyRequestType_ST_Vertex_Local) ||
        (request->m_type == RubyRequestType_ST_Vertex_Remote) ||
        (request->m_type == RubyRequestType_Locked_Vertex_Read_Local) ||
        (request->m_type == RubyRequestType_Locked_Vertex_Read_Remote) ||
        (request->m_type == RubyRequestType_Locked_Vertex_Write_Local) ||
        (request->m_type == RubyRequestType_Locked_Vertex_Write_Remote) ||
        (request->m_type == RubyRequestType_CHECK_ACTIVEVERTEX_Local) ||
        (request->m_type == RubyRequestType_CHECK_ACTIVEVERTEX_Remote) ||
        (request->m_type == RubyRequestType_PREFETCH_EDGE)) { 
       
       //do nothing
       success = true;
    
    }
    
    
    //if(!m_usingNetworkTester)
    else if (!m_usingNetworkTester)
        success = handleLlsc(address, request);

    if ((request->m_type == RubyRequestType_Locked_RMW_Read)) {
        m_controller->blockOnQueue(address, m_mandatory_q_ptr);
        DPRINTF(RubySequencer, "blockonqueue: block read Address %#x \n", address);
    }
   else if ((request->m_type == RubyRequestType_Locked_Vertex_Read_Local) ||
            (request->m_type == RubyRequestType_Locked_Vertex_Read_Remote)) {

   //else if ((request->m_type == RubyRequestType_Locked_Vertex_Read_Local)) {
        m_controller->blockOnQueue(address, m_mandatory_q_ptr);
        DPRINTF(RubySequencer, "blockonqueue: block vertex read local Address %#x \n", address);
        //m_controller->blockOnQueue(request->pkt->getAddr(), m_mandatory_q_ptr);
    }
    else if ((request->m_type == RubyRequestType_Locked_RMW_Write)){
        m_controller->unblock(address);
        DPRINTF(RubySequencer, "blockonqueue: unblock write Address %#x \n", address);
    } 
    else if ((request->m_type == RubyRequestType_Locked_Vertex_Write_Local) ||
               (request->m_type == RubyRequestType_Locked_Vertex_Write_Remote)) {

    //else if ((request->m_type == RubyRequestType_Locked_Vertex_Write_Local)) {
        m_controller->unblock(address);
        DPRINTF(RubySequencer, "blockonqueue: unblock vertex  write local Address %#x \n", address);
    }

    hitCallback(request, data, offset, machID, success, mach, externalHit,
                initialRequestTime, forwardRequestTime, firstResponseTime);
}

//Abraham - handling nonpointer based datablk access
void
Sequencer::readCallback(Addr address, const DataBlock& data,
                        bool externalHit,
                        const int offset, 
                        MachineID machID,
                        const MachineType mach,
                        Cycles initialRequestTime,
                        Cycles forwardRequestTime,
                        Cycles firstResponseTime)
{
    
    
    readCallback(address, const_cast<DataBlock&>(data), true, offset, machID);

}

void
Sequencer::readCallback(Addr address, DataBlock& data,
                        bool externalHit, 
                        const int offset, 
                        MachineID machID,
                        const MachineType mach,
                        Cycles initialRequestTime,
                        Cycles forwardRequestTime,
                        Cycles firstResponseTime)
{
 
    DPRINTF(RubySequencer, "readcallback Physical Address %#x\n", address);

    RequestTable::iterator i = m_readRequestTable.find(address);
    assert(i != m_readRequestTable.end());
    SequencerRequest* request = i->second;
    //Abraham - scratchpad mapped addresses are handled separetly
    if((request->m_type == RubyRequestType_LD_Vertex_Local) ||
        (request->m_type == RubyRequestType_LD_Vertex_Remote)) {
        
        assert(m_readRequestTable.count(address));
    }
    else {
        assert(address == makeLineAddress(address));
        assert(m_readRequestTable.count(makeLineAddress(address)));
    }
   
    m_readRequestTable.erase(i);
    markRemoved();


    assert((request->m_type == RubyRequestType_LD) ||
           (request->m_type == RubyRequestType_LD_Vertex_Local) ||
           (request->m_type == RubyRequestType_LD_Vertex_Remote) ||
           (request->m_type == RubyRequestType_IFETCH));

    hitCallback(request, data, offset, machID, true, mach, externalHit,
                initialRequestTime, forwardRequestTime, firstResponseTime);
}

void
Sequencer::hitCallback(SequencerRequest* srequest, DataBlock& data,
                       int offset,
                       MachineID machID,
                       bool llscSuccess,
                       const MachineType mach, const bool externalHit,
                       const Cycles initialRequestTime,
                       const Cycles forwardRequestTime,
                       const Cycles firstResponseTime)
{
    warn_once("Replacement policy updates recently became the responsibility "
              "of SLICC state machines. Make sure to setMRU() near callbacks "
              "in .sm files!");

    PacketPtr pkt = srequest->pkt;
    Addr request_address(pkt->getAddr());
    RubyRequestType type = srequest->m_type;
    Cycles issued_time = srequest->issue_time;

    assert(curCycle() >= issued_time);
    Cycles total_latency = curCycle() - issued_time;

    // Profile the latency for all demand accesses.
    recordMissLatency(total_latency, type, mach, externalHit, issued_time,
                      initialRequestTime, forwardRequestTime,
                      firstResponseTime, curCycle());

    DPRINTFR(ProtocolTrace, "%15s %3s %10s%20s %6s>%-6s %#x %d cycles\n",
             curTick(), m_version, "Seq",
             llscSuccess ? "Done" : "SC_Failed", "", "",
             printAddress(request_address), total_latency);
   
    // Nicholas - Adding memory access tracking
    // Set the packet's memory access type
    switch (srequest->m_type) {
    // Will need to handle atomics, PISC atomics, memory-mapped regs, flushes, etc
    case RubyRequestType_LD:
    case RubyRequestType_RMW_Read:  // Treating non-locked RMW reads the same as normal loads
        if (externalHit) {
            // Add extra tracking to differentiate between L2 and RAM?
            // Note: pkt->req->getAccessDepth() is not accurate (always 0)
            pkt->req->setNWMemoryAccessType(NWMemAccessType_LD_L2_RAM);
        } else {
            pkt->req->setNWMemoryAccessType(NWMemAccessType_LD_L1);
        }
        break;
    case RubyRequestType_LD_Vertex_Local:
        // Will need to diffentiate between SPM and source buffer?
        pkt->req->setNWMemoryAccessType(NWMemAccessType_LD_SPM_Local);
        break;
    case RubyRequestType_LD_Vertex_Remote:
        // Will need to diffentiate between SPM and source buffer
        pkt->req->setNWMemoryAccessType(NWMemAccessType_LD_SPM_Remote);
        break;
    case RubyRequestType_Locked_RMW_Read:
        if (externalHit) {
            // Add extra tracking to differentiate between L2 and RAM?
            // Note: pkt->req->getAccessDepth() is not accurate (always 0)
            pkt->req->setNWMemoryAccessType(NWMemAccessType_LD_LOCKED_RMW_L2_RAM);
        } else {
            pkt->req->setNWMemoryAccessType(NWMemAccessType_LD_LOCKED_RMW_L1);
        }
        break;

    case RubyRequestType_ST:
        if (externalHit) {
            // Add extra tracking to differentiate between L2 and RAM?
            // Note: pkt->req->getAccessDepth() is not accurate (always 0)
            switch (pkt->req->getNWMemoryAccessType()) {
            case NWMemAccessType_ST_MMIO_PISC_Local:
                pkt->req->setNWMemoryAccessType(NWMemAccessType_ST_MMIO_PISC_Local_Sparse_L2_RAM);
                break;
            case NWMemAccessType_ST_MMIO_PISC_Remote:
                pkt->req->setNWMemoryAccessType(NWMemAccessType_ST_MMIO_PISC_Remote_Sparse_L2_RAM);
                break;
            default:
                pkt->req->setNWMemoryAccessType(NWMemAccessType_ST_L2_RAM);
                break;
            }
        } else {
            switch (pkt->req->getNWMemoryAccessType()) {
            case NWMemAccessType_ST_MMIO_PISC_Local:
                pkt->req->setNWMemoryAccessType(NWMemAccessType_ST_MMIO_PISC_Local_Sparse_L1);
                break;
            case NWMemAccessType_ST_MMIO_PISC_Remote:
                pkt->req->setNWMemoryAccessType(NWMemAccessType_ST_MMIO_PISC_Remote_Sparse_L1);
                break;
            default:
                pkt->req->setNWMemoryAccessType(NWMemAccessType_ST_L1);
                break;
            }
        }
        break;
    case RubyRequestType_ST_Vertex_Local:
        // Will need to diffentiate between SPM and source buffer?
        if (pkt->getIsAtomic()) {
            pkt->req->setNWMemoryAccessType(NWMemAccessType_ST_MMIO_PISC_Local);
        } else {
            pkt->req->setNWMemoryAccessType(NWMemAccessType_ST_SPM_Local);
        }
        break;
    case RubyRequestType_ST_Vertex_Remote:
        // Will need to diffentiate between SPM and source buffer?
        if (pkt->getIsAtomic()) {
            pkt->req->setNWMemoryAccessType(NWMemAccessType_ST_MMIO_PISC_Remote);
        } else {
            pkt->req->setNWMemoryAccessType(NWMemAccessType_ST_SPM_Remote);
        }
        break;
    case RubyRequestType_Locked_RMW_Write:
        if (externalHit) {
            // Add extra tracking to differentiate between L2 and RAM?
            // Note: pkt->req->getAccessDepth() is not accurate (always 0)
            pkt->req->setNWMemoryAccessType(NWMemAccessType_ST_LOCKED_RMW_L2_RAM);
        } else {
            pkt->req->setNWMemoryAccessType(NWMemAccessType_ST_LOCKED_RMW_L1);
        }
        break;

    case RubyRequestType_IFETCH:
        if (externalHit) {
            // Add extra tracking to differentiate between L2 and RAM?
            // Note: pkt->req->getAccessDepth() is not accurate (always 0)
            pkt->req->setNWMemoryAccessType(NWMemAccessType_IF_L2_RAM);
        } else {
            pkt->req->setNWMemoryAccessType(NWMemAccessType_IF_L1);
        }
        break;

    case RubyRequestType_CHECK_ACTIVEVERTEX_Local:
        assert(pkt->isRead() || pkt->isWrite());
        if (pkt->isRead()) {
            pkt->req->setNWMemoryAccessType(NWMemAccessType_LD_Active_Local);
        } else {
            pkt->req->setNWMemoryAccessType(NWMemAccessType_ST_Active_Local);
        }
        break;
    case RubyRequestType_CHECK_ACTIVEVERTEX_Remote:
        assert(pkt->isRead() || pkt->isWrite());
        if (pkt->isRead()) {
            pkt->req->setNWMemoryAccessType(NWMemAccessType_LD_Active_Remote);
        } else {
            pkt->req->setNWMemoryAccessType(NWMemAccessType_ST_Active_Remote);
        }
        break;

    default:
        pkt->req->setNWMemoryAccessType(NWMemAccessType_NULL);
        unhandledCounts[srequest->m_type]++;
        break;
    }

    if (type == RubyRequestType_PREFETCH_EDGE) { //prefetch command does not have data
        DPRINTF(RubySequencer, "Prefetch VAddress %#x , Physical Address %#x Value %#x\n", pkt->getVaddr(), pkt->getAddr(),(uint64_t)*(pkt->getConstPtr<uint64_t>()));
    }
    
    
    // update the data unless it is a non-data-carrying flush
    if (RubySystem::getWarmupEnabled()) {
        data.setData(pkt->getConstPtr<uint8_t>(),
                     getOffset(request_address), pkt->getSize());
    } else if((!pkt->getIsAtomic()) &&  (!pkt->isFlush()) && (type != RubyRequestType_ST_Vertex_Remote) && 
               (type != RubyRequestType_Locked_Vertex_Write_Remote) && //already handled at the remote spd
               (type != RubyRequestType_CHECK_ACTIVEVERTEX_Local) &&
               (type != RubyRequestType_CHECK_ACTIVEVERTEX_Remote) &&
               (type != RubyRequestType_PREFETCH_EDGE)) { //prefetch command does not have data
        DPRINTF(RubySequencer, "Here VAddress %#x , Physical Address %#x Value %#x\n", pkt->getVaddr(), pkt->getAddr(),(uint64_t)*(pkt->getConstPtr<uint64_t>()));
        
        if ((type == RubyRequestType_LD_Vertex_Local) || //Read values
            (type == RubyRequestType_LD_Vertex_Remote) ||
            (type == RubyRequestType_Locked_Vertex_Read_Local) ||
            (type == RubyRequestType_Locked_Vertex_Read_Remote)){
          
            if ((type == RubyRequestType_LD_Vertex_Local) || //Read values
                (type == RubyRequestType_Locked_Vertex_Read_Local)) {
                //Abraham - 8 is the assumed maximum word length  
                memcpy(pkt->getPtr<uint8_t>(),
                       data.getData(offset*8, pkt->getSize()),
                       pkt->getSize());
                DPRINTF(RubySequencer, "vertex local read data %s vaddr %#x\n", data, pkt->getVaddr());
            }
            else if ((type == RubyRequestType_LD_Vertex_Remote)) {
                //Abraham - 8 is the assumed maximum word length  
                memcpy(pkt->getPtr<uint8_t>(),
                    data.getData(offset*8, pkt->getSize()),
                       pkt->getSize());
                DPRINTF(RubySequencer, "vertex remote read data %s vaddr %#x\n", data, pkt->getVaddr());
            }
            else if ((type == RubyRequestType_Locked_Vertex_Read_Remote)) {
                //Abraham - 8 is the assumed maximum word length  
                memcpy(pkt->getPtr<uint8_t>(),
                   data.getData(offset*8, pkt->getSize()),
                   pkt->getSize());
                DPRINTF(RubySequencer, "vertex remote locked read data %s vaddr %#x\n", data, pkt->getVaddr());
            }
        
            //Make sure the following code executed after the data is copied to the pkt 
            //copy the value  
            //Abraham to make BellmanFord happy 
            //need to fix the size requirement and check if new apps might complain
            DPRINTF(RubySequencer, " before checking enable cached vertex write data\n");
            //if(enableVertexCaching) {
            if(system->enableAtomic == 1) {
                
                ctx = system->getThreadContext(machID.num);
                system->validVCaching[ctx->contextId()] = true;
                addReadOnlyCachedData(pkt, ctx->contextId());
                DPRINTF(RubySequencer, "After writing cached vertex write data\n");
            } 
            
        }
        else if ((type == RubyRequestType_LD) ||
            (type == RubyRequestType_IFETCH) ||
            (type == RubyRequestType_RMW_Read) ||
            (type == RubyRequestType_Locked_RMW_Read) ||
            (type == RubyRequestType_Load_Linked)) {
            memcpy(pkt->getPtr<uint8_t>(),
                   data.getData(getOffset(request_address), pkt->getSize()),
                   pkt->getSize());
            DPRINTF(RubySequencer, "read data %s\n", data);
        }
        //Abraham - note that RubyRequestType_Locked_Vertex_Write_Remote" is handled inside the protocol/scratchapd
        else if ((type == RubyRequestType_ST_Vertex_Local) || 
                (type == RubyRequestType_Locked_Vertex_Write_Local)) {
            //Abraham - 8 bytes is the assumed maximum word length  
            DPRINTF(RubySequencer, "vertex local set begin\n");
            data.setData(pkt->getConstPtr<uint8_t>(),
                         offset*8, pkt->getSize());
            DPRINTF(RubySequencer, "vertex local set data %s\n", data);

        } else {
            data.setData(pkt->getConstPtr<uint8_t>(),
                         getOffset(request_address), pkt->getSize());
            DPRINTF(RubySequencer, "set data %s\n", data);
        }
    }

    // If using the RubyTester, update the RubyTester sender state's
    // subBlock with the recieved data.  The tester will later access
    // this state.
    if (m_usingRubyTester) {
        DPRINTF(RubySequencer, "hitCallback %s 0x%x using RubyTester\n",
                pkt->cmdString(), pkt->getAddr());
        RubyTester::SenderState* testerSenderState =
            pkt->findNextSenderState<RubyTester::SenderState>();
        assert(testerSenderState);
        testerSenderState->subBlock.mergeFrom(data);
    }

    delete srequest;

    RubySystem *rs = m_ruby_system;
    
    if (RubySystem::getWarmupEnabled()) {
        assert(pkt->req);
        delete pkt->req;
        delete pkt;
        rs->m_cache_recorder->enqueueNextFetchRequest();
    } else if (RubySystem::getCooldownEnabled()) {
        delete pkt;
        rs->m_cache_recorder->enqueueNextFlushRequest();
    } else {
        //Abraham create new request to update the next active vertex list
        //if (((type == RubyRequestType_CHECK_ACTIVEVERTEX_Local) || 
            //(type == RubyRequestType_CHECK_ACTIVEVERTEX_Remote)) &&
        if (!pkt->getIsActiveListHandled() && ((pkt->getIsAtomic() && pkt->getIsSparse()) ||
            (((type == RubyRequestType_CHECK_ACTIVEVERTEX_Local) || 
            (type == RubyRequestType_CHECK_ACTIVEVERTEX_Remote)) &&
            pkt->getIsCopy()))){
            
            DPRINTF(RubySequencer, "Got hitcallback for check_activevertex Here VAddress %#x , Physical Address %#x Value %#x size of pkt %i bool val :%i\n", pkt->getVaddr(), pkt->getAddr(),(uint64_t)*(pkt->getConstPtr<uint64_t>()), pkt->getSize(), (uint8_t)*(pkt->getConstPtr<uint8_t>()) );
            
            
            if (pkt->getIsAtomic() && pkt->getIsSparse()) {
                DPRINTF(RubySequencer, "Before changing addresses curr vaddr: %#x paddr %#x\n",  pkt->getVaddr(), pkt->getAddr());
                DPRINTF(RubySequencer, "Before changing addresses getEdgeAddr: %#x\n",  pkt->getEdgeVaddr());
                
                pkt->setVaddr(pkt->getEdgeVaddr());
                pkt->req->setVaddr(pkt->getEdgeVaddr());
                ///pkt->setAddr(pkt->getEdgePaddr());
                
                //Need to translate virtual to physical address
                ctx = system->getThreadContext(pkt->req->contextId());
                Fault fault = ctx->getDTBPtr()->translateAtomic(pkt->req, ctx, BaseTLB::Write);
                assert(fault == NoFault);

                DPRINTF(RubySequencer, "After changing addresses of req curr vaddr: %#x paddr %#x \n",  pkt->req->getVaddr(), pkt->req->getPaddr());
                pkt->setAddr(pkt->req->getPaddr());
            }
                
            DPRINTF(RubySequencer, "After changing addresses of pkt curr vaddr: %#x paddr %#x \n",  pkt->getVaddr(), pkt->getAddr());
           
            pkt->setIsActiveListHandled(true);
            pkt->setIsAtomic(false);
            writeActiveVertex(pkt); 
    
 
        } 
        else  { 

            #ifdef ASYNC_PISCS
            // Nicholas - Adding async PISCs
            // At this point, a PISC op packet has either "isAtomic" or "isActiveListHandled" true
            if (pkt->getIsAtomic() || pkt->getIsActiveListHandled()) {
                // Delete the async PISC op's request and packet
                //delete pkt->req;  // Depending on pkt's config, deleting it will delete req
                delete pkt;

                // PISC op complete, decrement the counter
                assert(system->outstandingPISCOps > 0);
                assert(localOutstandingPISCOps > 0);
                system->outstandingPISCOps--;
                localOutstandingPISCOps--;

                // Since the async PISC op has been handled, a slot has opened for a new request.
                // Have stalled MemSlavePorts try submitting a request again
                // Note: This process is copied from RubyPort::ruby_hit_callback()
                if (!retryList.empty()) {
                    std::vector<MemSlavePort *> curRetryList(retryList);
                    retryList.clear();
                    for (auto i = curRetryList.begin(); i != curRetryList.end(); ++i) {
                        (*i)->sendRetryReq();
                    }
                }
            } else {
                // Handle non-PISC packets normally
                ruby_hit_callback(pkt);
            }
            #else // #ifdef ASYNC_PISCS

            ruby_hit_callback(pkt);

            #endif // #ifdef ASYNC_PISCS

            // Abraham if we had to stall the pkt request to the vertex active list, try sending them now because the sequencer
            // likely has free resources now.
            //if ((isSparse) && (!retryActiveVertexList.empty())) {
            if ((!retryActiveVertexList.empty())) {
                //
                // Record the current list of ports to retry on a temporary list before
                // calling sendRetry on those ports.  sendRetry will cause an
                // immediate retry, which may result in the ports being put back on the
                // list. Therefore we want to clear the retryList before calling
                // sendRetry.
                //
                DPRINTF(RubySequencer, "Trying to write active vertex from retry list\n");
                std::vector<PacketPtr> curRetryActiveVertexList(retryActiveVertexList);

                #ifdef ASYNC_PISCS
                // Nicholas - Adding async PISCs
                // Count retryActiveVertexList entries as outstanding requests
                // Removing the list's contribution since the subsequent calls to "addToRetryActiveVertexList"
                // may add packets back to the list and increment m_outstanding_count
                m_outstanding_count -= retryActiveVertexList.size();
                assert(m_outstanding_count >= 0);
                #endif

                retryActiveVertexList.clear();

                for (auto i = curRetryActiveVertexList.begin(); i != curRetryActiveVertexList.end(); ++i) {
                    writeActiveVertex(*i);
                }
            }
        }
    }
}


//Abraham
//Make the packet to write to cache the next active list
void
Sequencer::writeActiveVertex(PacketPtr pkt) 
{
    DPRINTF(RubySequencer, "Issuing write active vertex\n");

    RequestStatus requestStatus = makeRequest(pkt);

    if (requestStatus == RequestStatus_Issued) {
        // Save the port in the sender state object to be used later to
        // route the response
        //pkt->pushSenderState(new SenderState(this));

        DPRINTF(RubySequencer, "Request %s vaddr 0x%x paddr 0x%x issued\n", pkt->cmdString(),
                pkt->getVaddr(), pkt->getAddr());
        DPRINTF(RubySequencer, "After issuing Writing to sparse vertex access data %i \n",  *(unsigned int *)(pkt->getConstPtr<uint8_t>()));
    }
    else {
        DPRINTF(RubySequencer, "Adding write active vertex to retry list\n");
        addToRetryActiveVertexList(pkt);
    }

}


bool
Sequencer::empty() const
{
    return m_writeRequestTable.empty() && m_readRequestTable.empty();
}

RequestStatus
Sequencer::makeRequest(PacketPtr pkt)
{
    //Abraham - copying configuration parameters from the application
    DPRINTF(RubySequencer, "VAddress %#x , PAddress %#x ,Value %#x\n", pkt->getVaddr(), pkt->getAddr(),(uint64_t)*(pkt->getConstPtr<uint64_t>()));
    //DPRINTF(RubySequencer, "Global variable setting %ix\n", system->atomicConfig());
    //int numSpdSets = m_vertexCache_ptr->getNumSets();
    //int numLinesMapped =  (system->numVertex > numSpdSets*m_numCPU) ? numSpdSets*m_numCPU : system->numVertex;
    //DPRINTF(RubySequencer, "Details of spec. system->numVertex: %i numSpdSets: %i numLinesMapped: %i system->stride1: %i  stride2: %i system->dataTypeSize1: %i system->dataTypeSize2: %i system->dataTypeSize3: %i\n", 
     //       system->numVertex, numSpdSets, numLinesMapped, system->stride1, stride2, system->dataTypeSize1, system->dataTypeSize2, system->dataTypeSize3);
    DPRINTF(RubySequencer, "Adddr current pkt: %#x stored addr1: %#x addr2 %#x addr3: %#x addr3_old: %#x\n", pkt->getVaddr(), system->vertexStartAddr1, system->vertexStartAddr2, system->vertexStartAddr3, system->vertexStartAddr3_old);

    //satisfied is true if there is no need to access the scratchpads
    bool satisfied = false;
    
    //Atomic instruction execution and reading flag 
    bool atomicExec = false;

    if(pkt->isWrite()) { 
        if (pkt->getVaddr() == configVertexStartAddr1) { 
            system->vertexStartAddr1 = (uint64_t)*(pkt->getConstPtr<uint64_t>()); 
            satisfied = true;
        }
        else if (pkt->getVaddr() == configVertexStartAddr2) { 
            system->vertexStartAddr2 = (uint64_t)*(pkt->getConstPtr<uint64_t>()); 
            satisfied = true;
        }
        else if (pkt->getVaddr() == configVertexStartAddr3) { 
            system->vertexStartAddr3 = (uint64_t)*(pkt->getConstPtr<uint64_t>());
            satisfied = true;
        }
        else if (pkt->getVaddr() == configVertexStartAddr4) { 
            system->vertexStartAddr4 = (uint64_t)*(pkt->getConstPtr<uint64_t>()); 
            satisfied = true;
        }
        else if (pkt->getVaddr() == configDataTypeSize1) { 
            system->dataTypeSize1 = (uint64_t)*(pkt->getConstPtr<uint64_t>()); 
            satisfied = true;
        }
        else if (pkt->getVaddr() == configDataTypeSize2) { 
            system->dataTypeSize2 = (uint64_t)*(pkt->getConstPtr<uint64_t>()); 
            satisfied = true;
        }
        else if (pkt->getVaddr() == configDataTypeSize3) { 
            system->dataTypeSize3 = (uint64_t)*(pkt->getConstPtr<uint64_t>()); 
            satisfied = true;
        }
        else if (pkt->getVaddr() == configDataTypeSize4) { 
            system->dataTypeSize4 = (uint64_t)*(pkt->getConstPtr<uint64_t>()); 
            satisfied = true;
        }
        else if (pkt->getVaddr() == configNumVertex) { 
            system->numVertex = (int)*(pkt->getConstPtr<uint64_t>()); 
            satisfied = true;
        }
        else if (pkt->getVaddr() == configNumOutEdge) { 
            system->numOutEdges = (int)*(pkt->getConstPtr<uint64_t>()); 
            satisfied = true;
        }
        else if (pkt->getVaddr() == configNumTrackedAtomic) { 
            system->numTrackedAtomic = (unsigned )*(pkt->getConstPtr<uint64_t>()); 
            satisfied = true;
        }
        else if (pkt->getVaddr() == configEnableTrackedAddr1) { 
            system->enableTrackedAddr1 = (int)*(pkt->getConstPtr<uint64_t>()); 
            satisfied = true;
        }
        else if (pkt->getVaddr() == configEnableTrackedAddr2) { 
            system->enableTrackedAddr2 = (int)*(pkt->getConstPtr<uint64_t>()); 
            satisfied = true;
        }
        else if (pkt->getVaddr() == configEnableTrackedAddr3) { 
            int tmp = (int)*(pkt->getConstPtr<uint64_t>()); 
           
            if ((system->atomicOppType1 !='4') && (system->atomicOppType1 !='7') &&
                (system->atomicOppType1 !='a') && (system->atomicOppType1 !='b') &&
                (system->atomicOppType1 !='c')) { //BC and KCore  and GraphMat's implementation don't require tracking old addresses 
                if(( tmp == 0)  && (!system->isSparse)) {
                    system->enableTrackedAddr3_old = system->enableTrackedAddr3;
                    system->dataTypeSize3_old = system->dataTypeSize3;
                    system->vertexStartAddr3_old = system->vertexStartAddr3;
                    system->stride3_old = system->stride3;

                    // Nicholas - Disabled iteration counting to avoid its effects on dense active lists
                    //system->iterCount++; 
                    
                    DPRINTF(RubySequencer, " iter count inc. %i\n", system->iterCount);
                }
                
                else if ((tmp ==0) && (system->isSparse)){
                    system->enableTrackedAddr3_old = 0;
                }
            
            }
            
            system->enableTrackedAddr3 = tmp;
            satisfied = true;
        }
        
        else if (pkt->getVaddr() == configEnableTrackedAddr4) { 
            system->enableTrackedAddr4 = (int)*(pkt->getConstPtr<uint64_t>()); 
            satisfied = true;
        }
        else if (pkt->getVaddr() == configStride1) { 
            system->stride1 = (int)*(pkt->getConstPtr<uint64_t>()); 
            satisfied = true;
        }
        else if (pkt->getVaddr() == configStride2) { 
            system->stride2 = (int)*(pkt->getConstPtr<uint64_t>()); 
            satisfied = true;
        }
        else if (pkt->getVaddr() == configStride3) { 
            system->stride3 = (int)*(pkt->getConstPtr<uint64_t>()); 
            satisfied = true;
        }
        else if (pkt->getVaddr() == configStride4) { 
            system->stride4 = (int)*(pkt->getConstPtr<uint64_t>()); 
            satisfied = true;
        }
        else if (pkt->getVaddr() == configEnablePrefetch) { 
            enablePrefetch = (int)*(pkt->getConstPtr<uint64_t>()); 
            satisfied = true;
        }
        else if (pkt->getVaddr() == configIsSparse) { 
            system->isSparse = ((unsigned )*(pkt->getConstPtr<uint64_t>()) == 1); 
            satisfied = true;
        }
        
        else if (pkt->getVaddr() == configIsCopy) { 
            system->isCopy = ((unsigned )*(pkt->getConstPtr<uint64_t>()) == 1); 
            satisfied = true;
        }
        else if (pkt->getVaddr() == configPrefetchAddr) { 
            
            if(enablePrefetch == 1) {
                //Need to translate virtual to physical address
                ctx = system->getThreadContext(pkt->req->contextId());
    
                //0x00000300 means valid size and data
                //@todo change the constat size 8 
                Addr vAddr = (uint64_t)*(pkt->getConstPtr<uint64_t>());
                Request *req = new Request(0, vAddr, 8, 0x0000003,
                        ctx->getCpuPtr()->cpuId(), 0x0, ctx->contextId(),
                        ctx->threadId());
                req->taskId(ctx->getCpuPtr()->taskId());

                DPRINTF(RubySequencer, "TLB vaddr: %#x\n", vAddr);
                // translate to physical address
                Fault fault = ctx->getDTBPtr()->translateAtomic(req, ctx, BaseTLB::Write);
                assert(fault == NoFault);
                system->prefetchAddr = makeLineAddress(req->getPaddr()); 
                DPRINTF(RubySequencer, "TLB paddr: %#x\n", system->prefetchAddr);
                delete req;
            }
            satisfied = true;
        }
        else if ((pkt->getVaddr() == configPrefetchSize)) { 
            //the incoming size is interms of number of edges 
            //So it need to be converted into cache line 
            //In addition, the prefetching need to start from after 64 bytes
            //the edge data structure unless for weighted graph, then
            //it should include the byts for the weights too
            //@todo: change constants to be reconfigurable
            int edgeDataTypeSize = 4; 
            //prefetchSize = (((int)*(pkt->getConstPtr<uint64_t>()) - 64/edgeDataTypeSize)*edgeDataTypeSize)/64; 
            prefetchSize = (((int)*(pkt->getConstPtr<uint64_t>()))*edgeDataTypeSize)/RubySystem::getBlockSizeBytes();
           
            //if prefetching is enabled, then continue to prefetch without early return
            if(enablePrefetch !=1)
                satisfied = true;
        }
        
        //For PISC implementation 
        else if ((pkt->getVaddr() == configOldValueAddr1)) { 
          system->atomicOldValue1 = (uint64_t)*(pkt->getConstPtr<uint64_t>()); 
            satisfied = true;
            
        }
        else if ((pkt->getVaddr() == configOldValueAddr2)) { 
            system->atomicOldValue2 = (uint64_t)*(pkt->getConstPtr<uint64_t>()); 
            satisfied = true;
        }
        else if ((pkt->getVaddr() == configEnableAtomicAddr)) { 
            system->enableAtomic = (unsigned int)*(pkt->getConstPtr<uint64_t>()); 
            
            if(system->enableAtomic == 1)
            {
                system->active_vertices_total = 0;
                std::fill(system->active_vertices_per_spm.begin(), system->active_vertices_per_spm.end(), 0);
                
                std::fill(system->enableVCaching.begin(), system->enableVCaching.end(), true);
                DPRINTF(RubySequencer, "Setting enable vertex caching \n");
           }
            else { 
                
                std::fill(system->enableVCaching.begin(), system->enableVCaching.end(), false);
                std::fill(system->validVCaching.begin(), system->validVCaching.end(), false);
                
                system->invalidateReadOnlyCachedData();
                
                DPRINTF(RubySequencer, "Resetting enable vertex caching \n");
            }
            satisfied = true;
        }
        else if ((pkt->getVaddr() == configAtomicOppTypeAddr1)) { 
            system->atomicOppType1 = (uint8_t)*(pkt->getConstPtr<uint8_t>()); 
            satisfied = true;
        }
        else if ((pkt->getVaddr() == configAtomicOppTypeAddr2)) { 
            system->atomicOppType2 = (uint8_t)*(pkt->getConstPtr<uint8_t>()); 
            satisfied = true;
        }
        else if ((pkt->getVaddr() == configNoStatusReadAddr)) { 
            system->noStatusRead = ((uint64_t)*(pkt->getConstPtr<uint64_t>()) == 1) ? true : false; 
            satisfied = true;
        }
        
        else if ((pkt->getVaddr() == configSrcInfo)) { 
            for(int i =0; i< pkt->getSize(); i++) srcInfo[i] = (uint8_t)*(pkt->getConstPtr<uint8_t>() + i); 
            satisfied = true;
        }
        else if ((pkt->getVaddr() == configAddInfo)) { 
            addInfo = *(int*)(pkt->getConstPtr<uint8_t>()); 
            satisfied = true;
       } 
       else if ((pkt->getVaddr() == configEdgeIndex)) { 
            edgeIndex = *(unsigned int*)(pkt->getConstPtr<uint8_t>()); 
            satisfied = true;
       } 
       else if ((pkt->getVaddr() == configChunkSize)) { 
            system->chunkSize = *(uint64_t*)(pkt->getConstPtr<uint8_t>()); 
            satisfied = true;
       }
       // Nicholas - Adding PISC support for Graphit-supported CAS operations
       else if ((pkt->getVaddr() == configCasCompareInfo)) {
            for(int i =0; i< pkt->getSize(); i++) casCompareInfo[i] = (uint8_t)*(pkt->getConstPtr<uint8_t>() + i); 
            satisfied = true;
       }
       // Nicholas - Adding optional hardware deduplication
       else if ((pkt->getVaddr() == configDedupEnabled)) {
           system->dedupEnabled = ((unsigned )*(pkt->getConstPtr<uint64_t>()) == 1);
           satisfied = true;
       }
       // Nicholas - Adding profiling interface
       else if ((pkt->getVaddr() == configClearProfile)) {
           uint64_t profileIndex = *(uint64_t*)(pkt->getConstPtr<uint8_t>());

           assert(profileIndex >= 0 && profileIndex < NW_PROFILE_COUNT);
           assert(system->nwProfiles[profileIndex].running == false);
           system->nwProfiles[profileIndex].clear();

           //DPRINTF(NWTiming, "$_PROFILE_$ Cleared Profile %i\n", *(uint64_t*)(pkt->getConstPtr<uint8_t>()));
           satisfied = true;
       }
       else if ((pkt->getVaddr() == configStartProfile)) {
           uint64_t profileIndex = *(uint64_t*)(pkt->getConstPtr<uint8_t>());

           assert(profileIndex >= 0 && profileIndex < NW_PROFILE_COUNT);
           assert(system->nwProfiles[profileIndex].running == false);
           system->nwProfiles[profileIndex].start();

           //DPRINTF(NWTiming, "$_PROFILE_$ Started Timer %i\n", *(uint64_t*)(pkt->getConstPtr<uint8_t>()));
           satisfied = true;
       }
       else if ((pkt->getVaddr() == configStopProfile)) {
           uint64_t profileIndex = *(uint64_t*)(pkt->getConstPtr<uint8_t>());

           assert(profileIndex >= 0 && profileIndex < NW_PROFILE_COUNT);
           assert(system->nwProfiles[profileIndex].running == true);
           system->nwProfiles[profileIndex].stop();

           //DPRINTF(NWTiming, "$_PROFILE_$ Stopped Timer %i\n", *(uint64_t*)(pkt->getConstPtr<uint8_t>()));
           satisfied = true;
       }
       else if ((pkt->getVaddr() == configDumpProfile)) {
           uint64_t profileIndex = *(uint64_t*)(pkt->getConstPtr<uint8_t>());

           assert(profileIndex >= 0 && profileIndex < NW_PROFILE_COUNT);
           assert(system->nwProfiles[profileIndex].running == false);

           DPRINTF(NWTiming, "$_PROFILE_$ Dump Profile %i : %i ticks ; %i starts\n%s\n",
             *(uint64_t*)(pkt->getConstPtr<uint8_t>()),
             system->nwProfiles[profileIndex].totalTicks,
             system->nwProfiles[profileIndex].startCount,
             system->nwProfiles[profileIndex].getMemAccessString()
           );
           
           // Nicholas - Temporary, print total counts for unhandled RubyRequestTypes
           ////////////////
           std::string temp = "$_$ Unhandled (NULL) totals breakdown:\n";
           for (int i = 0; i < RubyRequestType_NUM; i++) {
             if (unhandledCounts[i] != 0) {
               temp += "$_$ " + RubyRequestType_to_string(static_cast<RubyRequestType>(i)) + ": " + std::to_string(unhandledCounts[i]) + "\n";
             }
           }
           DPRINTF(NWTiming, "$_PROFILE_$ \n%s\n", temp);
           ////////////////

           //DPRINTF(NWTiming, "$_PROFILE_$ Dump Timer %i\n", *(uint64_t*)(pkt->getConstPtr<uint8_t>()));
           satisfied = true;
       }
       //////////////////////////////////////////////////////////////
       else if ((pkt->getVaddr() == configRangeStartProfile0)) {
           system->profileRangeStartAddr[0] = (uint64_t)*(pkt->getConstPtr<uint64_t>()); 
           //DPRINTF(NWTiming, "$_PROFILE_$ Range0 start addr= 0x%x\n", system->profileRangeStartAddr[0]);
           satisfied = true;
       }
       else if ((pkt->getVaddr() == configRangeBytesProfile0)) {
           system->profileRangeBytes[0] = (uint64_t)*(pkt->getConstPtr<uint64_t>()); 
           //DPRINTF(NWTiming, "$_PROFILE_$ Range0 bytes= %i\n", system->profileRangeBytes[0]);
           satisfied = true;
       }
       else if ((pkt->getVaddr() == configRangeStartProfile1)) {
           system->profileRangeStartAddr[1] = (uint64_t)*(pkt->getConstPtr<uint64_t>()); 
           //DPRINTF(NWTiming, "$_PROFILE_$ Range1 start addr= 0x%x\n", system->profileRangeStartAddr[1]);
           satisfied = true;
       }
       else if ((pkt->getVaddr() == configRangeBytesProfile1)) {
           system->profileRangeBytes[1] = (uint64_t)*(pkt->getConstPtr<uint64_t>()); 
           //DPRINTF(NWTiming, "$_PROFILE_$ Range1 bytes= %i\n", system->profileRangeBytes[1]);
           satisfied = true;
       }
       else if ((pkt->getVaddr() == configRangeStartProfile2)) {
           system->profileRangeStartAddr[2] = (uint64_t)*(pkt->getConstPtr<uint64_t>()); 
           //DPRINTF(NWTiming, "$_PROFILE_$ Range2 start addr= 0x%x\n", system->profileRangeStartAddr[2]);
           satisfied = true;
       }
       else if ((pkt->getVaddr() == configRangeBytesProfile2)) {
           system->profileRangeBytes[2] = (uint64_t)*(pkt->getConstPtr<uint64_t>()); 
           //DPRINTF(NWTiming, "$_PROFILE_$ Range2 bytes= %i\n", system->profileRangeBytes[2]);
           satisfied = true;
       }
       //////////////////////////////////////////////////////////////
       else if ((pkt->getVaddr() == configRangeStartProfile3)) {
           system->profileRangeStartAddr[3] = (uint64_t)*(pkt->getConstPtr<uint64_t>()); 
           //DPRINTF(NWTiming, "$_PROFILE_$ Range3 start addr= 0x%x\n", system->profileRangeStartAddr[3]);
           satisfied = true;
       }
       else if ((pkt->getVaddr() == configRangeBytesProfile3)) {
           system->profileRangeBytes[3] = (uint64_t)*(pkt->getConstPtr<uint64_t>()); 
           //DPRINTF(NWTiming, "$_PROFILE_$ Range3 bytes= %i\n", system->profileRangeBytes[3]);
           satisfied = true;
       }
       else if ((pkt->getVaddr() == configRangeStartProfile4)) {
           system->profileRangeStartAddr[4] = (uint64_t)*(pkt->getConstPtr<uint64_t>()); 
           //DPRINTF(NWTiming, "$_PROFILE_$ Range4 start addr= 0x%x\n", system->profileRangeStartAddr[4]);
           satisfied = true;
       }
       else if ((pkt->getVaddr() == configRangeBytesProfile4)) {
           system->profileRangeBytes[4] = (uint64_t)*(pkt->getConstPtr<uint64_t>()); 
           //DPRINTF(NWTiming, "$_PROFILE_$ Range4 bytes= %i\n", system->profileRangeBytes[4]);
           satisfied = true;
       }
       else if ((pkt->getVaddr() == configRangeStartProfile5)) {
           system->profileRangeStartAddr[5] = (uint64_t)*(pkt->getConstPtr<uint64_t>()); 
           //DPRINTF(NWTiming, "$_PROFILE_$ Range5 start addr= 0x%x\n", system->profileRangeStartAddr[5]);
           satisfied = true;
       }
       else if ((pkt->getVaddr() == configRangeBytesProfile5)) {
           system->profileRangeBytes[5] = (uint64_t)*(pkt->getConstPtr<uint64_t>()); 
           //DPRINTF(NWTiming, "$_PROFILE_$ Range5 bytes= %i\n", system->profileRangeBytes[5]);
           satisfied = true;
       }
       //////////////////////////////////////////////////////////////
       else if ((pkt->getVaddr() == configRangeStartProfile6)) {
           system->profileRangeStartAddr[6] = (uint64_t)*(pkt->getConstPtr<uint64_t>()); 
           //DPRINTF(NWTiming, "$_PROFILE_$ Range6 start addr= 0x%x\n", system->profileRangeStartAddr[6]);
           satisfied = true;
       }
       else if ((pkt->getVaddr() == configRangeBytesProfile6)) {
           system->profileRangeBytes[6] = (uint64_t)*(pkt->getConstPtr<uint64_t>()); 
           //DPRINTF(NWTiming, "$_PROFILE_$ Range6 bytes= %i\n", system->profileRangeBytes[6]);
           satisfied = true;
       }
       else if ((pkt->getVaddr() == configRangeStartProfile7)) {
           system->profileRangeStartAddr[7] = (uint64_t)*(pkt->getConstPtr<uint64_t>()); 
           //DPRINTF(NWTiming, "$_PROFILE_$ Range7 start addr= 0x%x\n", system->profileRangeStartAddr[7]);
           satisfied = true;
       }
       else if ((pkt->getVaddr() == configRangeBytesProfile7)) {
           system->profileRangeBytes[7] = (uint64_t)*(pkt->getConstPtr<uint64_t>()); 
           //DPRINTF(NWTiming, "$_PROFILE_$ Range7 bytes= %i\n", system->profileRangeBytes[7]);
           satisfied = true;
       }
       else if ((pkt->getVaddr() == configRangeStartProfile8)) {
           system->profileRangeStartAddr[8] = (uint64_t)*(pkt->getConstPtr<uint64_t>()); 
           //DPRINTF(NWTiming, "$_PROFILE_$ Range8 start addr= 0x%x\n", system->profileRangeStartAddr[8]);
           satisfied = true;
       }
       else if ((pkt->getVaddr() == configRangeBytesProfile8)) {
           system->profileRangeBytes[8] = (uint64_t)*(pkt->getConstPtr<uint64_t>()); 
           //DPRINTF(NWTiming, "$_PROFILE_$ Range8 bytes= %i\n", system->profileRangeBytes[8]);
           satisfied = true;
       }
       //////////////////////////////////////////////////////////////
        //else if ((system->enableAtomic == 1) && (pkt->getVaddr() == configDestInfo) && isVertexStartAddr(pkt)) { 
        else if ((system->enableAtomic == 1) && (pkt->getVaddr() == configDestInfo)) { 
            unsigned int destVertexId =  *(unsigned int*)(pkt->getConstPtr<uint8_t>());
           
            pkt->setSrcInfo(srcInfo); 
            pkt->setAddInfo(addInfo); 
            pkt->setDestInfo(destVertexId); 
            pkt->setAtomicOppType(system->atomicOppType1); 
            pkt->setIsAtomic(true);
            pkt->setIsSparse(system->isSparse);
           
            // Nicholas - Adding PISC support for Graphit-supported CAS operations
            pkt->setCasCompareInfo(casCompareInfo);

            // Nicholas - Adding optional hardware deduplication
            pkt->setDedupEnabled(system->dedupEnabled);

            unsigned int destCoreId = destSPM(destVertexId);
            //pkt->setDestSpd((destId) % m_numCPU); 
            pkt->setDestSpd(destCoreId); 
            //pkt->setSetSpd(destId); 
            pkt->setSetSpd(setEntry(destVertexId)); 
            //pkt->setOffsetSpd(0); 

            if (destCoreId == pkt->req->contextId())
                pkt->setVertexLocal(true);

            if(system->isSparse) {
               
                Addr VaddrEdge = system->vertexStartAddr3 + (system->dataTypeSize3)*edgeIndex;
                DPRINTF(RubySequencer, "VaddrEdge %#x index %#x\n", VaddrEdge, edgeIndex);
                pkt->setEdgeVaddr(VaddrEdge); 
                /* 
                //Need to translate virtual to physical address
                ctx = system->getThreadContext(pkt->req->contextId());
    
                DPRINTF(RubySequencer, "TLB VaddrEdge: %#x index %i", VaddrEdge, (sizeof(unsigned int))*edgeIndex);
                //0x00000003 means valid size, paddr, vaddr
                //@todo change the constat size 8 
                Request *req = new Request(0, VaddrEdge, pkt->getSize(), 0x00000007,
                        ctx->getCpuPtr()->cpuId(), 0x0, ctx->contextId(),
                        ctx->threadId());
                req->taskId(ctx->getCpuPtr()->taskId());

                // translate to physical address
                Fault fault = ctx->getDTBPtr()->translateAtomic(req, ctx, BaseTLB::Write);
                DPRINTF(RubySequencer, "TLB VaddrEdge: %#x PAddr: %#x\n", VaddrEdge, req->getPaddr());
                assert(fault == NoFault);
                Addr PaddrEdge = req->getPaddr(); 
                pkt->setEdgeVaddr(VaddrEdge); 
                pkt->setEdgePaddr(PaddrEdge); 
                
                delete req;
               */ 
            }
            
            atomicExec = true;    
            DPRINTF(RubySequencer, "AtomicExec started src info: %f destId: %i add info: %f\n", *(double*)srcInfo, destVertexId, (double)addInfo);
        }
       /* 
        else if ((system->enableAtomic == 1) && (isVertexStartAddr(pkt))) {
            if (isVertexInAddrRange(pkt, 1)) {
                 atomicDataMapped1 = *(pkt->getConstPtr<uint64_t>());
                 atomicDataMappedAddr1 = pkt->getVaddr();
            
                if (system->noStatusRead) {
                    pkt->setAtomicNewValue(atomicDataMapped1); 
                    pkt->setAtomicMappedAddr(atomicDataMappedAddr1); 
                    pkt->setAtomicOppType(system->atomicOppType1); 
                    pkt->setIsAtomic(true);
                    pkt->setIsSparse(isSparse);
                    pkt->setsystem->noStatusRead(true);
                    pkt->setSetSpd((atomicDataMappedAddr1 - vertexStartAddr1)/system->stride1); 
                    pkt->setOffsetSpd(0); 
                    pkt->setDestSpd(((atomicDataMappedAddr1 - vertexStartAddr1)/system->stride1) % m_numCPU); 
            
                    if ((((atomicDataMappedAddr1 - vertexStartAddr1)/system->stride1) % m_numCPU) == pkt->req->contextId())
                        pkt->setVertexLocal(true);

                    if ((system->atomicOppType1 == 'u') | (system->atomicOppType1 == 'i')) { 
                        pkt->setAtomicOldValue(system->atomicOldValue1); 
                    } 
                    atomicExec = true;    
                }
                else {
                    satisfied = true;
                }
           
            }
            else if (isVertexInAddrRange(pkt, 2)) {
                atomicDataMapped2 = *(pkt->getConstPtr<uint64_t>()); 
                 atomicDataMappedAddr2 = pkt->getVaddr();
            
                if (system->noStatusRead) {
                    pkt->setAtomicNewValue(atomicDataMapped2); 
                    pkt->setAtomicMappedAddr(atomicDataMappedAddr2); 
                    pkt->setAtomicOppType(system->atomicOppType2); 
                    pkt->setIsAtomic(true);
                    pkt->setIsSparse(isSparse);
                    pkt->setsystem->noStatusRead(true);
                    pkt->setSetSpd((atomicDataMappedAddr2 - system->vertexStartAddr2)/system->stride2); 
                    pkt->setOffsetSpd(1); 
                    pkt->setDestSpd(((atomicDataMappedAddr2 - system->vertexStartAddr2)/system->stride2) % m_numCPU); 
            
                    if ((((atomicDataMappedAddr2 - system->vertexStartAddr2)/system->stride2) % m_numCPU) == pkt->req->contextId())
                        pkt->setVertexLocal(true);

                    if ((system->atomicOppType1 == 'u') | (system->atomicOppType1 == 'i')) { 
                        pkt->setAtomicOldValue(system->atomicOldValue2); 
                    } 
                    atomicExec = true;    
                }
                else {
                    satisfied = true;
                }
            }
            else if (isVertexInAddrRange(pkt, 3)) {
                //do nothing
            }
            else {
                warn_once("Invalid range of addresses mapped for atomic execution, check if more than two addresses are mapped"); 
            }
        }
       */ 
    }
    else if (pkt->isRead()) {
       
        uint8_t* cachedData = lookupReadOnlyCachedData(pkt, pkt->req->contextId());
        if ((system->enableAtomic == 1) && (system->validVCaching[pkt->req->contextId()]) && (cachedData != NULL)) {
           //copy the value  
            DPRINTF(RubySequencer, "cached vertex read data %i\n", *(int*)cachedData);
            //uint8_t* data = static_cast<uint8_t*>(static_cast<void*>(&cachedData)); 
            memcpy(pkt->getPtr<uint8_t>(), cachedData, pkt->getSize());
            m_local_vertex_buffer_access++;
            satisfied = true;

            // Nicholas - Adding memory access tracking
            pkt->req->setNWMemoryAccessType(NWMemAccessType_LD_RO_Buffer);
        }
        else if (pkt->getVaddr() == configNumMappedVertices) {
            //copy the value  
            
            unsigned int numSpdSets = m_vertexCache_ptr->getNumSets();
            unsigned int numSpdLinesMapped = numSpdSets*m_numCPU;

            memcpy(pkt->getPtr<uint8_t>(), &numSpdLinesMapped, pkt->getSize());
            DPRINTF(RubySequencer, "number of vertices mapped to spm read data numSpdSets %i linesmapped %i numCPU %i\n", numSpdLinesMapped);
            satisfied = true;

            // Nicholas - Adding memory access tracking
            pkt->req->setNWMemoryAccessType(NWMemAccessType_LD_MMIO);
        }
       else if ((pkt->getVaddr() == configActiveVerticesTotal)) { 
            DPRINTF(RubySequencer, "number of vertices active %i\n", system->active_vertices_total );
            memcpy(pkt->getPtr<uint8_t>(), &(system->active_vertices_total) , pkt->getSize());
            satisfied = true;

            // Nicholas - Adding memory access tracking
            pkt->req->setNWMemoryAccessType(NWMemAccessType_LD_MMIO);
       }
       else if ((pkt->getVaddr() == configActiveVerticesPerSpm)) { 
            memcpy(pkt->getPtr<uint8_t>(), &(system->active_vertices_per_spm[pkt->req->contextId()]), pkt->getSize());
            satisfied = true;

            // Nicholas - Adding memory access tracking
            pkt->req->setNWMemoryAccessType(NWMemAccessType_LD_MMIO);
       }
       // Nicholas - Adding outstanding PISC op tracking
       else if ((pkt->getVaddr() == configOutstandingPISCOps)) {
           memcpy(pkt->getPtr<uint8_t>(), &system->outstandingPISCOps, pkt->getSize());
           satisfied = true;

           // Nicholas - Adding memory access tracking
           pkt->req->setNWMemoryAccessType(NWMemAccessType_LD_MMIO);
       }

        
        //Reusing the pkt to send atomic command to the scratchpads
        //its vaddr and paddr are reset to the vertex address and will be 
        //set to orignal addresses once the atomic instruction complete
        /* 
        else if ((pkt->getVaddr() == configsystem->atomicStatusAddr1)) { 
            DPRINTF(RubySequencer, "Got atomic status instruction 1\n");
            //modify load packet: old value, new value, and type of atomic operation
            
            pkt->setAtomicNewValue(atomicDataMapped1); 
            pkt->setAtomicMappedAddr(atomicDataMappedAddr1); 
            pkt->setAtomicOppType(system->atomicOppType1); 
            pkt->setIsAtomic(true);
            pkt->setsystem->noStatusRead(false);
            pkt->setIsSparse(isSparse);
            pkt->setSetSpd((atomicDataMappedAddr1 - vertexStartAddr1)/system->stride1); 
            pkt->setOffsetSpd(0); 
            pkt->setDestSpd(((atomicDataMappedAddr1 - vertexStartAddr1)/system->stride1) % m_numCPU); 
            
            DPRINTF(RubySequencer, "atomic mapped addr 1 %#x\n vertex startaddr 1 %#x\n", atomicDataMappedAddr1, vertexStartAddr1);
            
            if ((((atomicDataMappedAddr1 - vertexStartAddr1)/system->stride1) % m_numCPU) == pkt->req->contextId())
                pkt->setVertexLocal(true);

            if ((system->atomicOppType1 == 'u') | (system->atomicOppType1 == 'i')) { 
                pkt->setAtomicOldValue(system->atomicOldValue1); 
            } 
            atomicExec = true;    
        }
        else if ((pkt->getVaddr() == configsystem->atomicStatusAddr2)) { 
            DPRINTF(RubySequencer, "Got atomic status instruction 2\n");
            //modify load packet: old value, new value, and type of atomic operation
            //modify load packet: old value, new value, and type of atomic operation
             //modify load packet: old value, new value, and type of atomic operation
            
            pkt->setAtomicNewValue(atomicDataMapped2); 
            pkt->setAtomicOppType(system->atomicOppType2); 
            pkt->setIsAtomic(true);
            pkt->setsystem->noStatusRead(false);
            pkt->setSetSpd((atomicDataMappedAddr2 - system->vertexStartAddr2)/system->stride2); 
            pkt->setIsSparse(isSparse);
            pkt->setOffsetSpd(1); 
            pkt->setDestSpd(((atomicDataMappedAddr2 - system->vertexStartAddr2)/system->stride2) % m_numCPU); 
            DPRINTF(RubySequencer, "atomic mapped addr 2 %#x\n vertex startaddr 2 %#x\n", atomicDataMappedAddr2, system->vertexStartAddr2);

            if ((((atomicDataMappedAddr2 - system->vertexStartAddr2)/system->stride2) % m_numCPU) == pkt->req->contextId())
                pkt->setVertexLocal(true);


            if ((system->atomicOppType1 == 'u') | (system->atomicOppType1 == 'i')) { 
                pkt->setAtomicOldValue(system->atomicOldValue2); 
            }
            atomicExec = true;    
        } 
   
       */ 
        }
    // Abraham - adding vertex cache 
    //@param satisfied = true when there is no need to access scratchpads
    if (satisfied) {
        // Nicholas - Adding memory access tracking
        // Setting this access type here to cover many of the above cases
        if (pkt->isWrite()) {
            pkt->req->setNWMemoryAccessType(NWMemAccessType_ST_MMIO_Other);
        }

        return RequestStatus_Satisfied;
    }
    
    if (m_outstanding_count >= m_max_outstanding_requests) {
        return RequestStatus_BufferFull;
    }

    RubyRequestType primary_type = RubyRequestType_NULL;
    RubyRequestType secondary_type = RubyRequestType_NULL;
  

    //@todo
    //pass the atomicExec instruction without recording to the read and write reqeust table otherwise
    //we need to check the address of the stored metadata instead of the current packet
    //Adding atomic instruction execution 
    if (atomicExec) {
        if (pkt->getVertexLocal()) {
            DPRINTF(RubySequencer, "Issuing atomic offloading instruction local\n");
            primary_type = secondary_type = RubyRequestType_ST_Vertex_Local;
        }
        else {
            DPRINTF(RubySequencer, "Issuing atomic offloading instruction remote\n");
            primary_type = secondary_type = RubyRequestType_ST_Vertex_Remote;
        }
   }
    //Adding atomic instruction execution 
    //if (pkt->isLLSC()) {
    else if (pkt->isLLSC()) {
        //
        // Alpha LL/SC instructions need to be handled carefully by the cache
        // coherence protocol to ensure they follow the proper semantics. In
        // particular, by identifying the operations as atomic, the protocol
        // should understand that migratory sharing optimizations should not
        // be performed (i.e. a load between the LL and SC should not steal
        // away exclusive permission).
        //
        if (pkt->isWrite()) {
            DPRINTF(RubySequencer, "Issuing SC\n");
            primary_type = RubyRequestType_Store_Conditional;
        } else {
            DPRINTF(RubySequencer, "Issuing LL\n");
            assert(pkt->isRead());
            primary_type = RubyRequestType_Load_Linked;
        }
        secondary_type = RubyRequestType_ATOMIC;
    } else if (pkt->req->isLockedRMW()) {
        //
        // x86 locked instructions are translated to store cache coherence
        // requests because these requests should always be treated as read
        // exclusive operations and should leverage any migratory sharing
        // optimization built into the protocol.
        //
        if (pkt->isWrite()) {
            DPRINTF(RubySequencer, "Issuing Locked RMW Write addr: %#x\n", pkt->getVaddr());
            primary_type = RubyRequestType_Locked_RMW_Write;
             
            //Abraham - adding pkt type
            if (isVertexStartAddr(pkt)) {
                if (isVertexLocal(pkt)) {
                    DPRINTF(RubySequencer, "Issuing Locked RMW Vertex Write Local\n");
                    primary_type = RubyRequestType_Locked_Vertex_Write_Local;
                    secondary_type = RubyRequestType_ST_Vertex_Local;
                }
                else {
                    DPRINTF(RubySequencer, "Issuing Locked RMW Vertex Write Remote\n");
                    primary_type = RubyRequestType_Locked_Vertex_Write_Remote;
                    secondary_type = RubyRequestType_ST_Vertex_Remote;
                }
            }
            
        } else {
            DPRINTF(RubySequencer, "Issuing Locked RMW Read addr: %#x\n", pkt->getVaddr());
            assert(pkt->isRead());
            primary_type = RubyRequestType_Locked_RMW_Read;
       
            //Abraham - adding pkt type
            if (isVertexStartAddr(pkt)) {
                
                if (isVertexLocal(pkt)) {
                    DPRINTF(RubySequencer, "Issuing Locked RMW Vertex Read Local\n");
                    primary_type = RubyRequestType_Locked_Vertex_Read_Local;
                    secondary_type = RubyRequestType_ST_Vertex_Local;
                }
                else {
                    DPRINTF(RubySequencer, "Issuing Locked RMW Vertex Read Remote\n");
                    primary_type = RubyRequestType_Locked_Vertex_Read_Remote;
                    //This is different from the local because we writing inside ruby
                    //and we need distinction
                    secondary_type = RubyRequestType_Locked_LD_Vertex_Remote;
                }
            }
        }
        
        if (!isVertexStartAddr(pkt)) 
            secondary_type = RubyRequestType_ST;
       /* 
        //Abraham adding pkt type
        if (isVertexStartAddr(pkt)) {
            if (isVertexLocal(pkt))
                //secondary_type = RubyRequestType_ST_Vertex_Local;
                secondary_type = RubyRequestType_Locked_Vertex_Read_Local;
            else
                //secondary_type = RubyRequestType_ST_Vertex_Remote;
                secondary_type = RubyRequestType_Locked_Vertex_Read_Remote;
        }
        */
    } else {
        if (pkt->isRead()) {
            if (pkt->req->isInstFetch()) {
                primary_type = secondary_type = RubyRequestType_IFETCH;
                DPRINTF(RubySequencer, "Issuing Instruction Fetch addr: %#x\n", pkt->getVaddr());
            } else {
                bool storeCheck = false;
                // only X86 need the store check
                if (system->getArch() == Arch::X86ISA) {
                    uint32_t flags = pkt->req->getFlags();
                    storeCheck = flags &
                        (X86ISA::StoreCheck << X86ISA::FlagShift);
                }
                if (storeCheck) {
                    primary_type = RubyRequestType_RMW_Read;
                    secondary_type = RubyRequestType_ST;
                    
                    //Abraham - adding pkt type
                    if (isVertexStartAddr(pkt)) {
                        //panic("Issuing storecheck on vertex not handled\n");
                        DPRINTF(RubySequencer, "Issuing storecheck on vertex not handled\n");
                       
                       /* 
                        if (isVertexLocal(pkt)) {
                            primary_type = RubyRequestType_Locked_Vertex_Read_Local;
                            secondary_type = RubyRequestType_ST_Vertex_Local;
                        }
                        else {
                            primary_type = RubyRequestType_Locked_Vertex_Read_Remote;
                            secondary_type = RubyRequestType_ST_Vertex_Remote;
                        }
                        */
                    }
                
                } else {
                    primary_type = secondary_type = RubyRequestType_LD;
                    //Abraham adding pkt type
                    DPRINTF(RubySequencer, "Issuing Read vaddr: %#x\n", pkt->getVaddr());
                    
                    if ((!system->isSparse) && (isVertexStartAddr(pkt)) && (isVertexInAddrRange(pkt, 3))) {
                           
                            DPRINTF(RubySequencer, "Issuing Active list checking read\n");
                           
                            pkt->setIsSparse(false);
                            pkt->setNumTrackedAtomicAddr(system->numTrackedAtomic);
                            //pkt->setIsCopy(false);
                            
                            if (isVertexLocal(pkt)) {
                                DPRINTF(RubySequencer, "Issuing new active list checking read local\n");
                                primary_type = secondary_type = RubyRequestType_CHECK_ACTIVEVERTEX_Local;
                            }
                            else {
                                DPRINTF(RubySequencer, "Issuing new active list checking read remote\n");
                                primary_type = secondary_type = RubyRequestType_CHECK_ACTIVEVERTEX_Remote;
                            }
                    }
                  
                    else if ((!system->isSparse) && (isVertexStartAddr(pkt)) && (isVertexInAddrRange(pkt, 5))) {//old
                           
                            DPRINTF(RubySequencer, "Issuing Active list old checking read\n");
                           
                            pkt->setIsSparse(false);
                            pkt->setNumTrackedAtomicAddr(system->numTrackedAtomic);
                            //pkt->setIsCopy(false);
                            
                            if (isVertexLocal(pkt)) {
                                DPRINTF(RubySequencer, "Issuing old active list checking read local\n");
                                primary_type = secondary_type = RubyRequestType_CHECK_ACTIVEVERTEX_Local;
                            }
                            else {
                                DPRINTF(RubySequencer, "Issuing old active list checking read remote\n");
                                primary_type = secondary_type = RubyRequestType_CHECK_ACTIVEVERTEX_Remote;
                            }
                    }

                   /* 
                    else if ((!isSparse) &&(isCopy) && (!pkt->getIsActiveListHandled()) && (isVertexStartAddr(pkt)) && (isVertexInAddrRange(pkt, 3))) {
                        panic("Issuing Active list checking read\n"); 
                        
                        DPRINTF(RubySequencer, "Issuing old active list checking read dense\n");
                        //Atomic instructions execution update the next data structure so no need to write this
                        pkt->setIsSparse(false);
                        pkt->setsystem->numTrackedAtomicAddr(system->numTrackedAtomic);
                        pkt->setIsCopy(true);
                        
                        if (isVertexLocal(pkt)) {
                            DPRINTF(RubySequencer, "Issuing old active list checking read local\n");
                            primary_type = secondary_type = RubyRequestType_CHECK_ACTIVEVERTEX_Local;
                        }
                        else {
                            
                            DPRINTF(RubySequencer, "Issuing old active list checking read remote\n");
                            primary_type = secondary_type = RubyRequestType_CHECK_ACTIVEVERTEX_Remote;
                        }
                    }
                    else if ((isVertexStartAddr(pkt)) && (!isVertexInAddrRange(pkt, 3))) {
                   */ 
                    else if ((isVertexStartAddr(pkt))) {
                        DPRINTF(RubySequencer, "Issuing Vertex Read vaddr: %#x\n", pkt->getVaddr());
                        
                        DPRINTF(RubySequencer, "Thread Id: %#x numCPU: %#x \n", pkt->req->contextId(), m_numCPU);
                        if (isVertexLocal(pkt)) {
                            primary_type = secondary_type = RubyRequestType_LD_Vertex_Local;
                            DPRINTF(RubySequencer, "Issuing Vertex Read local vaddr: %#x\n", pkt->getVaddr());
                        }
                        else {
                            primary_type = secondary_type = RubyRequestType_LD_Vertex_Remote;
                            DPRINTF(RubySequencer, "Issuing Vertex Read remote vaddr: %#x\n", pkt->getVaddr());
                        }
                    }
                }
            }
        } else if (pkt->isWrite()) {
            //
            // Note: M5 packets do not differentiate ST from RMW_Write
            //
            primary_type = secondary_type = RubyRequestType_ST;
            //Abraham adding pkt type
            DPRINTF(RubySequencer, "Issuing Write vaddr: %#x\n", pkt->getVaddr());
            
            if (isEdgePrefetchSize(pkt)) {
                DPRINTF(RubySequencer, "Issuing Edge Prefetching\n");
                primary_type = secondary_type = RubyRequestType_PREFETCH_EDGE;
            }
           /* 
            else if ((isSparse) && (!pkt->getIsActiveListHandled()) && (isOutEdgeActiveList(pkt))) {
                DPRINTF(RubySequencer, "Issuing Active list checking write sparse\n");
                pkt->setIsSparse(true);
                pkt->setsystem->numTrackedAtomicAddr(system->numTrackedAtomic);
            
                if (isOutEdgeLocal(pkt)) {
                    DPRINTF(RubySequencer, "Issuing Active list checking write local\n");
                    primary_type = secondary_type = RubyRequestType_CHECK_ACTIVEVERTEX_Local;
                }
                else {
                    DPRINTF(RubySequencer, "Issuing Active list checking write remote\n");
                    primary_type = secondary_type = RubyRequestType_CHECK_ACTIVEVERTEX_Remote;
                }
            }
           */ 
            else if ((!system->isSparse)&& (system->isCopy) && (!pkt->getIsActiveListHandled()) && (isVertexStartAddr(pkt)) && (isVertexInAddrRange(pkt, 3))) {
                    DPRINTF(RubySequencer, "Issuing Active list checking write dense copy\n");
                    //Atomic instructions execution update the next data structure so no need to write this
                    pkt->setIsSparse(false);
                    pkt->setNumTrackedAtomicAddr(system->numTrackedAtomic);
                    pkt->setIsCopy(true);
            
                   if (isVertexLocal(pkt)) {
                        DPRINTF(RubySequencer, "Issuing Active list checking write local copy\n");
                        primary_type = secondary_type = RubyRequestType_CHECK_ACTIVEVERTEX_Local;
                    }
                    else {
                        DPRINTF(RubySequencer, "Issuing Active list checking write remote copy\n");
                       primary_type = secondary_type = RubyRequestType_CHECK_ACTIVEVERTEX_Remote;
                    }
            }
            else if ((!system->isSparse) && (!pkt->getIsActiveListHandled()) && (isVertexStartAddr(pkt)) && (isVertexInAddrRange(pkt, 3))) {
                    DPRINTF(RubySequencer, "Issuing Active list checking write dense non copy\n");
                    //Atomic instructions execution update the next data structure so no need to write this
                    pkt->setIsSparse(false);
                    pkt->setNumTrackedAtomicAddr(system->numTrackedAtomic);
                    ///pkt->setIsCopy(false);
            
                   if (isVertexLocal(pkt)) {
                        DPRINTF(RubySequencer, "Issuing Active list checking write local\n");
                        primary_type = secondary_type = RubyRequestType_CHECK_ACTIVEVERTEX_Local;
                    }
                    else {
                        DPRINTF(RubySequencer, "Issuing Active list checking write remote\n");
                       primary_type = secondary_type = RubyRequestType_CHECK_ACTIVEVERTEX_Remote;
                    }
            }
            //else if ((!system->isSparse) && (system->isCopy) &&(!pkt->getIsActiveListHandled()) && (isVertexStartAddr(pkt)) && (isVertexInAddrRange(pkt, 3))) {
            else if ((!system->isSparse) && (!pkt->getIsActiveListHandled()) && (isVertexStartAddr(pkt)) && (isVertexInAddrRange(pkt, 5))) {
                    DPRINTF(RubySequencer, "Issuing old active list checking write dense copy\n");
                    //Atomic instructions execution update the next data structure so no need to write this
                    pkt->setIsSparse(false);
                    pkt->setNumTrackedAtomicAddr(system->numTrackedAtomic);
                    //pkt->setIsCopy(true);
                   if (isVertexLocal(pkt)) {
                        DPRINTF(RubySequencer, "Issuing old active list checking write local\n");
                        primary_type = secondary_type = RubyRequestType_CHECK_ACTIVEVERTEX_Local;
                    }
                    else {
                      DPRINTF(RubySequencer, "Issuing old active list checking write remote\n");
                       primary_type = secondary_type = RubyRequestType_CHECK_ACTIVEVERTEX_Remote;
                    }
            }
            else if ((!pkt->getIsActiveListHandled()) && isVertexStartAddr(pkt)) {
                DPRINTF(RubySequencer, "Issuing Vertex Write\n");
                if (isVertexLocal(pkt)) {
                    DPRINTF(RubySequencer, "Issuing Vertex Write locally\n");
                    primary_type = secondary_type = RubyRequestType_ST_Vertex_Local;
                }
                else {
                    DPRINTF(RubySequencer, "Issuing Vertex Write remotely\n");

                    primary_type = secondary_type = RubyRequestType_ST_Vertex_Remote;
                }
            }
        } else if (pkt->isFlush()) {
          primary_type = secondary_type = RubyRequestType_FLUSH;
        } else {
            panic("Unsupported ruby packet type\n");
        }
    }

    #ifdef ASYNC_PISCS
    // Nicholas - Adding async PISCs
    if (atomicExec) {
        // Make a packet + request to handle asynchronously.
        // The original packet and request will be returned to the sender
        // later when we return RequestStatus_Satisfied

        // Copy the original request (original can be deleted before the PISC op is complete)
        // Note that this new request will start timing from this point
        assert(pkt->req->hasPaddr());
        RequestPtr reqPISC = new Request(
            pkt->req->getAsid(),
            pkt->req->getVaddr(),
            pkt->req->getSize(),
            pkt->req->getFlags(),
            pkt->req->masterId(),
            pkt->req->getPC(),
            pkt->req->contextId(),
            pkt->req->threadId()
        );
        assert(pkt->req->hasPaddr());
        reqPISC->setPaddr(pkt->req->getPaddr());

        // Create a packet for the new request
        // Using "Writeback" instead of "WriteReq" since this packet won't need
        // to generate a response to be returned to the sending CPU
        PacketPtr pktPISC = new Packet(reqPISC, MemCmd::Command::Writeback);
        // Copy PISC atomic contents
        pktPISC->setSrcInfo(       pkt->getSrcInfo()       );
        pktPISC->setAddInfo(       pkt->getAddInfo()       );
        pktPISC->setDestInfo(      pkt->getDestInfo()      );
        pktPISC->setAtomicOppType( pkt->getAtomicOppType() );
        pktPISC->setIsAtomic(      pkt->getIsAtomic()      );
        pktPISC->setIsSparse(      pkt->getIsSparse()      );
        pktPISC->setCasCompareInfo(pkt->getCasCompareInfo());
        pktPISC->setDedupEnabled(  pkt->getDedupEnabled()  );
        pktPISC->setDestSpd(       pkt->getDestSpd()       );
        pktPISC->setSetSpd(        pkt->getSetSpd()        );
        pktPISC->setVertexLocal(   pkt->getVertexLocal()   );
        pktPISC->setEdgeVaddr(     pkt->getEdgeVaddr()     );
        // Copy the original packet's dynamic data
        unsigned int *destVertexIdPtr = new unsigned int;
        *destVertexIdPtr = *(unsigned int*)(pkt->getConstPtr<uint8_t>());
        pktPISC->dataDynamic(destVertexIdPtr);

        // Submit the new request
        RequestStatus status = insertRequest(pktPISC, primary_type);
        assert(status == RequestStatus_Ready);
        issueRequest(pktPISC, secondary_type);

        // PISC op is now outstanding, increment the counter
        system->outstandingPISCOps++;
        localOutstandingPISCOps++;
        assert(localOutstandingPISCOps <= ASYNC_PISC_LIMIT);

        //NWMemAccessType_ST_MMIO_PISC_Local
        //NWMemAccessType_ST_MMIO_PISC_Remote
        //pkt->req->setNWMemoryAccessType(NWMemAccessType_ST_MMIO_Other);

        // PISC op launched, let the requester continue sending stores
        return RequestStatus_Satisfied;
    }
    #endif

    RequestStatus status = insertRequest(pkt, primary_type);
    if (status != RequestStatus_Ready) 
        return status;
    issueRequest(pkt, secondary_type);

    // TODO: issue hardware prefetches here
    return RequestStatus_Issued;
}

bool
Sequencer::isVertexInAddrRange(PacketPtr pkt, int addrRange) {

    int numSpdSets = m_vertexCache_ptr->getNumSets();
    int numLinesMapped = (system->numVertex > numSpdSets*m_numCPU) ? numSpdSets*m_numCPU : system->numVertex;
    //DPRINTF(RubySequencer, "Details of spec. system->numVertex: %i numSpdSets: %i numLinesMapped: %i system->stride1: %i  stride2: %i system->dataTypeSize1: %i system->dataTypeSize2: %i system->dataTypeSize3: %i\n", 
    //        system->numVertex, numSpdSets, numLinesMapped, system->stride1, stride2, system->dataTypeSize1, system->dataTypeSize2, system->dataTypeSize3);
    //DPRINTF(RubySequencer, "Adddr current pkt: %#x stored addr1: %#x addr2 %#x addr3: %#x \n", pkt->getVaddr(), vertexStartAddr1, system->vertexStartAddr2, system->vertexStartAddr3);

    if((system->enableTrackedAddr1 == 1) && (addrRange == 1) )
        return (((pkt->getVaddr() >= system->vertexStartAddr1) &&  (pkt->getVaddr() < 
                (system->vertexStartAddr1 + system->stride1*numLinesMapped)) && 
                ((pkt->getVaddr()-system->vertexStartAddr1)%system->stride1 == 0)));
    else if((system->enableTrackedAddr2 == 1) && (addrRange == 2) )
        return (((pkt->getVaddr() >= system->vertexStartAddr2) &&  (pkt->getVaddr() < 
                (system->vertexStartAddr2 + system->stride2*numLinesMapped)) && 
                ((pkt->getVaddr()-system->vertexStartAddr2)%system->stride2 == 0)));
    else if((system->enableTrackedAddr3 == 1) && (addrRange == 3) )
        return ((((pkt->getVaddr() >= system->vertexStartAddr3) &&  (pkt->getVaddr() < 
                (system->vertexStartAddr3 + system->stride3*numLinesMapped)) && 
                ((pkt->getVaddr()-system->vertexStartAddr3)%system->stride3 == 0))));
    else if((system->enableTrackedAddr3_old == 1) && (addrRange == 5) ) //old vaddr 3
        return  (((pkt->getVaddr() >= system->vertexStartAddr3_old) &&  (pkt->getVaddr() < 
                (system->vertexStartAddr3_old + system->stride3_old*numLinesMapped)) && 
                ((pkt->getVaddr()-system->vertexStartAddr3_old)%system->stride3_old == 0)));
    else if((system->enableTrackedAddr4 == 1) && (addrRange == 4) )
        return (((pkt->getVaddr() >= system->vertexStartAddr4) &&  (pkt->getVaddr() < 
                (system->vertexStartAddr4 + system->stride4*numLinesMapped)) && 
                ((pkt->getVaddr()-system->vertexStartAddr4)%system->stride4 == 0)));

  
  return false;
   
}
/*
bool
Sequencer::isVertexInAddrRange_old(PacketPtr pkt, int addrRange) {

    int numSpdSets = m_vertexCache_ptr->getNumSets();
    int numLinesMapped = (system->numVertex > numSpdSets*m_numCPU) ? numSpdSets*m_numCPU : system->numVertex;
    //DPRINTF(RubySequencer, "Details of spec. system->numVertex: %i numSpdSets: %i numLinesMapped: %i system->stride1: %i  stride2: %i system->dataTypeSize1: %i system->dataTypeSize2: %i system->dataTypeSize3: %i\n", 
    //        system->numVertex, numSpdSets, numLinesMapped, system->stride1, stride2, system->dataTypeSize1, system->dataTypeSize2, system->dataTypeSize3);
    //DPRINTF(RubySequencer, "Adddr current pkt: %#x stored addr1: %#x addr2 %#x addr3: %#x \n", pkt->getVaddr(), vertexStartAddr1, system->vertexStartAddr2, system->vertexStartAddr3);

    if (addrRange == 3) 
        return (((pkt->getVaddr() >= system->vertexStartAddr3_old) &&  (pkt->getVaddr() < 
                (system->vertexStartAddr3_old + system->stride3_old*numLinesMapped)) && 
                ((pkt->getVaddr()-system->vertexStartAddr3_old)%system->stride3_old == 0)));
    else
       panic("Invalid address range choice old address tracking only done on address 3");
  
  return false;
} 
*/

bool
Sequencer::isEdgePrefetchSize(PacketPtr pkt) {
    if(pkt->getVaddr() == configPrefetchSize)
        return true;
   
   return false;
}

bool
Sequencer::isOutEdgeActiveList(PacketPtr pkt) {
   
    unsigned int vertexId = (unsigned int)*(pkt->getConstPtr<uint64_t>());
    
    if (system->enableTrackedAddr3 == 1) {
        if (isEdgeInAddrRange(pkt, 3)) { 
            pkt->setIsSparse(system->isSparse);
            pkt->setOffsetSpd(2); 
            //pkt->setDestSpd((vertexId) % m_numCPU); 
            unsigned int destId = destSPM(vertexId);
            pkt->setDestSpd(destId); 
             //pkt->setSetSpd(vertexId); 
            pkt->setSetSpd(setEntry(vertexId)); 
           
            DPRINTF(RubySequencer, "Is outgoing edge writing number of tracked atomic %i set number %i offset %i\n", system->numTrackedAtomic, pkt->getSetSpd(), pkt->getOffsetSpd());
            return true;
        }
    }
   return false;
}

bool
Sequencer::isOutEdgeLocal(PacketPtr pkt)
{
    
    DPRINTF(RubySequencer, "IsOutEdgeLocal vaddr:%x difference from start addr: %i numCPU: %i contextId: %i\n", pkt->getVaddr(), (pkt->getVaddr() - system->vertexStartAddr1), m_numCPU, pkt->req->contextId());
   
    unsigned int vertexId = (unsigned int)*(pkt->getConstPtr<uint64_t>());
    //most probably it is in range 3 
    if (isEdgeInAddrRange(pkt, 3)) {
        if (((vertexId) % m_numCPU) == pkt->req->contextId())
            return true;
    }
    else if (isEdgeInAddrRange(pkt, 1)) {
        DPRINTF(RubySequencer, "pkt is in addre range 1\n");
        if (((vertexId) % m_numCPU) == pkt->req->contextId())
            return true;
    }
    else if (isEdgeInAddrRange(pkt, 2)) {
        if (((vertexId) % m_numCPU) == pkt->req->contextId())
            return true;
    }

    else if (isEdgeInAddrRange(pkt, 4)) {
        if (((vertexId) % m_numCPU) == pkt->req->contextId())
            return true;
    }
    else
       panic("Invalid address range choice");
 
    return false;

}



bool
Sequencer::isEdgeInAddrRange(PacketPtr pkt, int addrRange) {

    int numLinesMapped = system->numOutEdges;
    
    if (addrRange == 1) 
        return (((pkt->getVaddr() >= system->vertexStartAddr1) &&  (pkt->getVaddr() < 
                (system->vertexStartAddr1 + system->stride1*numLinesMapped)) && 
                ((pkt->getVaddr()-system->vertexStartAddr1)%system->stride1 == 0)));
    else if (addrRange == 2) 
        return (((pkt->getVaddr() >= system->vertexStartAddr2) &&  (pkt->getVaddr() < 
                (system->vertexStartAddr2 + system->stride2*numLinesMapped)) && 
                ((pkt->getVaddr()-system->vertexStartAddr2)%system->stride2 == 0)));
    else if (addrRange == 3) 
        return (((pkt->getVaddr() >= system->vertexStartAddr3) &&  (pkt->getVaddr() < 
                (system->vertexStartAddr3 + system->stride3*numLinesMapped)) && 
                ((pkt->getVaddr()-system->vertexStartAddr3)%system->stride3 == 0)));
    else if (addrRange == 4) 
        return (((pkt->getVaddr() >= system->vertexStartAddr4) &&  (pkt->getVaddr() < 
                (system->vertexStartAddr4 + system->stride4*numLinesMapped)) && 
                ((pkt->getVaddr()-system->vertexStartAddr4)%system->stride4 == 0)));
    else
       panic("Invalid address range choice or no address was mapped to scratchpad");
  
  return false;
   
}

unsigned 
Sequencer::setEntry(unsigned int vertexId) {

    //int numLinesMapped = (system->numVertex > numSpdSets*m_numCPU) ? numSpdSets*m_numCPU : system->numVertex;
    
    DPRINTF(RubySequencer, "system->chunkSize%#u system->numVertex %#x vertexId %#x\n", system->chunkSize, system->numVertex, vertexId);
    
    return ((((vertexId/system->chunkSize)/m_numCPU)*system->chunkSize) + (vertexId%system->chunkSize));
}

unsigned  
Sequencer::destSPM(unsigned int vertexId) {
    
    return (vertexId/system->chunkSize)%m_numCPU;

}

bool
Sequencer::isVertexStartAddr(PacketPtr pkt) 
{
    //DPRINTF(RubySequencer, "vertexADdrSize1 %#x Stored Start Address %#x New address %#x\n", system->dataTypeSize1,  system->vertexStartAddr1, pkt->getVaddr());

    if (system->enableTrackedAddr1 == 1) {
        if (isVertexInAddrRange(pkt, 1)) {
            //pkt->setSetSpd((pkt->getVaddr() - system->vertexStartAddr1)/system->stride1); 
            pkt->setIsSparse(system->isSparse);
            pkt->setOffsetSpd(0); 
            //pkt->setDestSpd((( pkt->getVaddr()- system->vertexStartAddr1)/system->stride1) % m_numCPU); 
            unsigned int destId = destSPM(( pkt->getVaddr()- system->vertexStartAddr1)/system->stride1);
            pkt->setDestSpd(destId); 
            pkt->setSetSpd(setEntry((pkt->getVaddr() - system->vertexStartAddr1)/system->stride1)); 
            
            DPRINTF(RubySequencer, " Num addr 1, Is vertex cache address \n");
            return true;
        }
    }
    if (system->enableTrackedAddr2 == 1) {
        if (isVertexInAddrRange(pkt, 2)) {
            //pkt->setSetSpd((pkt->getVaddr()  - system->vertexStartAddr2)/system->stride2); 
            pkt->setIsSparse(system->isSparse);
            pkt->setOffsetSpd(1); 
            //pkt->setDestSpd(((pkt->getVaddr() - system->vertexStartAddr2)/system->stride2) % m_numCPU); 
            unsigned int destId = destSPM(( pkt->getVaddr()- system->vertexStartAddr2)/system->stride2);
            pkt->setDestSpd(destId); 
            pkt->setSetSpd(setEntry((pkt->getVaddr() - system->vertexStartAddr2)/system->stride2)); 

            DPRINTF(RubySequencer, " Num addr 2, Is vertex cache address \n");
            return true;
        }
    }
    if ((system->enableTrackedAddr3 == 1) && (!system->isSparse)){ //fixme: it is assumed that "next" data structure is mapped to addr. range 3
        if (isVertexInAddrRange(pkt, 3)) { 
            
                unsigned int offset;
                if(system->iterCount%2 == 1)
                   offset=5; 
                else
                   offset=4; 

                pkt->setIsSparse(system->isSparse);
                pkt->setOffsetSpd(offset); 
                //pkt->setDestSpd(((pkt->getVaddr()  - system->vertexStartAddr3)/system->stride3) % m_numCPU); 
                //pkt->setSetSpd((pkt->getVaddr()  - system->vertexStartAddr3)/system->stride3); 
                unsigned int destId = destSPM(( pkt->getVaddr()- system->vertexStartAddr3)/system->stride3);
                pkt->setDestSpd(destId); 
                pkt->setSetSpd(setEntry((pkt->getVaddr() - system->vertexStartAddr3)/system->stride3)); 

                DPRINTF(RubySequencer, " Num addr 3, Is vertex cache address iter count %i offset %i\n", system->iterCount, offset);
                return true;
        }
    }
    if (system->enableTrackedAddr3_old == 1) { //fixme: it is assumed that "next" data structure is mapped to addr. range 3

        if (isVertexInAddrRange(pkt, 5)) { 
                unsigned int offset;
                if(system->iterCount%2 == 1)
                   offset=4; 
                else
                   offset=5; 
                
                pkt->setIsSparse(system->isSparse);
                pkt->setOffsetSpd(offset); 
                //pkt->setDestSpd(((pkt->getVaddr()  - system->vertexStartAddr3)/system->stride3) % m_numCPU); 
                //pkt->setSetSpd((pkt->getVaddr()  - system->vertexStartAddr3)/system->stride3); 
                unsigned int destId = destSPM(( pkt->getVaddr()- system->vertexStartAddr3_old)/system->stride3_old);
                pkt->setDestSpd(destId); 
                pkt->setSetSpd(setEntry((pkt->getVaddr() - system->vertexStartAddr3_old)/system->stride3_old)); 

                DPRINTF(RubySequencer, " Num addr 3 old, Is vertex cache address iter count %i offset %i\n", system->iterCount, offset);
                
                return true;
            }
    }
    if (system->enableTrackedAddr4 == 1) {
        if (isVertexInAddrRange(pkt, 4)){ 
            pkt->setIsSparse(system->isSparse);
            pkt->setOffsetSpd(3); 
            //pkt->setDestSpd(((pkt->getVaddr()   - system->vertexStartAddr4)/system->stride4) % m_numCPU); 
            //pkt->setSetSpd((pkt->getVaddr() - system->vertexStartAddr4)/system->stride4); 
            unsigned int destId = destSPM(( pkt->getVaddr()- system->vertexStartAddr4)/system->stride4);
            pkt->setDestSpd(destId); 
            pkt->setSetSpd(setEntry((pkt->getVaddr() - system->vertexStartAddr4)/system->stride4)); 



            DPRINTF(RubySequencer, " Num addr 4, Is vertex cache address \n");
            return true;
        }
    }
    
    return false;
}
/*
uint64_t
Sequencer::determineSet(PacketPtr pkt)
{
    if (isVertexInAddrRange(pkt, 1)) 
        return ((pkt->getVaddr() - system->vertexStartAddr1)/system->stride1); 
    else if (isVertexInAddrRange(pkt, 2)) 
        return ((pkt->getVaddr() - system->vertexStartAddr2)/system->stride2); 
    else if (isVertexInAddrRange(pkt, 3)) 
        return ((pkt->getVaddr() - system->vertexStartAddr3)/system->stride3);
 
    else if (isVertexInAddrRange(pkt, 4)) 
        return ((pkt->getVaddr() - system->vertexStartAddr4)/system->stride4);
 
    return 0;
}

int
Sequencer::determineOffset(PacketPtr pkt)
{
    if (isVertexInAddrRange(pkt, 1)) 
        return 0; 
    else if (isVertexInAddrRange(pkt, 2)) 
        return 1;
    else if (isVertexInAddrRange(pkt, 3)) 
        return 2;
 
    else if (isVertexInAddrRange(pkt, 4)) 
        return 3;
    
    return 0;
}

int
Sequencer::dest(PacketPtr pkt)
{
    if (isVertexInAddrRange(pkt, 1)) 
        return (((pkt->getVaddr() - system->vertexStartAddr1)/system->stride1) % m_numCPU);
    else if (isVertexInAddrRange(pkt, 2)) 
        return (((pkt->getVaddr() - system->vertexStartAddr2)/system->stride2) % m_numCPU);
    else if (isVertexInAddrRange(pkt, 3)) 
        return (((pkt->getVaddr() - system->vertexStartAddr3)/system->stride3) % m_numCPU);
    else if (isVertexInAddrRange(pkt, 4)) 
        return (((pkt->getVaddr() - system->vertexStartAddr4)/system->stride4) % m_numCPU);
   
    return 0;  
}
*/



bool
Sequencer::isVertexLocal(PacketPtr pkt)
{
    
    DPRINTF(RubySequencer, "IsVertexLocal vaddr:%x difference from start addr: %i numCPU: %i contextId: %i\n", pkt->getVaddr(), (pkt->getVaddr() - system->vertexStartAddr1), m_numCPU, pkt->req->contextId());
   
     
    if (isVertexInAddrRange(pkt, 1)) {
        DPRINTF(RubySequencer, "pkt is in address range 1\n");
        if (destSPM((pkt->getVaddr() - system->vertexStartAddr1)/system->stride1) == pkt->req->contextId())
            return true;
    }
    else if (isVertexInAddrRange(pkt, 2)) {
        DPRINTF(RubySequencer, "pkt is in address range 2\n");
        if (destSPM((pkt->getVaddr() - system->vertexStartAddr2)/system->stride2) == pkt->req->contextId())
            return true;
    }
    else if (isVertexInAddrRange(pkt, 3)) {
        DPRINTF(RubySequencer, "pkt is in address range 3\n");
        if (destSPM((pkt->getVaddr() - system->vertexStartAddr3)/system->stride3) == pkt->req->contextId())
            return true;
    }
    else if (isVertexInAddrRange(pkt, 5)) {
        DPRINTF(RubySequencer, "pkt is in address range 5 or old\n");
        if (destSPM((pkt->getVaddr() - system->vertexStartAddr3_old)/system->stride3_old) == pkt->req->contextId())
            return true;
    }
    

    else if (isVertexInAddrRange(pkt, 4)) {
        DPRINTF(RubySequencer, "pkt is in address range 4\n");
        if (destSPM((pkt->getVaddr() - system->vertexStartAddr4)/system->stride4) == pkt->req->contextId())
            return true;
    }
    else
       panic("Invalid address range choice");
 
    return false;

}

void
Sequencer::issueRequest(PacketPtr pkt, RubyRequestType secondary_type)
{
    assert(pkt != NULL);
    ContextID proc_id = pkt->req->hasContextId() ?
        pkt->req->contextId() : InvalidContextID;

    // If valid, copy the pc to the ruby request
    Addr pc = 0;
    if (pkt->req->hasPC()) {
        pc = pkt->req->getPC();
    }

    bool isVertex = false;
    //Abraham - line address and physical address are the same if vertex
    if((secondary_type == RubyRequestType_ST_Vertex_Local) ||
        (secondary_type == RubyRequestType_ST_Vertex_Remote) ||
        (secondary_type == RubyRequestType_Locked_Vertex_Read_Local) ||
        (secondary_type == RubyRequestType_Locked_Vertex_Read_Remote) ||
        (secondary_type == RubyRequestType_Locked_Vertex_Write_Local) ||
        (secondary_type == RubyRequestType_Locked_Vertex_Write_Remote) ||
        (secondary_type == RubyRequestType_PREFETCH_EDGE) ||
        (secondary_type == RubyRequestType_CHECK_ACTIVEVERTEX_Local) ||
        (secondary_type == RubyRequestType_CHECK_ACTIVEVERTEX_Remote) ||
        (secondary_type == RubyRequestType_LD_Vertex_Local) ||
        (secondary_type == RubyRequestType_LD_Vertex_Remote)) {
 
        isVertex = true; 
    }
    // check if the packet has data as for example prefetch and flush
    // requests do not
    std::shared_ptr<RubyRequest> msg =
        std::make_shared<RubyRequest>(clockEdge(), pkt->getAddr(),
                                      system->prefetchAddr, 
                                      isVertex,
                                      pkt->getIsAtomic(),
                                      pkt->getDestSpd(), 
                                      pkt->getSetSpd(),
                                      pkt->getOffsetSpd(),
                                      pkt->isFlush() ?
                                      nullptr : pkt->getPtr<uint8_t>(),
                                      pkt->getSize(), pc, secondary_type,
                                      RubyAccessMode_Supervisor, pkt,
                                      PrefetchBit_No, proc_id);

    DPRINTFR(ProtocolTrace, "%15s %3s %10s%20s %6s>%-6s %#x %s\n",
            curTick(), m_version, "Seq", "Begin", "", "",
            printAddress(msg->getPhysicalAddress()),
            RubyRequestType_to_string(secondary_type));

    // The Sequencer currently assesses instruction and data cache hit latency
    // for the top-level caches at the beginning of a memory access.
    // TODO: Eventually, this latency should be moved to represent the actual
    // cache access latency portion of the memory access. This will require
    // changing cache controller protocol files to assess the latency on the
    // access response path.
    Cycles latency(0);  // Initialize to zero to catch misconfigured latency

    //Abraham - adding priority to message: vertex access has lower priority than non-vertex
    //0 - high priority, 1 - low priority
    //not used for the time being
    int prio = 0;
    if (secondary_type == RubyRequestType_IFETCH)
        latency = m_inst_cache_hit_latency;
    else
        latency = m_data_cache_hit_latency;


     if((secondary_type == RubyRequestType_ST_Vertex_Local) ||
        (secondary_type == RubyRequestType_Locked_Vertex_Read_Local) ||
        (secondary_type == RubyRequestType_Locked_Vertex_Write_Local) ||
        (secondary_type == RubyRequestType_CHECK_ACTIVEVERTEX_Local) ||
        (secondary_type == RubyRequestType_LD_Vertex_Local) ||
        (secondary_type == RubyRequestType_Locked_Vertex_Read_Remote) ||
        (secondary_type == RubyRequestType_Locked_Vertex_Write_Remote) ||
        (secondary_type == RubyRequestType_CHECK_ACTIVEVERTEX_Remote) ||
        (secondary_type == RubyRequestType_PREFETCH_EDGE) ||
        (secondary_type == RubyRequestType_LD_Vertex_Remote)) 
            latency = m_vertex_cache_hit_latency;
     
     if(secondary_type == RubyRequestType_ST_Vertex_Local) {
        if(pkt->getIsAtomic()) {
            //int atomicOppType = (int)(system->atomicOppType1) - 48;
            //latency = Cycles(1 + latency_per_app[atomicOppType].second); // 1 account to vertex_cache_hit_latnecy
            
               for (auto i = latency_per_app.begin(); i != latency_per_app.end(); ++i) {
                    if (i->first == system->atomicOppType1) {
                        latency =  Cycles(i->second); 
                    }
                }
            
            DPRINTF(RubySequencer, "Latency for atomic instruction:%i | op = %c\n", latency, system->atomicOppType1);
        }    
    } 
    // Send the message to the cache controller
    assert(latency > 0);

    assert(m_mandatory_q_ptr != NULL);
    m_mandatory_q_ptr->enqueue(msg, clockEdge(), cyclesToTicks(latency), prio);
}

template <class KEY, class VALUE>
std::ostream &
operator<<(ostream &out, const std::unordered_map<KEY, VALUE> &map)
{
    auto i = map.begin();
    auto end = map.end();

    out << "[";
    for (; i != end; ++i)
        out << " " << i->first << "=" << i->second;
    out << " ]";

    return out;
}

void
Sequencer::print(ostream& out) const
{
    out << "[Sequencer: " << m_version
        << ", outstanding requests: " << m_outstanding_count
        << ", read request table: " << m_readRequestTable
        << ", write request table: " << m_writeRequestTable
        << "]";
}

// this can be called from setState whenever coherence permissions are
// upgraded when invoked, coherence violations will be checked for the
// given block
void
Sequencer::checkCoherence(Addr addr)
{
#ifdef CHECK_COHERENCE
    m_ruby_system->checkGlobalCoherenceInvariant(addr);
#endif
}

void
Sequencer::recordRequestType(SequencerRequestType requestType) {
    DPRINTF(RubyStats, "Recorded statistic: %s\n",
            SequencerRequestType_to_string(requestType));
}


void
Sequencer::evictionCallback(Addr address)
{
    ruby_eviction_callback(address);
}

void
Sequencer::regStats()
{
    //Abraham - evaluating read-only local buffer
    m_local_vertex_buffer_access
        .name(name() + ".local_vertex_buffer_access")
        .desc(" Number of accesses to read-only local buffer")
        .flags(Stats::nozero);

    m_store_waiting_on_load
        .name(name() + ".store_waiting_on_load")
        .desc("Number of times a store aliased with a pending load")
        .flags(Stats::nozero);
    m_store_waiting_on_store
        .name(name() + ".store_waiting_on_store")
        .desc("Number of times a store aliased with a pending store")
        .flags(Stats::nozero);
    m_load_waiting_on_load
        .name(name() + ".load_waiting_on_load")
        .desc("Number of times a load aliased with a pending load")
        .flags(Stats::nozero);
    m_load_waiting_on_store
        .name(name() + ".load_waiting_on_store")
        .desc("Number of times a load aliased with a pending store")
        .flags(Stats::nozero);

    // These statistical variables are not for display.
    // The profiler will collate these across different
    // sequencers and display those collated statistics.
    m_outstandReqHist.init(10);
    m_latencyHist.init(10);
    m_hitLatencyHist.init(10);
    m_missLatencyHist.init(10);

    for (int i = 0; i < RubyRequestType_NUM; i++) {
        m_typeLatencyHist.push_back(new Stats::Histogram());
        m_typeLatencyHist[i]->init(10);

        m_hitTypeLatencyHist.push_back(new Stats::Histogram());
        m_hitTypeLatencyHist[i]->init(10);

        m_missTypeLatencyHist.push_back(new Stats::Histogram());
        m_missTypeLatencyHist[i]->init(10);
    }

    for (int i = 0; i < MachineType_NUM; i++) {
        m_hitMachLatencyHist.push_back(new Stats::Histogram());
        m_hitMachLatencyHist[i]->init(10);

        m_missMachLatencyHist.push_back(new Stats::Histogram());
        m_missMachLatencyHist[i]->init(10);

        m_IssueToInitialDelayHist.push_back(new Stats::Histogram());
        m_IssueToInitialDelayHist[i]->init(10);

        m_InitialToForwardDelayHist.push_back(new Stats::Histogram());
        m_InitialToForwardDelayHist[i]->init(10);

        m_ForwardToFirstResponseDelayHist.push_back(new Stats::Histogram());
        m_ForwardToFirstResponseDelayHist[i]->init(10);

        m_FirstResponseToCompletionDelayHist.push_back(new Stats::Histogram());
        m_FirstResponseToCompletionDelayHist[i]->init(10);
    }

    for (int i = 0; i < RubyRequestType_NUM; i++) {
        m_hitTypeMachLatencyHist.push_back(std::vector<Stats::Histogram *>());
        m_missTypeMachLatencyHist.push_back(std::vector<Stats::Histogram *>());

        for (int j = 0; j < MachineType_NUM; j++) {
            m_hitTypeMachLatencyHist[i].push_back(new Stats::Histogram());
            m_hitTypeMachLatencyHist[i][j]->init(10);

            m_missTypeMachLatencyHist[i].push_back(new Stats::Histogram());
            m_missTypeMachLatencyHist[i][j]->init(10);
        }
    }
}
