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

#include "base/intmath.hh"
#include "debug/RubySPM.hh"
#include "debug/RubySPMTrace.hh"
#include "debug/RubyResourceStalls.hh"
#include "debug/RubyStats.hh"
#include "mem/protocol/AccessPermission.hh"
#include "mem/ruby/structures/spm/ScratchpadMemory.hh"
#include "mem/ruby/system/RubySystem.hh"
#include <limits.h>
#include "sim/system.hh"

using namespace std;

ostream&
operator<<(ostream& out, const ScratchpadMemory& obj)
{
    obj.print(out);
    out << flush;
    return out;
}

ScratchpadMemory *
RubySPMParams::create()
{
    return new ScratchpadMemory(this);
}

ScratchpadMemory::ScratchpadMemory(const Params *p)
    : SimObject(p), system(p->system),
        dataArray(p->dataArrayBanks, p->dataAccessLatency,
                  p->start_index_bit, p->ruby_system),
        tagArray(p->tagArrayBanks, p->tagAccessLatency,
                 p->start_index_bit, p->ruby_system)
    {
        m_spm_size = p->size;
        m_spm_assoc = p->assoc;
        m_replacementPolicy_ptr = p->replacement_policy;
        m_replacementPolicy_ptr->setCache(this);
        m_start_index_bit = p->start_index_bit;
        m_is_instruction_only_cache = p->is_icache;
        m_is_vertex_only_cache = p->is_vcache;
        m_resource_stalls = p->resourceStalls;
        m_keyAddr = p->keyAddr; 
        m_valueAddr = p->valueAddr;
        m_oppTypeAddr = p->oppTypeAddr;
        m_hashTypeAddr = p->hashTypeAddr;
        m_keyTypeAddr = p->keyTypeAddr;
        m_spillStartAddr = p->spillStartAddr;
        m_maxSpillAllocAddr = p->maxSpillAllocAddr;
        m_mapCompleteAddr = p->mapCompleteAddr;
        m_needSecHashingAddr = p->needSecHashingAddr;
       
        m_numCPU = p->numCPU;
           
        m_block_size = p->block_size;
        //setting default value from parameters
        hashType = 'm'; //p->defaultHashType;
        oppType = 'i'; //p->defaultOppType;
    }

    void
    ScratchpadMemory::init()
    {
       //Abraham - adding source and destination scratchpads
       //replacing RubySystem::getBlockSizeBytes() with valueSize
       //the former doesn't apply to the scratchpads. It is multiplied
       //by 2 because apps like avg require two values
        m_spm_num_sets = (m_spm_size / m_spm_assoc) /
            (m_block_size) + maxChunkSize; //Abraham substracting to account for chunk based scheduling

            //RubySystem::getBlockSizeBytes();
        assert(m_spm_num_sets > 1);
        m_spm_num_set_bits = floorLog2(m_spm_num_sets);
        assert(m_spm_num_set_bits > 0);

        m_spm.resize(m_spm_num_sets);
        for (int i = 0; i < m_spm_num_sets; i++) {
            m_spm[i].resize(m_spm_assoc);
            for (int j = 0; j < m_spm_assoc; j++) {
                m_spm[i][j] = NULL;
            }
        }
        
        numAccelCompleted = 0; 
       
        
        DPRINTF(RubySPM, "number of lines: %i\n", m_spm_num_sets); 
    }

    ScratchpadMemory::~ScratchpadMemory()
    {
        if (m_replacementPolicy_ptr != NULL)
            delete m_replacementPolicy_ptr;
        for (int i = 0; i < m_spm_num_sets; i++) {
            for (int j = 0; j < m_spm_assoc; j++) {
                delete m_spm[i][j];
            }
        }
    }

    /**
     * Abraham - mrnitro - adding source & destination scratchpads
     * Count the number of accelerator that finish execution
     */
    void
    ScratchpadMemory::incrNumAccelCompleted()
    {
        numAccelCompleted++; 
    }

    /**
     * Abraham - mrnitro - adding source & destination scratchpads
     * the input number
     */
    int 
    ScratchpadMemory::primeVersion() const
    {
        for (unsigned i = m_spm_num_sets; i > 0; i--) {
            for (unsigned j = i-1; j > 0; j--) {
                if (j == 1)
                    return i;
                else if ((i%j) == 0) {
                    break;
                }
            }
        }
        return 1;
    }

    /**
     * Abraham - mrnitro - adding source & destination scratchpads
     * Find the destination acceleration incase of collision at the
     * source scrathcpad
     */
    int 
    ScratchpadMemory::maxNumAccel()
    {
        return (m_numCPU - 1); 
    }

    /**
     * Abraham - mrnitro - adding source & destination scratchpads
     * Find the destination acceleration incase of collision at the
     * source scrathcpad
     */
    int 
    ScratchpadMemory::hashDest(Addr addr)
    {
        DPRINTF(RubySPM, "m_numCPU: %i dest: %i", m_numCPU, addr % m_numCPU); 
        return (addr % m_numCPU); 
    }

    /**
     * Abraham - mrnitro - adding source & destination scratchpads
     * Implement the modulo hash function from the address.
     * @todo make the 251(line number) getting from the parameters.
     * @return The set index of the address.
     */
    int64_t 
    ScratchpadMemory::moduloHash(Addr addr) const 
    {
        //Adding the modulo hash function
        //long(addr) has been changed to addr
        //shift by number of cpus as we have already used that to identify the home scratchpads
        DPRINTF(RubySPM, "m_numCPU: %i line address: %i number of lines: %i\n", m_numCPU, (addr), m_spm_num_sets); 
        //not m_numCPU rather the amount of bits for representing this value 
        //return ((addr >> m_numCPU) %  m_spm_num_sets);
        //return ((addr ) %  m_spm_num_sets);
        //return (addr / m_numCPU);
        return (addr);
}

/**
 * Abraham - mrnitro - adding source scratchpad
 * Implement the simple XOR hash function from the address.
 * @todo make the number of bits used either configurable or get them directly from parameters.
 * @return The set index of the address.
 */
int64_t 
ScratchpadMemory::simpleXORHash(Addr addr) const
{
    //Adding the modulo hash function
    //return ((addr >> setShift) & setMask);
    int res = 0;
    for (int i =0; i< 8; i++) {
        res ^= (addr & 0x00000000000000FF);
        addr = addr >> 8;
    }
    return res;
}

/**
 * Abraham - mrnitro - adding source scratchpad
 * Implement the XOR rotate shift hash function from the address.
 * @todo make the number of bits used either configurable or get them directly
 * from parameters.
 * @todo verify the code works 
 * @return The set index of the address.
 */
int64_t 
ScratchpadMemory::rotateXORHash(Addr addr) const 
{
    //Adding the modulo hash function
    //return ((addr >> setShift) & setMask);
    uint8_t res = 0;
    auto rot = [] (uint8_t v, uint8_t bits) { return (v>>bits) |
                    (v<<(8*sizeof(uint8_t)-bits)); };
    
    for (int i =0; i< 8; i++) {
        res ^= (addr & 0x00000000000000FF);
        addr = addr >> 8; //8 - maximum number of bits used to identify the
                          //line number
        res = rot(res, 5); //rotating by prime integer that is less than 8
    
    }
    return res;
}

/**
 * Calculate the set index from the address.
 * @param addr The address to get the set from.
 * @return The set index of the address.
 */
int64_t 
ScratchpadMemory::addressToCacheSet(Addr addr) const 
{
    DPRINTF(RubySPM, "hash: %c\n", hashType);
    //Abraham - mrnitro - adding source scratchpad
    //Adding the modulo hash function
    
    //return ((addr >> setShift) & setMask);
    switch (hashType) {
        case 'm':
            return moduloHash(addr);    
            break;
        case 'x':
            return rotateXORHash(addr); 
            break;
        case 'r':
            return simpleXORHash(addr); 
            break;

        default :
            panic("Invalid type of hash function type\n");
    }
}

/**
* Abraham - PISC engine
* Implement atomic operations -- could be either addition(+), 
* increament(i), min(<) or max(>), case
*/

