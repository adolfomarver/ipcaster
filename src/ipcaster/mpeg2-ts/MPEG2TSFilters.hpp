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

#include <map>
#include <iostream>
#include <fstream>

#include "ipcaster/base/Buffer.hpp"
#include "ipcaster/mpeg2-ts/MPEG2TS.hpp"

namespace ipcaster
{

/**
 * Accumulates PCRs and its positions to calculate a ts-stream bitrate
 */
class PCRFilter
{
public:

    /**
     * Finds PCRs in a ts buffer and store its values and positions
     * 
     * @param buffer TS stream buffer
     * 
     * @param position Position (in the whole stream) of the fist buffer's byte
     */
    void push(std::shared_ptr<Buffer> buffer, size_t position) {

        assert(std::dynamic_pointer_cast<MPEG2TSBuffer>(buffer));

        auto ts_buffer = std::static_pointer_cast<MPEG2TSBuffer>(buffer);

        TSPacket packet(static_cast<uint8_t*>(ts_buffer->data()), ts_buffer->packetSize());

        auto packet_index = 0;
        auto max_packets = ts_buffer->numPackets();

        while(packet_index < max_packets) {

            if(packet.hasPCR()) {

                std::shared_ptr<std::vector<PCRPosition>> pcrs;

                try {
                    // Each PID has its own PCRPosition array
                    pcrs = pids_pcrs_.at(packet.pid());
                }
                catch(std::exception&) {
                    pcrs = std::make_shared<std::vector<PCRPosition>>();
                    pids_pcrs_[packet.pid()] = pcrs;
                }

                pcrs->push_back({packet.pcr(), packet_index * ts_buffer->packetSize() + position});
            }

            packet.moveNext(); // Next TS packet
            packet_index++;
        }
    }

    /**
     * Obtain the PID with greater PCRs distance
     * 
     * @param [out] pid PID
     * 
     * @param [out] pcr_ticks_distance Distance in 27Mhz ticks 
     * 
     * @param [out] pcr_bytes_distance Distance in bytes
     */
    void getPIDWithGreaterPCRDistance(uint16_t& pid, uint64_t& pcr_ticks_distance, size_t& pcr_bytes_distance)
    {
        pcr_ticks_distance = 0;

        for(auto& pid_pcrs : pids_pcrs_) {
            auto& pcrs = *pid_pcrs.second;
            if(pcrs.size() > 2) {
                auto pcr_distance = pcrSub(pcrs[0].pcr, pcrs.back().pcr);
                if(pcr_distance > pcr_ticks_distance) {
                    pid = pid_pcrs.first;
                    pcr_ticks_distance = pcr_distance;
                    pcr_bytes_distance = pcrs.back().position - pcrs[0].position;
                }
            }
        }
    }

private:

    // Holds PCRs tick and position (byte in the whole stream)
    struct PCRPosition
    {
        uint64_t pcr;
        size_t position;
    };

    // Maps of PIDs, each PID has a vector with PCRPositions
    std::map<uint16_t, std::shared_ptr<std::vector<PCRPosition>>> pids_pcrs_;

};

}
