//
// Copyright (C) 2019 Adofo Martinez <adolfo at ipcaster dot net>
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#pragma once 

#include <stdint.h>
#include <stdio.h>

#include "ipcaster/base/Platform.hpp"

namespace ipcaster
{

const uint8_t TSNULL188[] = {
0x47, 0x1F, 0xFF, 0x10, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

/** Maximum value of the PCR's 42bits counter */
constexpr uint64_t PCRMAXVALUE = (((uint64_t)1 << 33) -1) * 300 + 299;

/** PCR's counter value frequency*/
constexpr double PCRCLOCKFREQUENCY = 27000000;

// PCR period
typedef std::ratio<1,(uint32_t)PCRCLOCKFREQUENCY> PCRPeriod;

// PCR time units
typedef std::chrono::duration<int64_t, PCRPeriod> PCRTicks;

/** Sync byte value for mpeg2-ts packets*/
constexpr uint8_t MPEG2TSSYNCBYTE = 0x47;

/** 
 * Performs substract between two PCR values 
 * 
 * @param pcrN First ocurred PCR
 * 
 * @param pcrN1 Second ocurred PCR
 * 
 * @note The values are considered to be obtain from a monotonic clock source
 */
inline uint64_t pcrSub(uint64_t pcrN, uint64_t pcrN1) { return (pcrN1 >= pcrN) ? pcrN1-pcrN : pcrN1 + PCRMAXVALUE - pcrN +1; }

/**
 * Wrapper class to handle mpeg-2 transport streams buffers
 * at packet level
 */
class TSPacket
{
public:

    /** Default constructor
     * Initializes a void packet, not pointing to any packet
     */
    TSPacket()
        : pkt_(nullptr), 
        size_(0)
    {
    }

    /** Constructor
     * Initializes the wrapper
     * 
     * @param pkt Pointer to the raw TS packet.
     * 
     * @param size TS packet size
     */
    TSPacket(uint8_t* pkt, uint8_t size)
        : pkt_(pkt), size_(size)
    {
        assert(size == 188 || size == 204);
    }

    /** 
     * Sets to TS packet pointer
     * @param pkt Pointer to the raw TS packet
     */
    inline void setBase(uint8_t* pkt) {pkt_ = pkt;}

    /** 
     * Sets to TS packet size
     * @param size TS packet size
     */
    inline void setSize(uint8_t size) {assert(size == 188 || size == 204); size_ = size;}

    /** Moves the pointer to the next packet */
    inline void moveNext() {pkt_+=size_;}

    /** @returns The continuity counter */
    inline uint8_t cc() const { return pkt_[3] & 0x0F; }

    /** 
     * Sets the continuity counter of the packet 
     * @param cc New cc in the 4 LSB
     */
    inline void setCC(uint8_t cc) { pkt_[3] = (pkt_[3] & 0xF0) | (cc & 0x0F); }

    /** @returns the packet's PID */
    inline uint16_t pid() const { return word16(pkt_+1) & 0x1FFF; }

    /** 
     * Sets the packet's PID
     * @param pid New PID
     */
    inline void setPID(uint16_t pid) { pkt_[1] = (pkt_[1] & 0xE0) | ((pid >> 8) & 0x1F); pkt_[2] = pid & 0x00FF;}

    /** @returns the packet's adaptation field control bits in the 4 LSB*/
    inline uint8_t afc() const { return (pkt_[3] & 0x30) >> 4; }

    /** 
     * Sets the packet's adaptation field control bits
     * @param afc New afc in the 4 LSB
     */
    inline uint8_t setAFC(uint8_t afc) const { return pkt_[3] &= (afc << 4);}

    /** @returns true if packet has payload */
    inline bool hasPayload() const { return afc() == 1 || afc() == 3;}

    /** @returns true if packet has adaptation field */
    inline bool hasAF() const { return (pkt_[3] & 0x20) != 0; }

    /** @returns The adaptation field size */
    inline size_t afSize() const { return hasAF() ? size_t(pkt_[4]) : 0; }

    /** @returns true if packet contains a PCR */
    inline bool hasPCR() const { return afSize() > 0 && (pkt_[5] & 0x10) != 0; }

    /** @returns The PCR's value */
    uint64_t pcr() const
    {
        // Computes the 42bit PCR value (33  90Khz based + 9 27Mhz based)
        const uint32_t v32 = word32(&pkt_[6]);
        const uint16_t v16 = word16(&pkt_[10]);
        const uint64_t pcr_base = (uint64_t(v32) << 1) | uint64_t(v16 >> 15);
        const uint64_t pcr_ext = uint64_t(v16 & 0x01FF);
        return pcr_base * 300 + pcr_ext;
    }

private:

    // @returns a 32bits word in the right endian order for current architecture
    inline uint32_t word32(const void* p) const
    {

    #if defined(WORDS_BIGENDIAN)
        return *static_cast<const uint32_t*>(p);
    #else
        return bswap_32(*(static_cast<const uint32_t*>(p)));
    #endif 
    }

    // @returns a 16bits word in the right endian order for current architecture
    inline uint16_t word16(const void* p) const
    {
    #if defined(WORDS_BIGENDIAN)
        return *static_cast<const uint16_t*>(p);
    #else
        return bswap_16(*(static_cast<const uint16_t*>(p)));
    #endif 
    }

    // TS packet base pointer
    uint8_t* pkt_;

    // TS packet size
    uint8_t size_;
};

static void getTestPacket188(size_t index, uint8_t* buffer)
{
    memcpy(buffer, TSNULL188, sizeof(TSNULL188));
    TSPacket packet(buffer, 188);
    packet.setAFC(1);
    packet.setPID(0);
    packet.setCC((uint8_t)index);
}

void genTestFile188(const std::string& file, size_t num_packets)
{
    uint8_t buffer[188];

    FILE* f = fopen(file.c_str(), "wb");
    for(size_t i = 0;i<num_packets;i++) {
        getTestPacket188(i, buffer);
        fwrite(buffer, sizeof(buffer), 1, f);
    }

    fclose(f);
}
    
}