void
ScratchpadMemory::atomicExecution(PacketPtr pkt, DataBlock& blk, int offset) 
{
    // Nicholas - Graphit [currently] supports atomic-min, atomic-sum, and compare-and-swap operations
    // Both atomic-min and atomic-sum support {int, float, double} only
    // GraphIt only produces 32-bit CAS ops

    uint8_t success = 0;
    //Update the new value 
    int offsetByte_value;
    //offset for valid bit is stored after the 32 bytes
    //alternatively we can have a separate valid/invalid bit
    int offsetByte_valid = 4*8;
   
    if ((system->iterCount)%2 ==1)
        offsetByte_valid = 5*8;
    
    switch (pkt->getAtomicOppType()) {
        case '1': //PageRank - double addition operation 
        {
            DPRINTF(RubySPM, "PageRank is running\n");

            offsetByte_value = 0;

            double blkValue;
            memcpy(&blkValue, blk.getData(offsetByte_value, 8),8);
            double pktValue = *(double*)(pkt->getSrcInfo());
            
            //Addition operation always successful
            success = 1; 
            
            //update the active vertices
            system->active_vertices_per_spm[pkt->getDestSpd()]++;
            system->active_vertices_total++;
            
            double res = pktValue + blkValue;
            
            uint8_t* data = static_cast<uint8_t*>(static_cast<void*>(&res)); 
            for (unsigned i = 0; i < 8; ++i) {
                blk.setByte(i + offsetByte_value, data[i]);
            }
            
            DPRINTF(RubySPM, "Success in atomic execution: blkvalue stored in %f  new pkt value %f res %f\n", blkValue, pktValue,  res);
           
            if(pkt->getIsSparse()) {
                //blk.setByte(offsetByte_valid, success);
                setSparseActiveVertex(pkt, success); 
            }
            else {
                if(success)
                    blk.setByte(offsetByte_valid, success);
            }
        }
            break;
        case '2': //BFS
        {
            DPRINTF(RubySPM, "BFS is running\n");

            offsetByte_value = 0;

            unsigned int blkValue;
            memcpy(&blkValue, blk.getData(offsetByte_value, 4), 4);
            
            unsigned int pktValue = *(unsigned int*)(pkt->getSrcInfo());
            
            unsigned int res;
            if ( blkValue == UINT_MAX) { 
                res = pktValue;
                success = 1;
                
                //update the active vertices
                system->active_vertices_per_spm[pkt->getDestSpd()]++;
                system->active_vertices_total++;
            
                DPRINTF(RubySPM, "Success in atomic execution\n");
            }
            else {
                res =   blkValue;
                DPRINTF(RubySPM, "No success in atomic execution\n");
            } 
           
            DPRINTF(RubySPM, "blkvalue %u res %u\n", blkValue, res);
            
            //Update the new value 
            uint8_t* data = static_cast<uint8_t*>(static_cast<void*>(&res)); 
            for (unsigned i = 0; i < sizeof(unsigned int); ++i) {
                blk.setByte(i + offsetByte_value, data[i]);
            }

            //offset for valid bit is stored after the 32 bytes
            //alternatively we can have a separate valid/invalid bit
            if(pkt->getIsSparse()) {
                //blk.setByte(offsetByte_valid, success);
                setSparseActiveVertex(pkt, success); 
            }
            else {
                if(success)
                    blk.setByte(offsetByte_valid, success);
            }
            DPRINTF(RubySPM, "blkvalue at set %i  and offset %i is %u\n", pkt->getSetSpd(), offsetByte_valid, blk.getByte(offsetByte_valid));

        }
            break;
        
        case '3': //BellmanFord
        { 
            
            DPRINTF(RubySPM, "BellmanFord is running\n");
            
            //offset for valid bit is stored after the 32 bytes
            //alternatively we can have a separate valid/invalid bit
            
            offsetByte_value = 0;
            int blkValue1;
            int blkValue2;
            
            memcpy(&blkValue1, blk.getData(offsetByte_value, 4), 4);
            memcpy(&blkValue2, blk.getData(offsetByte_value + 8, 4), 4);
            
            //int pktValue;
            //memcpy(&pktValue, pkt->getSrcInfo(), 4);
            int pktValue = *(int*)(pkt->getSrcInfo());
            
            int res1 = blkValue1;
            int res2 = blkValue2;
            if (blkValue1 > pktValue) { 
                res1 = pktValue;
                if ( blkValue2 == 0) { 
                    res2 = 1;
                    success = 1;
                    
                    //update the active vertices
                    system->active_vertices_per_spm[pkt->getDestSpd()]++;
                    system->active_vertices_total++;
                    
                    DPRINTF(RubySPM, "successful atomic operation\n");
                }
                else {
                    DPRINTF(RubySPM, "not successful atomic operation\n");
                } 
            }
            else {
                DPRINTF(RubySPM, "not successful atomic operation\n");
                res1 =   blkValue1;
            } 
            DPRINTF(RubySPM, "blkvalue1 %i res1 %i newValue %i\n", blkValue1, res1, pktValue);
            //Update the new value 
            uint8_t* data1 = static_cast<uint8_t*>(static_cast<void*>(&res1)); 
            for (unsigned i = 0; i < sizeof(int); ++i) {
                blk.setByte(i + offsetByte_value, data1[i]);
            }

            DPRINTF(RubySPM, "blkvalue2 %i res2 %i\n", blkValue2, res2);
            
            //Update the new value 
            uint8_t* data2 = static_cast<uint8_t*>(static_cast<void*>(&res2)); 
            for (unsigned i = 0; i < sizeof(int); ++i) {
                blk.setByte(i + offsetByte_value + 8, data2[i]);
            }
            
            //offset for valid bit is stored after the 32 bytes
            //alternatively we can have a separate valid/invalid bit
            if(pkt->getIsSparse()) {
                //blk.setByte(offsetByte_valid, success);
                setSparseActiveVertex(pkt, success); 
            }
            else {
                if(success)
                    blk.setByte(offsetByte_valid, success);
            }

        }
            break;
        
         case '4': //BC
        { 
        
            DPRINTF(RubySPM, "BC is running\n");
            offsetByte_value = 0;

            double blkValue1;
            memcpy(&blkValue1, blk.getData(offsetByte_value, 8), 8);
            double pktValue = *(double*)(pkt->getSrcInfo());
            
            double res = pktValue + blkValue1;

            bool blkValue2;
            memcpy(&blkValue2, blk.getData(offsetByte_value + 8, 1), 1);
            
            if (!blkValue2) {
                uint8_t* data = static_cast<uint8_t*>(static_cast<void*>(&res)); 
                for (unsigned i = 0; i < 8; ++i) {
                    blk.setByte(i + offsetByte_value, data[i]);
                }
            }
            //Check if old value is 0.0
            if (blkValue1 == 0.0) {
                success = 1; 
            
                //update the active vertices
                system->active_vertices_per_spm[pkt->getDestSpd()]++;
                system->active_vertices_total++;
            }
           
            DPRINTF(RubySPM, "Success in atomic execution: blkvalue2 stored %f  new pkt value %f res %f\n", blkValue1, pktValue, res);
            
            if(pkt->getIsSparse()) {
                //blk.setByte(offsetByte_valid, success);
                setSparseActiveVertex(pkt, success); 
            }
            else {
                if(success)
                    blk.setByte(offsetByte_valid, success);
            }
        } 
            break;
        case '5': //Components
        { 
        
            DPRINTF(RubySPM, "Components is running\n");
        
            int offsetByte_value1 = 0;
            int offsetByte_value2 = 8;
            unsigned int blkValue1;
            unsigned int blkValue2;
            
            memcpy(&blkValue1, blk.getData(offsetByte_value1, 4), 4);
            memcpy(&blkValue2, blk.getData(offsetByte_value2, 4), 4);
            
            unsigned int pktValue = *(unsigned int*)(pkt->getSrcInfo());
            
            unsigned int res;
            if (pktValue < blkValue1) { 
                
                if( blkValue1 == blkValue2){
                    success = 1; 
                            
                    //update the active vertices
                    system->active_vertices_per_spm[pkt->getDestSpd()]++;
                    system->active_vertices_total++;

                }
                res = pktValue;
                DPRINTF(RubySPM, "successful min atomic operation\n");
            }
            else {
                DPRINTF(RubySPM, "not successful min atomic operation\n");
                res =   blkValue1;
            } 
            
            DPRINTF(RubySPM, "blkvalue1 %i res %i newValue %i\n", blkValue1, res, pktValue);
           

            //Update the new value 
            uint8_t* data = static_cast<uint8_t*>(static_cast<void*>(&res)); 
            for (unsigned i = 0; i < sizeof(int); ++i) {
                blk.setByte(i + offsetByte_value1, data[i]);
            }

            //offset for valid bit is stored after the 32 bytes
            //alternatively we can have a separate valid/invalid bit
            if(pkt->getIsSparse()) {
                //blk.setByte(offsetByte_valid, success);
                setSparseActiveVertex(pkt, success); 
            }
            else {
                if(success)
                    blk.setByte(offsetByte_valid, success);
            }
        } 
            break;

        case '6': //Radii
        {
        
            DPRINTF(RubySPM, "Radii is running\n");
            offsetByte_value = 0;

            int radii;
            long Visited;
            long NextVisited;
            
            memcpy(&radii, blk.getData(offsetByte_value, 4), 4);
            memcpy(&Visited, blk.getData(offsetByte_value + 8, sizeof(long)), sizeof(long));
            memcpy(&NextVisited, blk.getData(offsetByte_value + 3*8, sizeof(long)), sizeof(long));
            
            long pktValue = *(long*)(pkt->getSrcInfo());
            int round = pkt->getAddInfo();
            
        
            long toWrite = Visited | pktValue;
            if(Visited != toWrite) {
                NextVisited |= toWrite;
                if(radii != round) { 
                    radii = round; 
                    success = 1; 
                
                    //update the active vertices
                    system->active_vertices_per_spm[pkt->getDestSpd()]++;
                    system->active_vertices_total++;


                }
            }
            int res1 = radii;
            long res2 = NextVisited;
                        
            uint8_t* data1 = static_cast<uint8_t*>(static_cast<void*>(&res1)); 
            for (unsigned i = 0; i < 4; ++i) {
                blk.setByte(i + offsetByte_value, data1[i]);
            }
            
            uint8_t* data2 = static_cast<uint8_t*>(static_cast<void*>(&res2)); 
            for (unsigned i = 0; i < sizeof(long); ++i) {
                blk.setByte(i + offsetByte_value + 3*8, data2[i]);
            }
           
            DPRINTF(RubySPM, "Success in atomic execution: blkvalue stored in uint64_t %u  new pkt value %u\n", (uint64_t)*blk.getData(offsetByte_value, pkt->getSize()), (uint64_t)*(pkt->getConstPtr<uint8_t>()));
            
            if(pkt->getIsSparse()) {
                //blk.setByte(offsetByte_valid, success);
                setSparseActiveVertex(pkt, success); 
            }
            else {
                if(success)
                    blk.setByte(offsetByte_valid, success);
            }
        }        
            break;

        case '7': //KCore
        {
            DPRINTF(RubySPM, "KCore is running\n");
            offsetByte_value = 0;

            int blkValue;
            memcpy(&blkValue, blk.getData(offsetByte_value, 4), 4);
            int pktValue = *(int*)(pkt->getSrcInfo());
            
            //Addition operation always successful
            success = 1; 


            //update the active vertices
            system->active_vertices_per_spm[pkt->getDestSpd()]++;
            system->active_vertices_total++;


            int res = pktValue + blkValue;
           
            uint8_t* data = static_cast<uint8_t*>(static_cast<void*>(&res)); 
            for (unsigned i = 0; i < 4; ++i) {
                blk.setByte(i + offsetByte_value, data[i]);
            }
            
            
            DPRINTF(RubySPM, "Success in atomic execution: blkvalue stored %i new pkt value %i\n", blkValue, pktValue);
            
            if(pkt->getIsSparse()) {
                //blk.setByte(offsetByte_valid, success);
                setSparseActiveVertex(pkt, success); 
            }
            else {
                if(success)
                    blk.setByte(offsetByte_valid, success);
            }
        }        
            break;
        
        case '8': //Triangle- long addition operation 
        {
            DPRINTF(RubySPM, "PageRank is running\n");

            offsetByte_value = 0;

            long blkValue;
            memcpy(&blkValue, blk.getData(offsetByte_value, 8),8);
            long pktValue = *(long*)(pkt->getSrcInfo());
            
            //Addition operation always successful
            success = 1; 
            
            //update the active vertices
            system->active_vertices_per_spm[pkt->getDestSpd()]++;
            system->active_vertices_total++;
            
            long res = pktValue + blkValue;
            
            uint8_t* data = static_cast<uint8_t*>(static_cast<void*>(&res)); 
            for (unsigned i = 0; i < 8; ++i) {
                blk.setByte(i + offsetByte_value, data[i]);
            }
            
            DPRINTF(RubySPM, "Success in atomic execution: blkvalue stored in %i  new pkt value %i res %i\n", blkValue, pktValue,  res);
           
            if(pkt->getIsSparse()) {
                //blk.setByte(offsetByte_valid, success);
                setSparseActiveVertex(pkt, success); 
            }
            else {
                if(success)
                    blk.setByte(offsetByte_valid, success);
            }
        }
            break;
        //GraphMat implemenation
        case 'a': //PageRank - double addition operation 
        {
            DPRINTF(RubySPM, "PageRank is running\n");

            offsetByte_value = 0;

            double blkValue;
            memcpy(&blkValue, blk.getData(offsetByte_value, 8),8);
            double pktValue = *(double*)(pkt->getSrcInfo());
            
            double res; 
            if (blk.getByte(offsetByte_valid)) {
                
                res = pktValue + blkValue;
           } 
           else {
                //Addition operation always successful
                success = 1; 
            
                //update the active vertices
                system->active_vertices_per_spm[pkt->getDestSpd()]++;
                system->active_vertices_total++;
                
                res = pktValue;
           }
            
            uint8_t* data = static_cast<uint8_t*>(static_cast<void*>(&res)); 
            for (unsigned i = 0; i < 8; ++i) {
                blk.setByte(i + offsetByte_value, data[i]);
            }
            
            DPRINTF(RubySPM, "Success in atomic execution: blkvalue stored in %f  new pkt value %f res %f\n", blkValue, pktValue,  res);
           
            if(pkt->getIsSparse()) {
                //blk.setByte(offsetByte_valid, success);
                setSparseActiveVertex(pkt, success); 
            }
            else {
                if(success)
                    blk.setByte(offsetByte_valid, success);
            }
        }
            break;
        case 'b': //BFS
        {
            DPRINTF(RubySPM, "BFS is running\n");

            offsetByte_value = 0;

            unsigned long long int blkValue;
            unsigned int size = sizeof(unsigned long long int);
            memcpy(&blkValue, blk.getData(offsetByte_value, size), size);
            
            unsigned long long int pktValue = *(unsigned long long int*)(pkt->getSrcInfo());
            unsigned long long int res;
            if (blk.getByte(offsetByte_valid)) {
                
                res = pktValue;
           } 
           else {
                //Addition operation always successful
                success = 1; 
            
                //update the active vertices
                system->active_vertices_per_spm[pkt->getDestSpd()]++;
                system->active_vertices_total++;
                
                res = pktValue;
           }

            
            DPRINTF(RubySPM, "blkvalue %u res %u\n", blkValue, res);
            
            //Update the new value 
            uint8_t* data = static_cast<uint8_t*>(static_cast<void*>(&res)); 
            for (unsigned i = 0; i < size; ++i) {
                blk.setByte(i + offsetByte_value, data[i]);
            }

            //offset for valid bit is stored after the 32 bytes
            //alternatively we can have a separate valid/invalid bit
            if(pkt->getIsSparse()) {
                //blk.setByte(offsetByte_valid, success);
                setSparseActiveVertex(pkt, success); 
            }
            else {
                if(success)
                    blk.setByte(offsetByte_valid, success);
            }
            DPRINTF(RubySPM, "blkvalue at set %i  and offset %i is %u size %i\n", pkt->getSetSpd(), offsetByte_valid, blk.getByte(offsetByte_valid), size);

        }
            break;
        
        case 'c': //BellmanFord
        { 
            
            DPRINTF(RubySPM, "BellmanFord is running\n");
            
            //offset for valid bit is stored after the 32 bytes
            //alternatively we can have a separate valid/invalid bit
            
            offsetByte_value = 0;
            unsigned int blkValue;
            
            memcpy(&blkValue, blk.getData(offsetByte_value, 4), 4);
            
            //int pktValue;
            //memcpy(&pktValue, pkt->getSrcInfo(), 4);
            unsigned int pktValue = *(unsigned int*)(pkt->getSrcInfo());
            
            unsigned int res;
            if (blk.getByte(offsetByte_valid)) {
                
                res = (blkValue <= pktValue) ? blkValue : pktValue;
           } 
           else {
                //Addition operation always successful
                success = 1; 
            
                //update the active vertices
                system->active_vertices_per_spm[pkt->getDestSpd()]++;
                system->active_vertices_total++;
                
                res = pktValue;
           }
            
            DPRINTF(RubySPM, "blkvalue1 %i res %i newValue %i\n", blkValue, res, pktValue);
            //Update the new value 
            uint8_t* data = static_cast<uint8_t*>(static_cast<void*>(&res)); 
            for (unsigned i = 0; i < sizeof(unsigned int); ++i) {
                blk.setByte(i + offsetByte_value, data[i]);
            }

            //offset for valid bit is stored after the 32 bytes
            //alternatively we can have a separate valid/invalid bit
            if(pkt->getIsSparse()) {
                //blk.setByte(offsetByte_valid, success);
                setSparseActiveVertex(pkt, success); 
            }
            else {
                if(success)
                    blk.setByte(offsetByte_valid, success);
            }

        }
            break;

        // Nicholas - adding Graphit-supported atomic operations
        // Nicholas - TODO: cleanup
        case 'd': //Graphit: ATOMIC_SUM, int
        {
            DPRINTF(RubySPM, "Graphit: ATOMIC_SUM, int\n");

            offsetByte_value = 0;

            int blkValue;
            memcpy(&blkValue, blk.getData(offsetByte_value, 4),4);
            int pktValue = *(int*)(pkt->getSrcInfo());
            
            // Addition operation always successful
            success = 1; 

            // Update the active vertices
            system->active_vertices_per_spm[pkt->getDestSpd()]++;
            system->active_vertices_total++;
            
            int res = pktValue + blkValue;
            
            uint8_t* data = static_cast<uint8_t*>(static_cast<void*>(&res)); 
            for (unsigned i = 0; i < 4; ++i) {
                blk.setByte(i + offsetByte_value, data[i]);
            }
            
            DPRINTF(RubySPM, "Success in atomic execution: blkvalue stored in %i  new pkt value %i res %i\n", blkValue, pktValue,  res);
           
            // Nicholas - Adding deduplication active set update logic
            if(pkt->getIsSparse()) {
                if (success) {
                    if (pkt->getDedupEnabled()) {
                        // Deduplication is enabled, check if this vertex has already been updated
                        if (blk.getByte(offsetByte_valid)) {
                            // Vertex has already been updated, set the sparse active list entry to -1
                            setSparseActiveVertex(pkt, false);
                        } else {
                            // This is the first update to this vertex, set the active list entry and valid bit
                            setSparseActiveVertex(pkt, true);
                            blk.setByte(offsetByte_valid, true);
                        }
                    } else {
                        // Deduplication not enabled, set the sparse active list entry
                        setSparseActiveVertex(pkt, true);
                    }
                } else {
                    // No update made, set the sparse active list entry to -1
                    setSparseActiveVertex(pkt, false);
                }
            } else {
                // Normal dense operation; set the valid bit if an update was made
                if(success) {
                    blk.setByte(offsetByte_valid, true);
                }
            }
        }
        break;

        case 'e': //Graphit: ATOMIC_SUM, float
        {
            DPRINTF(RubySPM, "Graphit: ATOMIC_SUM, float\n");

            offsetByte_value = 0;

            float blkValue;
            memcpy(&blkValue, blk.getData(offsetByte_value, 4),4);
            float pktValue = *(float*)(pkt->getSrcInfo());
            
            // Addition operation always successful
            success = 1; 
            
            // Update the active vertices
            system->active_vertices_per_spm[pkt->getDestSpd()]++;
            system->active_vertices_total++;
            
            float res = pktValue + blkValue;
            
            uint8_t* data = static_cast<uint8_t*>(static_cast<void*>(&res)); 
            for (unsigned i = 0; i < 4; ++i) {
                blk.setByte(i + offsetByte_value, data[i]);
            }
            
            DPRINTF(RubySPM, "Success in atomic execution: blkvalue stored in %f  new pkt value %f res %f\n", blkValue, pktValue,  res);
           
            // Nicholas - Adding deduplication active set update logic
            if(pkt->getIsSparse()) {
                if (success) {
                    if (pkt->getDedupEnabled()) {
                        // Deduplication is enabled, check if this vertex has already been updated
                        if (blk.getByte(offsetByte_valid)) {
                            // Vertex has already been updated, set the sparse active list entry to -1
                            setSparseActiveVertex(pkt, false);
                        } else {
                            // This is the first update to this vertex, set the active list entry and valid bit
                            setSparseActiveVertex(pkt, true);
                            blk.setByte(offsetByte_valid, true);
                        }
                    } else {
                        // Deduplication not enabled, set the sparse active list entry
                        setSparseActiveVertex(pkt, true);
                    }
                } else {
                    // No update made, set the sparse active list entry to -1
                    setSparseActiveVertex(pkt, false);
                }
            } else {
                // Normal dense operation; set the valid bit if an update was made
                if(success) {
                    blk.setByte(offsetByte_valid, true);
                }
            }
        }
        break;

        case 'f': //Graphit: ATOMIC_SUM, double (currently, the same as Pagerank)
        {
            DPRINTF(RubySPM, "Graphit: ATOMIC_SUM, double\n");

            offsetByte_value = 0;

            double blkValue;
            memcpy(&blkValue, blk.getData(offsetByte_value, 8),8);
            double pktValue = *(double*)(pkt->getSrcInfo());
            
            // Addition operation always successful
            success = 1; 
            
            // Update the active vertices
            system->active_vertices_per_spm[pkt->getDestSpd()]++;
            system->active_vertices_total++;
            
            double res = pktValue + blkValue;
            
            uint8_t* data = static_cast<uint8_t*>(static_cast<void*>(&res)); 
            for (unsigned i = 0; i < 8; ++i) {
                blk.setByte(i + offsetByte_value, data[i]);
            }
            
            DPRINTF(RubySPM, "Success in atomic execution: blkvalue stored in %f  new pkt value %f res %f\n", blkValue, pktValue,  res);
           
            // Nicholas - Adding deduplication active set update logic
            if(pkt->getIsSparse()) {
                if (success) {
                    if (pkt->getDedupEnabled()) {
                        // Deduplication is enabled, check if this vertex has already been updated
                        if (blk.getByte(offsetByte_valid)) {
                            // Vertex has already been updated, set the sparse active list entry to -1
                            setSparseActiveVertex(pkt, false);
                        } else {
                            // This is the first update to this vertex, set the active list entry and valid bit
                            setSparseActiveVertex(pkt, true);
                            blk.setByte(offsetByte_valid, true);
                        }
                    } else {
                        // Deduplication not enabled, set the sparse active list entry
                        setSparseActiveVertex(pkt, true);
                    }
                } else {
                    // No update made, set the sparse active list entry to -1
                    setSparseActiveVertex(pkt, false);
                }
            } else {
                // Normal dense operation; set the valid bit if an update was made
                if(success) {
                    blk.setByte(offsetByte_valid, true);
                }
            }
        }
        break;

        case 'g': //Graphit: ATOMIC_MIN, int
        {
            DPRINTF(RubySPM, "Graphit: ATOMIC_MIN, int\n");

            offsetByte_value = 0;

            int blkValue;
            memcpy(&blkValue, blk.getData(offsetByte_value, 4),4);
            int pktValue = *(int*)(pkt->getSrcInfo());
            
            int res;
            if ( pktValue < blkValue ) {
                res = pktValue;
                success = 1;
                
                // Update the active vertices
                system->active_vertices_per_spm[pkt->getDestSpd()]++;
                system->active_vertices_total++;

                DPRINTF(RubySPM, "Success in atomic execution: blkvalue stored in %i new pkt value %i res %i\n", blkValue, pktValue, res);
            }
            else {
                res = blkValue;
                DPRINTF(RubySPM, "No success in atomic execution\n");
            } 
            
            uint8_t* data = static_cast<uint8_t*>(static_cast<void*>(&res)); 
            for (unsigned i = 0; i < 4; ++i) {
                blk.setByte(i + offsetByte_value, data[i]);
            }
           
            // Nicholas - Adding deduplication active set update logic
            if(pkt->getIsSparse()) {
                if (success) {
                    if (pkt->getDedupEnabled()) {
                        // Deduplication is enabled, check if this vertex has already been updated
                        if (blk.getByte(offsetByte_valid)) {
                            // Vertex has already been updated, set the sparse active list entry to -1
                            setSparseActiveVertex(pkt, false);
                        } else {
                            // This is the first update to this vertex, set the active list entry and valid bit
                            setSparseActiveVertex(pkt, true);
                            blk.setByte(offsetByte_valid, true);
                        }
                    } else {
                        // Deduplication not enabled, set the sparse active list entry
                        setSparseActiveVertex(pkt, true);
                    }
                } else {
                    // No update made, set the sparse active list entry to -1
                    setSparseActiveVertex(pkt, false);
                }
            } else {
                // Normal dense operation; set the valid bit if an update was made
                if(success) {
                    blk.setByte(offsetByte_valid, true);
                }
            }
        }
        break;

        case 'h': //Graphit: ATOMIC_MIN, float
        {
            DPRINTF(RubySPM, "Graphit: ATOMIC_MIN, float\n");

            offsetByte_value = 0;

            float blkValue;
            memcpy(&blkValue, blk.getData(offsetByte_value, 4),4);
            float pktValue = *(float*)(pkt->getSrcInfo());
            
            float res;
            if ( pktValue < blkValue ) {
                res = pktValue;
                success = 1;
                
                // Update the active vertices
                system->active_vertices_per_spm[pkt->getDestSpd()]++;
                system->active_vertices_total++;

                DPRINTF(RubySPM, "Success in atomic execution: blkvalue stored in %f new pkt value %f res %f\n", blkValue, pktValue, res);
            }
            else {
                res = blkValue;
                DPRINTF(RubySPM, "No success in atomic execution\n");
            } 
            
            uint8_t* data = static_cast<uint8_t*>(static_cast<void*>(&res)); 
            for (unsigned i = 0; i < 4; ++i) {
                blk.setByte(i + offsetByte_value, data[i]);
            }
           
            // Nicholas - Adding deduplication active set update logic
            if(pkt->getIsSparse()) {
                if (success) {
                    if (pkt->getDedupEnabled()) {
                        // Deduplication is enabled, check if this vertex has already been updated
                        if (blk.getByte(offsetByte_valid)) {
                            // Vertex has already been updated, set the sparse active list entry to -1
                            setSparseActiveVertex(pkt, false);
                        } else {
                            // This is the first update to this vertex, set the active list entry and valid bit
                            setSparseActiveVertex(pkt, true);
                            blk.setByte(offsetByte_valid, true);
                        }
                    } else {
                        // Deduplication not enabled, set the sparse active list entry
                        setSparseActiveVertex(pkt, true);
                    }
                } else {
                    // No update made, set the sparse active list entry to -1
                    setSparseActiveVertex(pkt, false);
                }
            } else {
                // Normal dense operation; set the valid bit if an update was made
                if(success) {
                    blk.setByte(offsetByte_valid, true);
                }
            }
        }
        break;

        case 'i': //Graphit: ATOMIC_MIN, double
        {
            DPRINTF(RubySPM, "Graphit: ATOMIC_MIN, double\n");

            offsetByte_value = 0;

            double blkValue;
            memcpy(&blkValue, blk.getData(offsetByte_value, 8),8);
            double pktValue = *(double*)(pkt->getSrcInfo());
            
            double res;
            if ( pktValue < blkValue ) {
                res = pktValue;
                success = 1;
                
                // Update the active vertices
                system->active_vertices_per_spm[pkt->getDestSpd()]++;
                system->active_vertices_total++;

                DPRINTF(RubySPM, "Success in atomic execution: blkvalue stored in %f new pkt value %f res %f\n", blkValue, pktValue, res);
            }
            else {
                res = blkValue;
                DPRINTF(RubySPM, "No success in atomic execution\n");
            } 
            
            uint8_t* data = static_cast<uint8_t*>(static_cast<void*>(&res)); 
            for (unsigned i = 0; i < 8; ++i) {
                blk.setByte(i + offsetByte_value, data[i]);
            }
           
            // Nicholas - Adding deduplication active set update logic
            if(pkt->getIsSparse()) {
                if (success) {
                    if (pkt->getDedupEnabled()) {
                        // Deduplication is enabled, check if this vertex has already been updated
                        if (blk.getByte(offsetByte_valid)) {
                            // Vertex has already been updated, set the sparse active list entry to -1
                            setSparseActiveVertex(pkt, false);
                        } else {
                            // This is the first update to this vertex, set the active list entry and valid bit
                            setSparseActiveVertex(pkt, true);
                            blk.setByte(offsetByte_valid, true);
                        }
                    } else {
                        // Deduplication not enabled, set the sparse active list entry
                        setSparseActiveVertex(pkt, true);
                    }
                } else {
                    // No update made, set the sparse active list entry to -1
                    setSparseActiveVertex(pkt, false);
                }
            } else {
                // Normal dense operation; set the valid bit if an update was made
                if(success) {
                    blk.setByte(offsetByte_valid, true);
                }
            }
        }
        break;

        case 'j': //Graphit: CAS, 32-bit
        {
            DPRINTF(RubySPM, "Graphit: CAS, 32-bit\n");

            offsetByte_value = 0;

            uint32_t blkValue;
            memcpy(&blkValue, blk.getData(offsetByte_value, 4),4);
            uint32_t pktValue = *(uint32_t*)(pkt->getSrcInfo());
            uint32_t casValue = *(uint32_t*)(pkt->getCasCompareInfo());

            uint32_t res;
            if ( casValue == blkValue ) {
                res = pktValue;
                success = 1;
                
                // Update the active vertices
                system->active_vertices_per_spm[pkt->getDestSpd()]++;
                system->active_vertices_total++;

                DPRINTF(RubySPM, "Success in atomic execution: blkvalue stored in %i new pkt value %i res %i\n", blkValue, pktValue, res);
            }
            else {
                res = blkValue;
                DPRINTF(RubySPM, "No success in atomic execution\n");
            } 
            
            uint8_t* data = static_cast<uint8_t*>(static_cast<void*>(&res)); 
            for (unsigned i = 0; i < 4; ++i) {
                blk.setByte(i + offsetByte_value, data[i]);
            }
           
            // Nicholas - Adding deduplication active set update logic
            if(pkt->getIsSparse()) {
                if (success) {
                    if (pkt->getDedupEnabled()) {
                        // Deduplication is enabled, check if this vertex has already been updated
                        if (blk.getByte(offsetByte_valid)) {
                            // Vertex has already been updated, set the sparse active list entry to -1
                            setSparseActiveVertex(pkt, false);
                        } else {
                            // This is the first update to this vertex, set the active list entry and valid bit
                            setSparseActiveVertex(pkt, true);
                            blk.setByte(offsetByte_valid, true);
                        }
                    } else {
                        // Deduplication not enabled, set the sparse active list entry
                        setSparseActiveVertex(pkt, true);
                    }
                } else {
                    // No update made, set the sparse active list entry to -1
                    setSparseActiveVertex(pkt, false);
                }
            } else {
                // Normal dense operation; set the valid bit if an update was made
                if(success) {
                    blk.setByte(offsetByte_valid, true);
                }
            }
        }
        break;

        default:
            panic("Invalid type of aggregation operation\n");
            break;
    };
}
// Abraham - mrnitro - adding source & destination scratchpads
Cycles
ScratchpadMemory::getAtomicOverhead() {
    int v_response_latency =2; //default value set in MESI protocol
    //int atomicOppType = (int)(system->atomicOppType1) - 48;
   int latency=0; 
    for (auto i = latency_per_app.begin(); i != latency_per_app.end(); ++i) {
                    if (i->first == system->atomicOppType1) {
                        latency =  Cycles(i->second); 
                    }
                }
    
   DPRINTF(RubySPM, "Latency for atomic %i\n", Cycles( v_response_latency + latency));
            
    return Cycles(v_response_latency + latency);

}

