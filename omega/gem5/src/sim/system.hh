/*
 * Copyright (c) 2012, 2014 ARM Limited
 * All rights reserved
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Copyright (c) 2002-2005 The Regents of The University of Michigan
 * Copyright (c) 2011 Regents of the University of California
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
 *
 * Authors: Steve Reinhardt
 *          Lisa Hsu
 *          Nathan Binkert
 *          Rick Strong
 */

#ifndef __SYSTEM_HH__
#define __SYSTEM_HH__

#include <string>
#include <utility>
#include <vector>

#include "arch/isa_traits.hh"
#include "base/loader/symtab.hh"
#include "base/misc.hh"
#include "base/statistics.hh"
#include "config/the_isa.hh"
#include "enums/MemoryMode.hh"
#include "mem/mem_object.hh"
#include "mem/port.hh"
#include "mem/port_proxy.hh"
#include "mem/physical.hh"
#include "params/System.hh"

// Nicholas
#include <unordered_map>

/**
 * To avoid linking errors with LTO, only include the header if we
 * actually have the definition.
 */
#if THE_ISA != NULL_ISA
#include "cpu/pc_event.hh"
#endif

class BaseCPU;
class BaseRemoteGDB;
class GDBListener;
class ObjectFile;
class Platform;
class ThreadContext;

class System : public MemObject
{
  private:

    /**
     * Private class for the system port which is only used as a
     * master for debug access and for non-structural entities that do
     * not have a port of their own.
     */
    class SystemPort : public MasterPort
    {
      public:

        /**
         * Create a system port with a name and an owner.
         */
        SystemPort(const std::string &_name, MemObject *_owner)
            : MasterPort(_name, _owner)
        { }
        bool recvTimingResp(PacketPtr pkt) override
        { panic("SystemPort does not receive timing!\n"); return false; }
        void recvReqRetry() override
        { panic("SystemPort does not expect retry!\n"); }
    };

    SystemPort _systemPort;

  public:

    /**
     * After all objects have been created and all ports are
     * connected, check that the system port is connected.
     */
    void init() override;

    /**
     * Get a reference to the system port that can be used by
     * non-structural simulation objects like processes or threads, or
     * external entities like loaders and debuggers, etc, to access
     * the memory system.
     *
     * @return a reference to the system port we own
     */
    MasterPort& getSystemPort() { return _systemPort; }

    /**
     * Additional function to return the Port of a memory object.
     */
    BaseMasterPort& getMasterPort(const std::string &if_name,
                                  PortID idx = InvalidPortID) override;

    /** @{ */
    /**
     * Is the system in atomic mode?
     *
     * There are currently two different atomic memory modes:
     * 'atomic', which supports caches; and 'atomic_noncaching', which
     * bypasses caches. The latter is used by hardware virtualized
     * CPUs. SimObjects are expected to use Port::sendAtomic() and
     * Port::recvAtomic() when accessing memory in this mode.
     */
    bool isAtomicMode() const {
        return memoryMode == Enums::atomic ||
            memoryMode == Enums::atomic_noncaching;
    }

    /**
     * Is the system in timing mode?
     *
     * SimObjects are expected to use Port::sendTiming() and
     * Port::recvTiming() when accessing memory in this mode.
     */
    bool isTimingMode() const {
        return memoryMode == Enums::timing;
    }

    /**
     * Should caches be bypassed?
     *
     * Some CPUs need to bypass caches to allow direct memory
     * accesses, which is required for hardware virtualization.
     */
    bool bypassCaches() const {
        return memoryMode == Enums::atomic_noncaching;
    }
    /** @} */

    /** @{ */
    /**
     * Get the memory mode of the system.
     *
     * \warn This should only be used by the Python world. The C++
     * world should use one of the query functions above
     * (isAtomicMode(), isTimingMode(), bypassCaches()).
     */
    Enums::MemoryMode getMemoryMode() const { return memoryMode; }

    /**
     * Change the memory mode of the system.
     *
     * \warn This should only be called by the Python!
     *
     * @param mode Mode to change to (atomic/timing/...)
     */
    void setMemoryMode(Enums::MemoryMode mode);
    /** @} */

