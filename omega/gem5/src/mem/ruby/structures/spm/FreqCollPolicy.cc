/*
 * Copyright (c) 2013 Advanced Micro Devices, Inc
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
 * Author: Derek Hower
 */

#include "mem/ruby/structures/spm/FreqCollPolicy.hh"



FreqCollPolicy::FreqCollPolicy(const Params * p)
    : AbstractReplacementPolicySPM(p)
{
}


FreqCollPolicy::~FreqCollPolicy()
{
}

FreqCollPolicy *
FreqCollReplacementPolicyParams::create()
{
    return new FreqCollPolicy(this);
}

//abraham - do nothing - only used for casm
void
FreqCollPolicy::touch(int64_t set, int64_t index)
{
    //m_freq_ref_ptr[set][index] +=1;
}

int64_t
FreqCollPolicy::getVictim(int64_t set) const
{
    int freqDiffMax = 0;
    //Abraham - adding source & destination scratchpads
    //@todo - set least_freq_index params to -1 and if it is 
    //this means all kv pairs in the same set are frequent
    int64_t least_freq_index = 0;
    for (int i = 0; i < m_assoc; i++) {
        int freq_diff = m_coll_ref_ptr[set][i] - m_freq_ref_ptr[set][i];
        if (freq_diff > 0) {
            if (freqDiffMax < freq_diff) {
                freqDiffMax = freq_diff;
                least_freq_index = i;
            }
        }
    }
    
    //If we reach here, it means we have a  collision
    for (unsigned i = 0; i < m_assoc; i++) {
        m_coll_ref_ptr[set][i] += 1;
    }
    
    return least_freq_index;
}