// Abraham - mrnitro - adding source & destination scratchpads
//write key-value pairs to scratchpad after receiving from the core 
void
ScratchpadMemory::setDataSPM(PacketPtr pkt, DataBlock& blk, int offset, bool isAtomic) {
    //word size is assumed to be of max. size of 8 bytes
    int offsetByte = offset*8;
    
    DPRINTF(RubySPM, "remote  pkt value: %x pkt_value_1_byte %x addr: %x offset: %i\n", (uint64_t)*(pkt->getConstPtr<uint64_t>()), (uint8_t)*(pkt->getConstPtr<uint8_t>()), pkt->getVaddr() , offset);
    
    if (isAtomic) {
        DPRINTF(RubySPM, " Atomic call: remote  pkt value: %x pkt_value_1_byte %x addr: %x offset: %i\n", (uint64_t)*(pkt->getConstPtr<uint64_t>()), (uint8_t)*(pkt->getConstPtr<uint8_t>()), pkt->getVaddr() , offset);
        atomicExecution(pkt, blk, offset);
    }
    else {
        DPRINTF(RubySPM, " Non atomic call: remote  pkt value: %x pkt_value_1_byte %x addr: %x offset: %i\n", (uint64_t)*(pkt->getConstPtr<uint64_t>()), (uint8_t)*(pkt->getConstPtr<uint8_t>()), pkt->getVaddr() , offset);
        for (unsigned i = 0; i < pkt->getSize(); ++i) {
            blk.setByte(i + offsetByte, (uint8_t)*(pkt->getConstPtr<uint8_t>() + i));
        }
    } 
}