    /**
     * Get the cache line size of the system.
     */
    unsigned int cacheLineSize() const { return _cacheLineSize; }
    unsigned int get_num_cpus() const { return _num_cpus; }
    
    
    void  invalidateReadOnlyCachedData() {
        for (int i = 0; i < _num_cpus ; i++) {
            if ((!readOnlyCachedData[i].empty())) {
                for (auto j = readOnlyCachedData[i].begin(); j != readOnlyCachedData[i].end(); ++j) {
                    delete [] j->second;
                }
                readOnlyCachedData[i].clear();
            }
        }
    } 


    //Abraham - adding vertex cache
    //unsigned int atomicConfig() const { return _global_atomic_enable; }
    //Scratchpad configuration parameters
    Addr vertexStartAddr1;
    Addr vertexStartAddr2;
    Addr vertexStartAddr3;
    Addr vertexStartAddr3_old;
    
    Addr vertexStartAddr4;
    Addr dataTypeSize1 = 0;
    Addr dataTypeSize2 = 0;
    Addr dataTypeSize3 = 0;
    Addr dataTypeSize4 = 0;
    Addr dataTypeSize3_old = 0;
    Addr enableTrackedAddr1 = 0;
    Addr enableTrackedAddr2 = 0;
    Addr enableTrackedAddr3 = 0;
    Addr enableTrackedAddr4 = 0;
    Addr enableTrackedAddr3_old = 0;
    int numVertex = 0;
    int stride1 = 1;
    int stride2 = 1;
    int stride3 = 1;
    int stride4 = 1;
    int stride3_old = 1;
   

    int numOutEdges = 0;

    int enablePrefetch = 0;
    Addr prefetchAddr = 0x0; 
    int prefetchSize = 0;

    //bool enableVertexCaching = false;
    //bool validVertexCaching = false;

    uint64_t atomicOldValue1;
    uint64_t atomicOldValue2;
    unsigned int enableAtomic;
    uint64_t atomicDataMapped1;
    uint64_t atomicDataMapped2;
    uint64_t atomicDataMappedAddr1;
    uint64_t atomicDataMappedAddr2;

    bool isSparse = true;
    bool isCopy = false;

    // Nicholas - Adding optional hardware deduplication
    bool dedupEnabled = false;

    // Nicholas - Adding oustanding PISC op tracking
    uint64_t outstandingPISCOps = 0;

    // Nicholas - Adding profile interface
    #define NW_PROFILE_COUNT 10
    #define NW_RANGE_COUNT   9
    Addr profileRangeStartAddr[NW_RANGE_COUNT] = {0};
    uint64_t profileRangeBytes[NW_RANGE_COUNT] = {0};

    typedef struct {
      uint64_t count  = 0;
      Tick latencySum = 0;

      void clear() {
        count      = 0;
        latencySum = 0;
      }
    } NW_Mem_Stats;

    typedef struct {
      bool running         = false;
      Tick startTick       = 0;
      Tick totalTicks      = 0;
      uint64_t startCount  = 0;
      NW_Mem_Stats memNoRange[NWMemAccessType_COUNT];
      NW_Mem_Stats memRange[NW_RANGE_COUNT][NWMemAccessType_COUNT];
      void clear() {
        startTick  = 0;
        totalTicks = 0;
        startCount = 0;
        for (int i = 0; i < NWMemAccessType_COUNT; i++) {
          memNoRange[i].clear();
          for (int j = 0; j < NW_RANGE_COUNT; j++) {
            memRange[j][i].clear();
          }
        }
      }
      void start() {
        assert(running == false);
        running = true;
        startTick = curTick();
        startCount++;
      }
      void stop() {
        assert(running == true);
        running = false;
        totalTicks += curTick() - startTick;
      }
      void logMemAccess(const Addr addr,    const NWMemAccessType accessType,
                        const Tick latency, const System *system) {
        // Assumes that tracked ranges will never overlap (a multi-range access will only contribute to one range's stats)
        // Only catches accesses that start in the range (those that just cross into it are likely rare)
        for (int i = 0; i < NW_RANGE_COUNT; i++) {
          if (addr >= system->profileRangeStartAddr[i] &&
              addr - system->profileRangeStartAddr[i] < system->profileRangeBytes[i]) {
            memRange[i][accessType].count++;
            memRange[i][accessType].latencySum += latency;
            return;
          }
        }
        // Use "NoRange" to track all other accesses
        memNoRange[accessType].count++;
        memNoRange[accessType].latencySum += latency;
      }
      std::string getMemAccessString() {
        std::string result = "\n$_$ NoRange:         Count, Tick Sum, Tick Mean\n";
        for (int i = 0; i < NWMemAccessType_COUNT; i++) {
          if (memNoRange[i].count != 0) {
            result += "$_$   " + NWMemAccessTypeStrings[i]     + ": "
                    + std::to_string(memNoRange[i].count)      + ", "
                    + std::to_string(memNoRange[i].latencySum) + ", "
                    + std::to_string(memNoRange[i].latencySum / (double)memNoRange[i].count) + "\n";
          }
        }
        for (int j = 0; j < NW_RANGE_COUNT; j++) {
          result += "$_$ Range" + std::to_string(j) + ":          Count, Tick Sum, Tick Mean\n";
          for (int i = 0; i < NWMemAccessType_COUNT; i++) {
          if (memRange[j][i].count != 0) {
              result += "$_$   " + NWMemAccessTypeStrings[i]      + ": "
                      + std::to_string(memRange[j][i].count)      + ", "
                      + std::to_string(memRange[j][i].latencySum) + ", "
                      + std::to_string(memRange[j][i].latencySum / (double)memRange[j][i].count) + "\n";
            }
          }
        }
        return result;
      }
    } NW_Profile_t;

