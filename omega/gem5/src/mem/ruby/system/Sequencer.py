# Copyright (c) 2009 Advanced Micro Devices, Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Authors: Steve Reinhardt
#          Brad Beckmann

from m5.params import *
from m5.proxy import *
from MemObject import MemObject

class RubyPort(MemObject):
    type = 'RubyPort'
    abstract = True
    cxx_header = "mem/ruby/system/RubyPort.hh"
    version = Param.Int(0, "")

    slave = VectorSlavePort("CPU slave port")
    master = VectorMasterPort("CPU master port")
    pio_master_port = MasterPort("Ruby mem master port")
    mem_master_port = MasterPort("Ruby mem master port")
    pio_slave_port = SlavePort("Ruby pio slave port")
    mem_slave_port = SlavePort("Ruby memory port")

    using_ruby_tester = Param.Bool(False, "")
    ruby_system = Param.RubySystem(Parent.any, "")
    system = Param.System(Parent.any, "system object")
    support_data_reqs = Param.Bool(True, "data cache requests supported")
    support_inst_reqs = Param.Bool(True, "inst cache requests supported")

class RubyPortProxy(RubyPort):
    type = 'RubyPortProxy'
    cxx_header = "mem/ruby/system/RubyPortProxy.hh"

class RubySequencer(RubyPort):
    type = 'RubySequencer'
    cxx_class = 'Sequencer'
    cxx_header = "mem/ruby/system/Sequencer.hh"

    icache = Param.RubyCache("")
    dcache = Param.RubyCache("")
    vcache = Param.RubySPM("")

    configDataTypeSize1 = Param.Addr(0x300008, "address used by the app")
    configDataTypeSize2 = Param.Addr(0x300010, "address used by the app")
    configDataTypeSize3 = Param.Addr(0x300018, "address used by the app")
    configDataTypeSize4 = Param.Addr(0x300020, "address used by the app")
    
 
    configStride1 = Param.Addr(0x300028, "address used by the app")
    configStride2 = Param.Addr(0x300030, "address used by the app")
    configStride3 = Param.Addr(0x300038, "address used by the app")
    configStride4 = Param.Addr(0x300040, "address used by the app")
   
    configNumVertex = Param.Addr(0x300048, "address used by the app")
    
    configVertexStartAddr1 = Param.Addr(0x300058, "address used by the app")
    configVertexStartAddr2 = Param.Addr(0x300060, "address used by the app")
    configVertexStartAddr3 = Param.Addr(0x300068, "address used by the app")
    configVertexStartAddr4 = Param.Addr(0x300070, "address used by the app")
    
    configEnableTrackedAddr1 = Param.Addr(0x300078, "address used by the app")
    configEnableTrackedAddr2 = Param.Addr(0x300080, "address used by the app")
    configEnableTrackedAddr3 = Param.Addr(0x300088, "address used by the app")
    configEnableTrackedAddr4 = Param.Addr(0x300090, "address used by the app")
   
    #Prefetching related MMIO registers
    configEnablePrefetch = Param.Addr(0x300098, "address used by the app")
    configPrefetchAddr   = Param.Addr(0x3000a0, "address used by the app")
    configPrefetchSize   = Param.Addr(0x3000a8, "address used by the app")
   
    #PISC offloading related MMIO registers
    configOldValueAddr1 = Param.Addr(0x3000b0, "address used by the app")
    configOldValueAddr2 = Param.Addr(0x3000b8, "address used by the app")
    configAtomicStatusAddr1 = Param.Addr(0x3000c0, "address used by the app")
    configAtomicStatusAddr2 = Param.Addr(0x3000c8, "address used by the app")
    configEnableAtomicAddr   = Param.Addr(0x3000d0, "address used by the app y")
    configAtomicOppTypeAddr1 = Param.Addr(0x3000d8, "address used by the app")
    configAtomicOppTypeAddr2 = Param.Addr(0x3000e0, "address used by the app y")
    
    configNoStatusReadAddr = Param.Addr(0x3000e8, "address used by the app y")
    
    configNumMappedVertices = Param.Addr(0x3000f0, "address used by the app y")
    
    configNumOutEdge = Param.Addr(0x3000f8, "address used by the app")
    configNumTrackedAtomic = Param.Addr(0x300100, "address used by the app")
    configIsSparse = Param.Addr(0x300108, "address used by the app")
    
    configIsCopy = Param.Addr(0x300110, "address used by the app")
    
    configSrcInfo = Param.Addr(0x300118, "address used by the app")
    configDestInfo = Param.Addr(0x300120, "address used by the app")
    configAddInfo = Param.Addr(0x300128, "address used by the app")
    configEdgeIndex = Param.Addr(0x300130, "address used by the app")
    
    configChunkSize = Param.Addr(0x300138, "address used by the app")

    configActiveVerticesPerSpm = Param.Addr(0x300140, "address used by the app")
    configActiveVerticesTotal = Param.Addr(0x300148, "address used by the app")
    
    # Nicholas - Adding PISC support for Graphit-supported CAS operations
    configCasCompareInfo = Param.Addr(0x300150, "address used by the app")

    # Nicholas - Adding optional hardware deduplication
    configDedupEnabled = Param.Addr(0x300158, "address used by the app")

    # Nicholas - Adding profling interface
    configClearProfile = Param.Addr(0x300160, "address used by the app")
    configStartProfile = Param.Addr(0x300168, "address used by the app")
    configStopProfile  = Param.Addr(0x300170, "address used by the app")
    configDumpProfile  = Param.Addr(0x300178, "address used by the app")
    #
    configRangeStartProfile0 = Param.Addr(0x300180, "address used by the app")
    configRangeBytesProfile0 = Param.Addr(0x300188, "address used by the app")
    configRangeStartProfile1 = Param.Addr(0x300190, "address used by the app")
    configRangeBytesProfile1 = Param.Addr(0x300198, "address used by the app")
    configRangeStartProfile2 = Param.Addr(0x3001A0, "address used by the app")
    configRangeBytesProfile2 = Param.Addr(0x3001A8, "address used by the app")
    #
    configRangeStartProfile3 = Param.Addr(0x3001B0, "address used by the app")
    configRangeBytesProfile3 = Param.Addr(0x3001B8, "address used by the app")
    configRangeStartProfile4 = Param.Addr(0x3001C0, "address used by the app")
    configRangeBytesProfile4 = Param.Addr(0x3001C8, "address used by the app")
    configRangeStartProfile5 = Param.Addr(0x3001D0, "address used by the app")
    configRangeBytesProfile5 = Param.Addr(0x3001D8, "address used by the app")
    #
    configRangeStartProfile6 = Param.Addr(0x3001E0, "address used by the app")
    configRangeBytesProfile6 = Param.Addr(0x3001E8, "address used by the app")
    configRangeStartProfile7 = Param.Addr(0x3001F0, "address used by the app")
    configRangeBytesProfile7 = Param.Addr(0x3001F8, "address used by the app")
    configRangeStartProfile8 = Param.Addr(0x300200, "address used by the app")
    configRangeBytesProfile8 = Param.Addr(0x300208, "address used by the app")

    # Nicholas - Adding outstanding PISC op tracking
    configOutstandingPISCOps = Param.Addr(0x300210, "address used by the app")

    numCPU = Param.Unsigned(1, "Number of cores")
    # Cache latencies currently assessed at the beginning of each access
    # NOTE: Setting these values to a value greater than one will result in
    # O3 CPU pipeline bubbles and negatively impact performance
    # TODO: Latencies should be migrated into each top-level cache controller
    icache_hit_latency = Param.Cycles(1, "Inst cache hit latency")
    dcache_hit_latency = Param.Cycles(1, "Data cache hit latency") # Abraham - l1 cache has 8 assoc with cycles 2
    vertex_hit_latency = Param.Cycles(1, "Data cache hit latency")
    max_outstanding_requests = Param.Int(16,
        "max requests (incl. prefetches) outstanding")
    deadlock_threshold = Param.Cycles(500000,
        "max outstanding cycles for a request before deadlock/livelock declared")
    using_network_tester = Param.Bool(False, "")

class DMASequencer(MemObject):
    type = 'DMASequencer'
    cxx_header = "mem/ruby/system/DMASequencer.hh"

    version = Param.Int(0, "")
    slave = SlavePort("Device slave port")
    using_ruby_tester = Param.Bool(False, "")
    ruby_system = Param.RubySystem(Parent.any, "")
    system = Param.System(Parent.any, "system object")
