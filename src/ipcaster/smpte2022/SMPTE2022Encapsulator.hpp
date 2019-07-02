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

/**
 * class SMPTE2022Encapsulator 
 * 
 * Encapsulates data according to SMPTE2022-2 standard.
 * In this version RTP header is not supported
 * 
 * @todo Implement RTP header
 * @todo Implement ts-null packets removal
 * @todo Support num-packets per datagram configuration
 * @todo Socket level parameters (TTL...) to fully comply SMPTE2022
 */

#include <stdint.h>
#include <cstddef>
#include <string.h>

#include "ipcaster/mpeg2-ts/MPEG2TSBuffer.hpp"
#include "ipcaster/mpeg2-ts/MPEG2TSFilters.hpp"
#include "ipcaster/net/Datagram.hpp"

namespace ipcaster
{

/**
 * Receives mpeg2-ts packets and produces datagram buffers
 * encapsulated according to SMPTE2022-2 standard
 * 
 * @param DatagramConsumer class of the object receiving the
 * datagram buffers
 */
template<class DatagramConsumer>
class SMPTE2022Part2Encapsulator
{
    using Clock = std::chrono::high_resolution_clock;

public:

    /** Constructor
     * @param consumer Reference to object where datagram buffers will be pushed
     */
    SMPTE2022Part2Encapsulator(DatagramConsumer& consumer)
        : consumer_(consumer)
    {
        ts_packets_per_datagram_ = 7;
    }

    /** 
     * Encapsulates the input buffer into a datagram.
     * When a datagram is completed then it is pushed to the consumer_ object
     * 
     * @param buffer MPEG2TSBuffer reference with the ts packets to encapsulate
     */
    void push(std::shared_ptr<Buffer> buffer)
    {
        assert(std::dynamic_pointer_cast<MPEG2TSBuffer>(buffer));

        std::shared_ptr<MPEG2TSBuffer> ts_buffer = std::static_pointer_cast<MPEG2TSBuffer>(buffer);

        auto payload_size = ts_packets_per_datagram_*ts_buffer->packetSize();
        auto num_packets = ts_buffer->numPackets();
        auto datagrams = std::make_shared<std::vector<Datagram>>();
        Clock::time_point send_tick;

        size_t pkt_index = 0;

        if(unfinished_datagram_)
            pkt_index = handleUnfinishedDatagram(ts_buffer->packet(0), num_packets);

        // While there's enough packets to make a full datagram
        for(; pkt_index + ts_packets_per_datagram_ < num_packets; pkt_index += ts_packets_per_datagram_) {

            // The payload of the datagram is a number of ts packets defined by ts_packets_per_datagram_
            // so a child reference is created to point those packets, and then pushed to the consumer
            auto payload = ts_buffer->makeChild(pkt_index, ts_packets_per_datagram_, ts_packets_per_datagram_);

            consumer_.push(std::make_shared<Datagram>("", 0, payload, sendTick(ts_buffer->timestamp(pkt_index))));
        }

        auto remaining_packets = num_packets - pkt_index;

        if(remaining_packets)
            storeUnfinishedDatagram(ts_buffer, pkt_index, remaining_packets);
    }

    /** 
     * If there's an unfinished datagram this function
     * forces to push the datagram to the consumer event is not
     * completed, and after that called flush on the consumer
     */
    void flush()
    {
        if(unfinished_datagram_) {
            consumer_.push(unfinished_datagram_);
            unfinished_datagram_.reset();
        }

        consumer_.flush();
    }

	/**
	* When no more push will be done close should be called
	* to free the consumer resources
	*/
	void close()
	{
		consumer_.close();
	}

    /**
	* Called by the producer with information that helps to
    * setup the buffers
    * 
	* @param estimated_buffers_per_second Estimated number of buffers per second
    * that will be pushed by the producer
    * 
    * @param estimated_bitrate Estimated bitrate that will be produced by the 
    * producer
	*/
    void setBuffering(size_t estimated_buffers_per_second, uint64_t estimated_bitrate)
    {
        // Estimation of how many buffers per second this component will produce
        auto next_estimated_buffers_per_second = estimated_bitrate / (ts_packets_per_datagram_ * 8 * 188);
        consumer_.setBuffering(next_estimated_buffers_per_second, estimated_bitrate);
    }

private:

    // Number of ts packets to insert in every datagram
    size_t ts_packets_per_datagram_;

    // Datagram partially full 
    std::shared_ptr<Datagram> unfinished_datagram_;

    // Reference to the consumer object where datagrams will be pushed
    DatagramConsumer& consumer_;

    /**
     * Converts from PCRTime to  high_resolution_clock time
     * @param ts_packet_timestamp PCRTime
     * @returns ts_packet_timestamp value converted to high_resolution_clock units 
     */
    inline Clock::time_point sendTick(uint64_t ts_packet_timestamp)
    {
        return Clock::time_point(
            std::chrono::duration_cast<std::chrono::nanoseconds>(PCRTicks(ts_packet_timestamp))
            );
    }

    /** 
     * If an unfinished datagram is pendant this function adds packets to it. If 
     * the datagram is fully complete with the input packets this function will also 
     * send the completed datagram.
     * 
     * @returns The number of input packets consumed.
     */
    size_t handleUnfinishedDatagram(uint8_t* p, size_t num_packets)
    {
        auto payload = std::static_pointer_cast<MPEG2TSBuffer>(unfinished_datagram_->payload());
        auto remaining_packets = ts_packets_per_datagram_ - payload->numPackets();
        auto packets_to_copy = std::min(remaining_packets, num_packets);
        auto packet_size = payload->packetSize();
        auto copy_size = packet_size * packets_to_copy;

        memcpy(static_cast<uint8_t*>(payload->data()) + packet_size * payload->numPackets(), p, copy_size);

        payload->setNumPackets(payload->numPackets() + packets_to_copy);

        if(payload->numPackets() == ts_packets_per_datagram_) {
            consumer_.push(unfinished_datagram_);
            unfinished_datagram_.reset();
        }

        return packets_to_copy;
    }

    /** 
     * Called by push() when the number of ts-packets remaining to process is lower
     * than ts_packets_per_datagram_. In this case those packets are stored to
     * complete the datagram in the next call to push.
     */
    void storeUnfinishedDatagram(std::shared_ptr<MPEG2TSBuffer> ts_buffer, size_t pkt_index, size_t num_packets)
    {
        auto payload = std::make_shared<MPEG2TSBuffer>(ts_packets_per_datagram_, ts_buffer->packetSize());
        unfinished_datagram_ = std::make_shared<Datagram>("", 0, payload, sendTick(ts_buffer->timestamp(pkt_index)));
        memcpy(payload->data(), ts_buffer->packet(pkt_index), num_packets*ts_buffer->packetSize());
        payload->setNumPackets(num_packets);
    }    
}; // SMPTE2022Part2Encapsulator

}