    NW_Profile_t nwProfiles[NW_PROFILE_COUNT];

    void nwProfiles_logMemAccess(const Addr addr, const NWMemAccessType accessType, const Tick latency) {
      for (int i = 0; i < NW_PROFILE_COUNT; i++) {
        if (nwProfiles[i].running) {
          nwProfiles[i].logMemAccess(addr, accessType, latency, this);
        }
      }
    }

    char atomicOppType1;
    char atomicOppType2;
    bool noStatusRead = false;
    unsigned numTrackedAtomic;
    
    uint64_t chunkSize;
   
    unsigned int iterCount =0;
 
    // Nicholas - Switching to unordered_map for read-only cache data
    std::vector<std::unordered_map<Addr, uint8_t*>> readOnlyCachedData;
    //std::vector<std::vector<std::pair<Addr, uint8_t*>>> readOnlyCachedData;
    std::vector<bool> enableVCaching;
    std::vector<bool> validVCaching;

    std::vector<long> active_vertices_per_spm;
    long active_vertices_total =0;

#if THE_ISA != NULL_ISA
    PCEventQueue pcEventQueue;
#endif

    std::vector<ThreadContext *> threadContexts;
    int _numContexts;
    const bool multiThread;

    ThreadContext *getThreadContext(ContextID tid)
    {
        return threadContexts[tid];
    }

    int numContexts()
    {
        assert(_numContexts == (int)threadContexts.size());
        return _numContexts;
    }

    /** Return number of running (non-halted) thread contexts in
     * system.  These threads could be Active or Suspended. */
    int numRunningContexts();

    Addr pagePtr;

    uint64_t init_param;

    /** Port to physical memory used for writing object files into ram at
     * boot.*/
    PortProxy physProxy;

    /** kernel symbol table */
    SymbolTable *kernelSymtab;

    /** Object pointer for the kernel code */
    ObjectFile *kernel;

    /** Begining of kernel code */
    Addr kernelStart;

    /** End of kernel code */
    Addr kernelEnd;

    /** Entry point in the kernel to start at */
    Addr kernelEntry;

    /** Mask that should be anded for binary/symbol loading.
     * This allows one two different OS requirements for the same ISA to be
     * handled.  Some OSes are compiled for a virtual address and need to be
     * loaded into physical memory that starts at address 0, while other
     * bare metal tools generate images that start at address 0.
     */
    Addr loadAddrMask;

    /** Offset that should be used for binary/symbol loading.
     * This further allows more flexibily than the loadAddrMask allows alone in
     * loading kernels and similar. The loadAddrOffset is applied after the
     * loadAddrMask.
     */
    Addr loadAddrOffset;

  protected:
    uint64_t nextPID;

  public:
    uint64_t allocatePID()
    {
        return nextPID++;
    }

