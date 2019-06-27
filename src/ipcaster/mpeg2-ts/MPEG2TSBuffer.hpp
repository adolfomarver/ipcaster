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

#include "ipcaster/base/Buffer.hpp"

namespace ipcaster
{

/**
 * Handles TS memory buffers, extends ipcaster::Buffer
 * 
 * @see ipcaster::Buffer
 */
class MPEG2TSBuffer : public Buffer
{
public:

    /** Constructor
     * 
     * Alloc the buffer
     * 
     * @param num_packets_capacity number of packets space to allocate
     * 
     * @param packet_size TS packet's size
     */
    MPEG2TSBuffer(size_t num_packets_capacity, uint8_t packet_size) 
        :   Buffer(num_packets_capacity*packet_size), 
            packet_size_(packet_size),
            num_packets_(0)
    {
	    allocated_timestamps_ = std::unique_ptr<uint64_t[]>(new uint64_t[num_packets_capacity]);
        timestamps_ = allocated_timestamps_.get();   
    }

    /** Constructor
     * 
     * Creates a sub-buffer pointing to a fragment of a parent buffer already allocated
     * A strong reference to the parent buffer is holded.
     * 
     * @param data Pointer to the sub-buffer data (must belong to the parent buffer space)
     * 
     * @param timestamps Pointer to the sub-buffer parent's timestamps 
     * 
     * @param num_packets_capacity "Allocated" number of packets of the sub-buffer
     * 
     * @param num_packets_size Size of the payload (number of valid packets in the buffer)
     * 
     * @param packet_size TS packet's size
     * 
     * @param parent Shared pointer to the parent
     */
    MPEG2TSBuffer(void* data, uint64_t* timestamps, size_t num_packets_capacity, size_t num_packets_size, uint8_t packet_size, std::shared_ptr<MPEG2TSBuffer> parent)
        :   Buffer(data, num_packets_capacity*packet_size, num_packets_size*packet_size, parent),
            packet_size_(packet_size),
            num_packets_(num_packets_size)

    {
        // TODO: assert data belongs to parent

        timestamps_ = timestamps;
    }

    /**
     * Creates a sub-buffer pointing to a fragment of this buffer
     * A strong reference to the parent buffer is holded.
     * 
     * @param packet_index Start packet of the sub-buffer
     * 
     * @param num_packets_capacity "Allocated" number of packets of the sub-buffer
     * 
     * @param num_packets_size Size of the payload (number of valid packets in the buffer)
     * 
     * @param packet_size TS packet's size
     * 
     * @returns A Shared pointer to the new sub-buffer
     */
    std::shared_ptr<MPEG2TSBuffer> makeChild(size_t packet_index, size_t num_packets_capacity, size_t num_packets_size)
    {
        auto a = std::make_shared<MPEG2TSBuffer>(packet(packet_index), &timestamps_[packet_index], num_packets_capacity, num_packets_size, packet_size_, std::static_pointer_cast<MPEG2TSBuffer>(this->shared_from_this()));
        return a;
    }

    /**
     * Sets the size of valid data in the buffer
     * @param num_packets Number of valid ts packets in the buffer
     */
    void setNumPackets(size_t num_packets) 
    {   
        num_packets_ = num_packets;
        setSize(num_packets*packet_size_);
    }

    /** @returns The number of valid ts packets in the buffer */
    inline size_t numPackets() const { return num_packets_;}

    /** @returns The capacity (max number of ts packets that can be stored) of the buffer*/
    inline size_t numPacketsCapacity() const { return capacity()/packet_size_;}

    /** @returns The ts packet's size (188 or 204)*/
    inline uint8_t packetSize() const {return packet_size_;}

    /** @returns Pointer to the current ts packet*/
    inline uint8_t* packet(size_t index) const { return &static_cast<uint8_t*>(data())[index*packet_size_]; }

    /** 
     * Gets the timestamp of the packet  in the "index-n" position of the buffer
     * 
     * @param index Index of the packet in the buffer
     * 
     * @returns The timestamp (in PCR units) of the packet at a specific index
     */
    inline uint64_t timestamp(size_t index) const { return timestamps_[index]; }

    /** @returns A pointer to the packet's timestamps array */
    inline uint64_t* timestamps() const { return timestamps_; }

private:

    // Number of valid packets in the buffer
    size_t num_packets_;

    // TS packet size (188 / 204)
    uint8_t packet_size_;

    // Allocated space for timestamps (only allocated if this is the parent buffer)
    std::unique_ptr<uint64_t[]> allocated_timestamps_;
    
    // PCR Timestamps array, every element is paired with a ts packet in the buffer
    uint64_t* timestamps_;
};

}