void
ScratchpadMemory::setSparseActiveVertex(PacketPtr pkt, uint8_t success){
        DPRINTF(RubySPM, "Sparse vertex access vaddr: %#x\n",  pkt->getVaddr());
        //offset for valid bit is stored after the 32 bytes
        //alternatively we can have a separate valid/invalid bit
        //default value for next data structure for a dense based iteration is UINT_MAX
        
        unsigned int destId = UINT_MAX;
        if (success) 
           destId = (unsigned int)pkt->getDestInfo(); 
      
      
        DPRINTF(RubySPM, "Writing to sparse vertex access vaddr: %#x destId: %i \n",  pkt->getVaddr(), destId);
            
        DPRINTF(RubySPM, "Before setting Writing to sparse vertex access data %i \n",  *(unsigned int *)(pkt->getConstPtr<uint8_t>()));
        memcpy(pkt->getPtr<uint8_t>(), &destId,pkt->getSize());
        
        DPRINTF(RubySPM, "After setting Writing to sparse vertex access data %i \n",  *(unsigned int *)(pkt->getConstPtr<uint8_t>()));
        
}
/*
void 
ScratchpadMemory::CopyVertexActive() {
    unsigned int offset_from = 4*8;
    unsigned int offset_to = 5*8;
    
    if((system->iterCounter > 0) && ((system->iterCounter)%2)) {
       for (int i = 0; i < m_spm_num_sets; i++) {
            for (int j = 0; j < m_spm_assoc; j++) {
                if(m_spm[i][j] != NULL) {
                    uint8_t value_stored = m_spm[i][j].getByte(offset_from); 
                    m_spm[i][j].setByte(offset_to, &value_stored); 
                }
            }
        }
    }
}
*/
void 
ScratchpadMemory::isVertexActiveSPM(PacketPtr pkt, DataBlock& blk, int offset)
{

    if(pkt->getIsSparse()) {
   
    
        DPRINTF(RubySPM, "Sparse vertex access vaddr: %#x\n",  pkt->getVaddr());
        panic("Trying to write to sparse packet. This functionality is now handled by the atomic execution engine\n");
    /* 
        //offset for valid bit is stored after the 32 bytes
        //alternatively we can have a separate valid/invalid bit
        //default value for next data structure for a dense based iteration is UINT_MAX
        const uint32_t defaultValue = UINT_MAX;
        int offsetByte = (offset + 2)*8;

        uint8_t blkValue = blk.getByte(offsetByte);
        if (blkValue == 0) {
            //reset the value of the packet
            //Abraham - 8 is the assumed maximum word length  
            memcpy(pkt->getPtr<uint8_t>(), &defaultValue,
                   pkt->getSize());
                
            DPRINTF(RubySPM, "Writing UINT_MAX to sparse vertex access vaddr: %#x set: %i \n",  pkt->getVaddr(), pkt->getSetSpd());
    
        }
    */
    }
    else {
       
        //int offsetByte = (offset + 2)*8;
        int offsetByte = offset*8;
        
        DPRINTF(RubySPM, "Dense vertex access vaddr: %#x offset %i with 1 tracked atomic set %i\n",  pkt->getVaddr(),offsetByte, pkt->getSetSpd());
        if (pkt->isWrite()) {
            if(pkt->getIsCopy()) {
                uint8_t blkValue = blk.getByte(offsetByte);
                memcpy(pkt->getPtr<uint8_t>(), &blkValue,
                        pkt->getSize());
                
                DPRINTF(RubySPM, "Writing 0 to dense vertex access vaddr: %#x set: %i \n",  pkt->getVaddr(), pkt->getSetSpd());
            }
            else {
                uint8_t reset=0; 
                blk.setByte(offsetByte , reset);
                blk.setByte(offsetByte, reset);
                DPRINTF(RubySPM, "Dense vertex access write vaddr: %#x\n",  pkt->getVaddr());
            }
        }
        else {
            
            if (pkt->getSize() == 1)  {
                uint8_t blkValue = blk.getByte(offsetByte);
                memcpy(pkt->getPtr<uint8_t>(), &blkValue,1);
                DPRINTF(RubySPM, "Dense vertex access vaddr: %#x value %i with 1 tracked atomic set %i\n",  pkt->getVaddr(), (int)blkValue, pkt->getSetSpd());
            }
            //else if (pkt->getSize() == sizeof(unsigned int))  {
            else   {
                for (int i =0; i < pkt->getSize(); i++) {
                    
                    AbstractCacheEntry* entry = lookup(pkt->getSetSpd() + i);
                    if (entry) {
                        uint8_t blkValue = (entry->getDataBlk()).getByte(offsetByte);
                        memcpy(pkt->getPtr<uint8_t>() + i, &blkValue,1);
                    }
                    else {
                        uint8_t padding =0;
                        memcpy(pkt->getPtr<uint8_t>() + i, &padding,1);
                    }
                }
            }
            //else {
            //    panic("Invalid size for boolean active list access. It should be either 1 or 4(for an optimized access, got size: %i\n", pkt->getSize());
            //}
        }
    }
}
// Given a cache index: returns the index of the tag in a set.
// returns -1 if the tag is not found.
int
ScratchpadMemory::findTagInSet(int64_t cacheSet, Addr tag) const
{
    // search the set for the tags
    auto it = m_tag_index.find(tag);
    if (it != m_tag_index.end())
        if (m_spm[cacheSet][it->second]->m_Permission !=
            AccessPermission_NotPresent)
            return it->second;
    return -1; // Not found
}