    /** Get a pointer to access the physical memory of the system */
    PhysicalMemory& getPhysMem() { return physmem; }

    /** Amount of physical memory that is still free */
    Addr freeMemSize() const;

    /** Amount of physical memory that exists */
    Addr memSize() const;

    /**
     * Check if a physical address is within a range of a memory that
     * is part of the global address map.
     *
     * @param addr A physical address
     * @return Whether the address corresponds to a memory
     */
    bool isMemAddr(Addr addr) const;

    /**
     * Get the architecture.
     */
    Arch getArch() const { return Arch::TheISA; }

     /**
     * Get the page bytes for the ISA.
     */
    Addr getPageBytes() const { return TheISA::PageBytes; }

    /**
     * Get the number of bits worth of in-page adress for the ISA.
     */
    Addr getPageShift() const { return TheISA::PageShift; }

  protected:

    PhysicalMemory physmem;

    Enums::MemoryMode memoryMode;

    const unsigned int _cacheLineSize;
    
    //Abraham - adding vertex cache
    const unsigned int _vertexLineSize;
    //Abraham - adding vertex cache
    const unsigned int _num_cpus;

    uint64_t workItemsBegin;
    uint64_t workItemsEnd;
    uint32_t numWorkIds;
    std::vector<bool> activeCpus;

    /** This array is a per-sytem list of all devices capable of issuing a
     * memory system request and an associated string for each master id.
     * It's used to uniquely id any master in the system by name for things
     * like cache statistics.
     */
    std::vector<std::string> masterIds;

  public:

   
   /** Request an id used to create a request object in the system. All objects
     * that intend to issues requests into the memory system must request an id
     * in the init() phase of startup. All master ids must be fixed by the
     * regStats() phase that immediately preceeds it. This allows objects in the
     * memory system to understand how many masters may exist and
     * appropriately name the bins of their per-master stats before the stats
     * are finalized
     */
    MasterID getMasterId(std::string req_name);

    /** Get the name of an object for a given request id.
     */
    std::string getMasterName(MasterID master_id);

    /** Get the number of masters registered in the system */
    MasterID maxMasters()
    {
        return masterIds.size();
    }

    void regStats() override;
    /**
     * Called by pseudo_inst to track the number of work items started by this
     * system.
     */
    uint64_t
    incWorkItemsBegin()
    {
        return ++workItemsBegin;
    }

    /**
     * Called by pseudo_inst to track the number of work items completed by
     * this system.
     */
    uint64_t
    incWorkItemsEnd()
    {
        return ++workItemsEnd;
    }

    /**
     * Called by pseudo_inst to mark the cpus actively executing work items.
     * Returns the total number of cpus that have executed work item begin or
     * ends.
     */
    int
    markWorkItem(int index)
    {
        int count = 0;
        assert(index < activeCpus.size());
        activeCpus[index] = true;
        for (std::vector<bool>::iterator i = activeCpus.begin();
             i < activeCpus.end(); i++) {
            if (*i) count++;
        }
        return count;
    }

    inline void workItemBegin(uint32_t tid, uint32_t workid)
    {
        std::pair<uint32_t,uint32_t> p(tid, workid);
        lastWorkItemStarted[p] = curTick();
    }

    void workItemEnd(uint32_t tid, uint32_t workid);

    /**
     * Fix up an address used to match PCs for hooking simulator
     * events on to target function executions.  See comment in
     * system.cc for details.
     */
    virtual Addr fixFuncEventAddr(Addr addr)
    {
        panic("Base fixFuncEventAddr not implemented.\n");
    }

    /** @{ */
    /**
     * Add a function-based event to the given function, to be looked
     * up in the specified symbol table.
     *
     * The ...OrPanic flavor of the method causes the simulator to
     * panic if the symbol can't be found.
     *
     * @param symtab Symbol table to use for look up.
     * @param lbl Function to hook the event to.
     * @param desc Description to be passed to the event.
     * @param args Arguments to be forwarded to the event constructor.
     */
    template <class T, typename... Args>
    T *addFuncEvent(const SymbolTable *symtab, const char *lbl,
                    const std::string &desc, Args... args)
    {
        Addr addr M5_VAR_USED = 0; // initialize only to avoid compiler warning

#if THE_ISA != NULL_ISA
        if (symtab->findAddress(lbl, addr)) {
            T *ev = new T(&pcEventQueue, desc, fixFuncEventAddr(addr),
                          std::forward<Args>(args)...);
            return ev;
        }
#endif

        return NULL;
    }

