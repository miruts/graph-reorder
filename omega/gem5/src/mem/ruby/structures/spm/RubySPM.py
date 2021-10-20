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
from FreqCollReplacementPolicy import FreqCollReplacementPolicy
from m5.SimObject import SimObject

class RubySPM(SimObject):
    type = 'RubySPM'
    cxx_class = 'ScratchpadMemory'
    cxx_header = "mem/ruby/structures/spm/ScratchpadMemory.hh"
    size = Param.MemorySize("capacity in bytes");
    block_size = Param.Int(Parent.vertex_line_size, "Size of the block for the scratchpads");
    assoc = Param.Int("");
    replacement_policy = Param.ReplacementPolicySPM(FreqCollReplacementPolicy()
                         , "")
    start_index_bit = Param.Int(6, "index start, default 6 for 64-byte line");
    is_icache = Param.Bool(False, "is instruction only cache");
    is_vcache = Param.Bool(True, "is vertex only cache");
    
    #Abraham - adding source & destination scratchapds
    #The following addresses are hardwire into the scratchpads
    #and these addresses are assumed to be allocated by the apps
    #only for these purposes
    keyAddr = Param.Addr(0x300000, "address used by the app to write key");
    valueAddr = Param.Addr(0x300008, "address used by the app to write value");
    oppTypeAddr = Param.Addr(0x300010, "address used by the app to write " +
                  "type of operation");
    hashTypeAddr = Param.Addr(0x300018, "address used by the app to write " +
                   "type of hash function");
    keyTypeAddr = Param.Addr(0x300020, "address used by the app to write " +
                   "type of key");
    spillStartAddr = Param.Addr(0x300028, "address used by the app to write " +
                   "spilled key-value pairs");
    mapCompleteAddr = Param.Addr(0x300030, "address used by the app to write " +
                   "completion of map operation by the core");

    maxSpillAllocAddr = Param.Addr(0x300038, "address used by the app to write " +
                   "maximum allocated space for handling spilled key-value pairs");
 
    needSecHashingAddr = Param.Addr(0x300040, "address used by the app to write " +
                   "maximum allocated space for handling spilled key-value pairs");
 

    numCPU = Param.Unsigned(1, "Number of cores")
    
    dataArrayBanks = Param.Int(1, "Number of banks for the data array")
    tagArrayBanks = Param.Int(1, "Number of banks for the tag array")
    dataAccessLatency = Param.Cycles(1, "cycles for a data array access")
    tagAccessLatency = Param.Cycles(1, "cycles for a tag array access")
    resourceStalls = Param.Bool(False, "stall if there is a resource failure")
    ruby_system = Param.RubySystem(Parent.any, "")
    system = Param.System(Parent.any, "system object")