// Given a cache index: returns the index of the tag in a set.
// returns -1 if the tag is not found.
int
ScratchpadMemory::findTagInSetIgnorePermissions(int64_t cacheSet,
                                           Addr tag) const
{
    // search the set for the tags
    auto it = m_tag_index.find(tag);
    if (it != m_tag_index.end())
        return it->second;
    return -1; // Not found
}

// Given an unique cache block identifier (idx): return the valid address
// stored by the cache block.  If the block is invalid/notpresent, the
// function returns the 0 address
Addr
ScratchpadMemory::getAddressAtIdx(int idx) const
{
    Addr tmp(0);

    int set = idx / m_spm_assoc;
    DPRINTF(RubySPM, "idx: %i set %i\n", idx, set);
    assert(set < m_spm_num_sets);

    int way = idx - set * m_spm_assoc;
    assert (way < m_spm_assoc);

    AbstractCacheEntry* entry = m_spm[set][way];
    if (entry == NULL ||
        entry->m_Permission == AccessPermission_Invalid ||
        entry->m_Permission == AccessPermission_NotPresent) {
        return tmp;
    }
    return entry->m_Address;
}

bool
ScratchpadMemory::tryCacheAccess(Addr address, RubyRequestType type,
                            DataBlock*& data_ptr)
{
    DPRINTF(RubySPM, "address: %#x\n", address);
    int64_t cacheSet = addressToCacheSet(address);
    int loc = findTagInSet(cacheSet, address);
    if (loc != -1) {
        // Do we even have a tag match?
        AbstractCacheEntry* entry = m_spm[cacheSet][loc];
        m_replacementPolicy_ptr->touch(cacheSet, loc);
        data_ptr = &(entry->getDataBlk());
        return true;
    }
    // The line must not be accessible
    data_ptr = NULL;
    return false;
}