    template <class T>
    T *addFuncEvent(const SymbolTable *symtab, const char *lbl)
    {
        return addFuncEvent<T>(symtab, lbl, lbl);
    }

    template <class T, typename... Args>
    T *addFuncEventOrPanic(const SymbolTable *symtab, const char *lbl,
                           Args... args)
    {
        T *e(addFuncEvent<T>(symtab, lbl, std::forward<Args>(args)...));
        if (!e)
            panic("Failed to find symbol '%s'", lbl);
        return e;
    }
    /** @} */

    /** @{ */
    /**
     * Add a function-based event to a kernel symbol.
     *
     * These functions work like their addFuncEvent() and
     * addFuncEventOrPanic() counterparts. The only difference is that
     * they automatically use the kernel symbol table. All arguments
     * are forwarded to the underlying method.
     *
     * @see addFuncEvent()
     * @see addFuncEventOrPanic()
     *
     * @param lbl Function to hook the event to.
     * @param args Arguments to be passed to addFuncEvent
     */
    template <class T, typename... Args>
    T *addKernelFuncEvent(const char *lbl, Args... args)
    {
        return addFuncEvent<T>(kernelSymtab, lbl,
                               std::forward<Args>(args)...);
    }

    template <class T, typename... Args>
    T *addKernelFuncEventOrPanic(const char *lbl, Args... args)
    {
        T *e(addFuncEvent<T>(kernelSymtab, lbl,
                             std::forward<Args>(args)...));
        if (!e)
            panic("Failed to find kernel symbol '%s'", lbl);
        return e;
    }
    /** @} */

  public:
    std::vector<BaseRemoteGDB *> remoteGDB;
    std::vector<GDBListener *> gdbListen;
    bool breakpoint();

  public:
    typedef SystemParams Params;

  protected:
    Params *_params;

  public:
    System(Params *p);
    ~System();

    void initState() override;

    const Params *params() const { return (const Params *)_params; }

  public:

    /**
     * Returns the addess the kernel starts at.
     * @return address the kernel starts at
     */
    Addr getKernelStart() const { return kernelStart; }

    /**
     * Returns the addess the kernel ends at.
     * @return address the kernel ends at
     */
    Addr getKernelEnd() const { return kernelEnd; }

    /**
     * Returns the addess the entry point to the kernel code.
     * @return entry point of the kernel code
     */
    Addr getKernelEntry() const { return kernelEntry; }

    /// Allocate npages contiguous unused physical pages
    /// @return Starting address of first page
    Addr allocPhysPages(int npages);

    ContextID registerThreadContext(ThreadContext *tc,
                                    ContextID assigned = InvalidContextID);
    void replaceThreadContext(ThreadContext *tc, ContextID context_id);

    void serialize(CheckpointOut &cp) const override;
    void unserialize(CheckpointIn &cp) override;

    void drainResume() override;

  public:
    Counter totalNumInsts;
    EventQueue instEventQueue;
    std::map<std::pair<uint32_t,uint32_t>, Tick>  lastWorkItemStarted;
    std::map<uint32_t, Stats::Histogram*> workItemStats;

    ////////////////////////////////////////////
    //
    // STATIC GLOBAL SYSTEM LIST
    //
    ////////////////////////////////////////////

    static std::vector<System *> systemList;
    static int numSystemsRunning;

    static void printSystems();

    // For futex system call
    std::map<uint64_t, std::list<ThreadContext *> * > futexMap;

  protected:

    /**
     * If needed, serialize additional symbol table entries for a
     * specific subclass of this sytem. Currently this is used by
     * Alpha and MIPS.
     *
     * @param os stream to serialize to
     */
    virtual void serializeSymtab(CheckpointOut &os) const {}

    /**
     * If needed, unserialize additional symbol table entries for a
     * specific subclass of this system.
     *
     * @param cp checkpoint to unserialize from
     * @param section relevant section in the checkpoint
     */
    virtual void unserializeSymtab(CheckpointIn &cp) {}

};

void printSystems();

#endif // __SYSTEM_HH__