bool
ScratchpadMemory::testCacheAccess(Addr address, RubyRequestType type,
                             DataBlock*& data_ptr)
{
    DPRINTF(RubySPM, "address: %#x\n", address);
    int64_t cacheSet = addressToCacheSet(address);
    int loc = findTagInSet(cacheSet, address);

    if (loc != -1) {
        // Do we even have a tag match?
        AbstractCacheEntry* entry = m_spm[cacheSet][loc];
        m_replacementPolicy_ptr->touch(cacheSet, loc);
        data_ptr = &(entry->getDataBlk());

        return m_spm[cacheSet][loc]->m_Permission !=
            AccessPermission_NotPresent;
    }

    data_ptr = NULL;
    return false;
}

// tests to see if an address is present in the cache
bool
ScratchpadMemory::isTagPresent(Addr address) const
{
    int64_t cacheSet = addressToCacheSet(address);
    int loc = findTagInSet(cacheSet, address);

    if (loc == -1) {
        // We didn't find the tag
        DPRINTF(RubySPM, "No tag match for address: %#x\n", address);
        return false;
    }
    DPRINTF(RubySPM, "address: %#x found\n", address);
    return true;
}

// Returns true if there is:
//   a) a tag match on this address or there is
//   b) an unused line in the same cache "way"
bool
ScratchpadMemory::cacheAvail(Addr address) const
{

    int64_t cacheSet = addressToCacheSet(address);

    DPRINTF(RubySPM, "Tracking addresses:  %#x cacheSet %i assoc %#x\n", address, cacheSet, m_spm_assoc);
    
    for (int i = 0; i < m_spm_assoc; i++) {
        AbstractCacheEntry* entry = m_spm[cacheSet][i];
        if (entry != NULL) {
            if (entry->m_Address == address ||
                entry->m_Permission == AccessPermission_NotPresent) {
                // Already in the cache or we found an empty entry
                return true;
            }
        } else {
            return true;
        }
    }
    DPRINTF(RubySPM, " Not Cache avail case: tracking addresses:  %#x cacheSet %i assoc %#x\n", address, cacheSet, m_spm_assoc);
    return false;
}

AbstractCacheEntry*
ScratchpadMemory::allocate(Addr address, AbstractCacheEntry *entry, bool touch)
{
    assert(!isTagPresent(address));
    assert(cacheAvail(address));
    DPRINTF(RubySPM, "address: %#x\n", address);

    // Find the first open slot
    int64_t cacheSet = addressToCacheSet(address);
    std::vector<AbstractCacheEntry*> &set = m_spm[cacheSet];
    for (int i = 0; i < m_spm_assoc; i++) {
        if (!set[i] || set[i]->m_Permission == AccessPermission_NotPresent) {
            if (set[i] && (set[i] != entry)) {
                warn_once("This protocol contains a cache entry handling bug: "
                    "Entries in the cache should never be NotPresent! If\n"
                    "this entry (%#x) is not tracked elsewhere, it will memory "
                    "leak here. Fix your protocol to eliminate these!",
                    address);
           }
            set[i] = entry;  // Init entry
            set[i]->m_Address = address;
            set[i]->m_Permission = AccessPermission_Invalid;
            if (m_is_vertex_only_cache) {
                DPRINTF(RubySPM, "SSPM - Allocate clearing lock for addr: %x\n",
                    address);
            }
            set[i]->m_locked = -1;
            m_tag_index[address] = i;
            entry->setSetIndex(cacheSet);
            entry->setWayIndex(i);

            if (touch) {
                m_replacementPolicy_ptr->touch(cacheSet, i);
            }

            return entry;
        }
    }
    panic("Allocate didn't find an available entry");
}

void
ScratchpadMemory::deallocate(Addr address)
{
    assert(isTagPresent(address));
    DPRINTF(RubySPM, "address: %#x\n", address);
    int64_t cacheSet = addressToCacheSet(address);
    int loc = findTagInSet(cacheSet, address);
    if (loc != -1) {
        delete m_spm[cacheSet][loc];
        m_spm[cacheSet][loc] = NULL;
        m_tag_index.erase(address);
    }
}

// Returns true if there is a replaceable cache line.
// Returns false if all cachelines are frequent
bool
ScratchpadMemory::isVictimPresent(Addr address) const
{
    assert(!cacheAvail(address));

    int64_t cacheSet = addressToCacheSet(address);
    if(m_replacementPolicy_ptr->getVictim(cacheSet) == -1) {
        return false;
    }
    
    return true;
}

// Returns with the physical address of the conflicting cache line
Addr
ScratchpadMemory::cacheProbe(Addr address) const
{
    assert(isVictimPresent(address));

    int64_t cacheSet = addressToCacheSet(address);
    return m_spm[cacheSet][m_replacementPolicy_ptr->getVictim(cacheSet)]->
        m_Address;
}


// looks an address up in the cache
AbstractCacheEntry*
ScratchpadMemory::lookup(Addr address)
{
    int64_t cacheSet = addressToCacheSet(address);
    int loc = findTagInSet(cacheSet, address);
    if(loc == -1) return NULL;
    return m_spm[cacheSet][loc];
}

// looks an address up in the cache
const AbstractCacheEntry*
ScratchpadMemory::lookup(Addr address) const
{
    int64_t cacheSet = addressToCacheSet(address);
    int loc = findTagInSet(cacheSet, address);
    if(loc == -1) return NULL;
    return m_spm[cacheSet][loc];
}

// Sets the most recently used bit for a cache block
void
ScratchpadMemory::incFrequency(Addr address)
{
    int64_t cacheSet = addressToCacheSet(address);
    int loc = findTagInSet(cacheSet, address);

    if(loc != -1)
        m_replacementPolicy_ptr->touch(cacheSet, loc);
}

void
ScratchpadMemory::incFrequency(const AbstractCacheEntry *e)
{
    uint32_t cacheSet = e->getSetIndex();
    uint32_t loc = e->getWayIndex();
    m_replacementPolicy_ptr->touch(cacheSet, loc);
}

void
ScratchpadMemory::recordCacheContents(int cntrl, CacheRecorder* tr) const
{
    uint64_t warmedUpBlocks = 0;
    uint64_t totalBlocks M5_VAR_USED = (uint64_t)m_spm_num_sets *
                                       (uint64_t)m_spm_assoc;

    for (int i = 0; i < m_spm_num_sets; i++) {
        for (int j = 0; j < m_spm_assoc; j++) {
            if (m_spm[i][j] != NULL) {
                AccessPermission perm = m_spm[i][j]->m_Permission;
                RubyRequestType request_type = RubyRequestType_NULL;
                if (perm == AccessPermission_Read_Only) {
                    if (m_is_instruction_only_cache) {
                        request_type = RubyRequestType_IFETCH;
                    } else {
                        request_type = RubyRequestType_LD;
                    }
                } else if (perm == AccessPermission_Read_Write) {
                    request_type = RubyRequestType_ST;
                }

                if (request_type != RubyRequestType_NULL) {
                    tr->addRecord(cntrl, m_spm[i][j]->m_Address,
                                  0, request_type,
                                  m_replacementPolicy_ptr->getLastAccess(i, j),
                                  m_spm[i][j]->getDataBlk());
                    warmedUpBlocks++;
                }
            }
        }
    }

    DPRINTF(RubySPMTrace, "%s: %lli blocks of %lli total blocks"
            "recorded %.2f%% \n", name().c_str(), warmedUpBlocks,
            totalBlocks, (float(warmedUpBlocks) / float(totalBlocks)) * 100.0);
}

void
ScratchpadMemory::print(ostream& out) const
{
    out << "Cache dump: " << name() << endl;
    for (int i = 0; i < m_spm_num_sets; i++) {
        for (int j = 0; j < m_spm_assoc; j++) {
            if (m_spm[i][j] != NULL) {
                out << "  Index: " << i
                    << " way: " << j
                    << " entry: " << *m_spm[i][j] << endl;
            } else {
                out << "  Index: " << i
                    << " way: " << j
                    << " entry: NULL" << endl;
            }
        }
    }
}

void
ScratchpadMemory::printData(ostream& out) const
{
    out << "printData() not supported" << endl;
}

void
ScratchpadMemory::setLocked(Addr address, int context)
{
    DPRINTF(RubySPM, "Setting Lock for addr: %#x to %d\n", address, context);
    int64_t cacheSet = addressToCacheSet(address);
    int loc = findTagInSet(cacheSet, address);
    assert(loc != -1);
    m_spm[cacheSet][loc]->setLocked(context);
}

void
ScratchpadMemory::clearLocked(Addr address)
{
    DPRINTF(RubySPM, "Clear Lock for addr: %#x\n", address);
    int64_t cacheSet = addressToCacheSet(address);
    int loc = findTagInSet(cacheSet, address);
    assert(loc != -1);
    m_spm[cacheSet][loc]->clearLocked();
}

bool
ScratchpadMemory::isLocked(Addr address, int context)
{
    int64_t cacheSet = addressToCacheSet(address);
    int loc = findTagInSet(cacheSet, address);
    assert(loc != -1);
    DPRINTF(RubySPM, "Testing Lock for addr: %#llx cur %d con %d\n",
            address, m_spm[cacheSet][loc]->m_locked, context);
    return m_spm[cacheSet][loc]->isLocked(context);
}

void
ScratchpadMemory::regStats()
{
    
    m_atomic_execution
        .name(name() + ".atomic_execution")
        .desc("Number of cache atomic execution")
        ;

    m_demand_hits
        .name(name() + ".demand_hits")
        .desc("Number of cache demand hits")
        ;

    m_demand_misses
        .name(name() + ".demand_misses")
        .desc("Number of cache demand misses")
        ;

    m_demand_accesses
        .name(name() + ".demand_accesses")
        .desc("Number of cache demand accesses")
        ;

    m_demand_accesses = m_demand_hits + m_demand_misses;

    m_sw_prefetches
        .name(name() + ".total_sw_prefetches")
        .desc("Number of software prefetches")
        .flags(Stats::nozero)
        ;

    m_hw_prefetches
        .name(name() + ".total_hw_prefetches")
        .desc("Number of hardware prefetches")
        .flags(Stats::nozero)
        ;

    m_prefetches
        .name(name() + ".total_prefetches")
        .desc("Number of prefetches")
        .flags(Stats::nozero)
        ;

    m_prefetches = m_sw_prefetches + m_hw_prefetches;

    m_accessModeType
        .init(RubyRequestType_NUM)
        .name(name() + ".access_mode")
        .flags(Stats::pdf | Stats::total)
        ;
    for (int i = 0; i < RubyAccessMode_NUM; i++) {
        m_accessModeType
            .subname(i, RubyAccessMode_to_string(RubyAccessMode(i)))
            .flags(Stats::nozero)
            ;
    }

    numDataArrayReads
        .name(name() + ".num_data_array_reads")
        .desc("number of data array reads")
        .flags(Stats::nozero)
        ;

    numDataArrayWrites
        .name(name() + ".num_data_array_writes")
        .desc("number of data array writes")
        .flags(Stats::nozero)
        ;

    numTagArrayReads
        .name(name() + ".num_tag_array_reads")
        .desc("number of tag array reads")
        .flags(Stats::nozero)
        ;

    numTagArrayWrites
        .name(name() + ".num_tag_array_writes")
        .desc("number of tag array writes")
        .flags(Stats::nozero)
        ;

    numTagArrayStalls
        .name(name() + ".num_tag_array_stalls")
        .desc("number of stalls caused by tag array")
        .flags(Stats::nozero)
        ;

    numDataArrayStalls
        .name(name() + ".num_data_array_stalls")
        .desc("number of stalls caused by data array")
        .flags(Stats::nozero)
        ;
}

// assumption: SLICC generated files will only call this function
// once **all** resources are granted
void
ScratchpadMemory::recordRequestType(CacheRequestType requestType, Addr addr)
{
    DPRINTF(RubyStats, "Recorded statistic: %s\n",
            CacheRequestType_to_string(requestType));
    switch(requestType) {
    case CacheRequestType_DataArrayRead:
        if (m_resource_stalls)
            dataArray.reserve(addressToCacheSet(addr));
        numDataArrayReads++;
        return;
    case CacheRequestType_DataArrayWrite:
        if (m_resource_stalls)
            dataArray.reserve(addressToCacheSet(addr));
        numDataArrayWrites++;
        return;
    case CacheRequestType_TagArrayRead:
        if (m_resource_stalls)
            tagArray.reserve(addressToCacheSet(addr));
        numTagArrayReads++;
        return;
    case CacheRequestType_TagArrayWrite:
        if (m_resource_stalls)
            tagArray.reserve(addressToCacheSet(addr));
        numTagArrayWrites++;
        return;
    default:
        warn("SPM access_type not found: %s",
             CacheRequestType_to_string(requestType));
    }
}

bool
ScratchpadMemory::checkResourceAvailable(CacheResourceType res, Addr addr)
{
    if (!m_resource_stalls) {
        return true;
    }

    if (res == CacheResourceType_TagArray) {
        if (tagArray.tryAccess(addressToCacheSet(addr))) return true;
        else {
            DPRINTF(RubyResourceStalls,
                    "Tag array stall on addr %#x in set %d\n",
                    addr, addressToCacheSet(addr));
            numTagArrayStalls++;
            return false;
        }
    } else if (res == CacheResourceType_DataArray) {
        if (dataArray.tryAccess(addressToCacheSet(addr))) return true;
        else {
            DPRINTF(RubyResourceStalls,
                    "Data array stall on addr %#x in set %d\n",
                    addr, addressToCacheSet(addr));
            numDataArrayStalls++;
            return false;
        }
    } else {
        assert(false);
        return true;
    }
}

bool
ScratchpadMemory::isBlockInvalid(int64_t cache_set, int64_t loc)
{
  return (m_spm[cache_set][loc]->m_Permission == AccessPermission_Invalid);
}

bool
ScratchpadMemory::isBlockNotBusy(int64_t cache_set, int64_t loc)
{
  return (m_spm[cache_set][loc]->m_Permission != AccessPermission_Busy);
}

//Abraham - adding source & destination scratchpads
//several set functions specific to the scratchpad implementations
void 
ScratchpadMemory::setKey (Addr keyIn)
{
   key = key; 
}

void 
ScratchpadMemory::setOppType (char oppTypeIn)
{
    oppType = oppTypeIn;
}

void 
ScratchpadMemory::setHashType (char hashTypeIn)
{
    DPRINTF(RubySPM, "hash: %c hashTypeIn: %c\n", hashType, hashTypeIn);
    hashType = hashTypeIn;
}

void 
ScratchpadMemory::setKeyType (char keyTypeIn)
{
    keyType = keyTypeIn;
}

//Abraham - adding source & destination scratchpads
//several get functions specific to the scratchpad implementations

Addr
ScratchpadMemory::getKey () const
{
   return key; 
}

char 
ScratchpadMemory::getOppType () const
{
    return oppType;
}

char 
ScratchpadMemory::getHashType () const
{
    return hashType;
}

char 
ScratchpadMemory::getKeyType () const
{
    return keyType;
}

Addr 
ScratchpadMemory::getKeyAddr () const
{
    return m_keyAddr;
}

Addr 
ScratchpadMemory::getValueAddr () const
{
    return m_valueAddr;
}

Addr 
ScratchpadMemory::getOppTypeAddr () const
{
    return m_oppTypeAddr;
}

Addr 
ScratchpadMemory::getHashTypeAddr () const
{
    return m_hashTypeAddr;
}

Addr 
ScratchpadMemory::getKeyTypeAddr () const
{
    return m_keyTypeAddr;
}

Addr 
ScratchpadMemory::getSpillStartAddr() const 
{
    return m_spillStartAddr;
}

Addr 
ScratchpadMemory::getMaxSpillAddr() const 
{
    return m_maxSpillAllocAddr;
}

Addr 
ScratchpadMemory::getNeedSecHashingAddr() const 
{
    return m_needSecHashingAddr;
}

Addr 
ScratchpadMemory::getMapCompleteAddr() const 
{
    return m_mapCompleteAddr;
}

int 
ScratchpadMemory::getNumAccelCompleted() const 
{
    return numAccelCompleted;
}

int 
ScratchpadMemory::getNumAssoc() const 
{
    return m_spm_assoc;
}

int 
ScratchpadMemory::getNumSets() const 
{
    return (m_spm_num_sets - maxChunkSize) ; //Abraham - substracting to account for chunk based scheduling
}

int
ScratchpadMemory::getNumEntries() const 
{
    return ((m_spm_num_sets - maxChunkSize) * m_spm_assoc); //Abraham - substracting to account for chunk based scheduling

}

