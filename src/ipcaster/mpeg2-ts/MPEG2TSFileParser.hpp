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

#include <string>
#include <cstdio>
#include <cerrno>
#include <ios>

#include "ipcaster/base/Exception.hpp"
#include "ipcaster/base/Logger.hpp"
#include "ipcaster/mpeg2-ts/MPEG2TSBuffer.hpp"
#include "ipcaster/mpeg2-ts/MPEG2TSFilters.hpp"

namespace ipcaster
{

/**
 * MPEG-2 TS Files parser
 * Reads from file and push TS packets buffers to a consumer target.
 * Producer/Consumer pattern with threaded buffered IO is used for fine performance.
 */
class MPEG2TSFileParser
{
public:

    // Size of the buffered read, will be rounded to ts-packet size multiple
    static const size_t APROX_READ_SIZE = (128*1024);

    /** Constructor
     * 
     * Opens the file, finds a valid mpeg2-ts sync and computes the bitrate
     * 
     * @param file File path
     * 
     * @notes Only CBR TS files including PCRs are supported, see ITU-T H.222.0
     * standard
     */
    MPEG2TSFileParser(const std::string& file)
    {
        Logger::get().debug() << logfn(MPEG2TSFileParser) << "file: " << file << std::endl;

        file_ = fopen(file.c_str(), "rb");
    
        if(!file_)
            throw Exception(fndbg(MPEG2TSFileParser) + "file: " + file + " - " + strerror(errno));

        // Look for sync
        sync();

        // Computes the file bitrate based on PCRs
        computeBitrate();
    }

    /** Destructor
     * 
     * Closes the file
     */
    ~MPEG2TSFileParser()
    {
        if(file_)
            fclose(file_);
    }

    /** 
     * Looks for 3 consecutive packets with valid sync byte and
     * sets the file pointer to first packet
     * Support TS packets of 188 or 204 bytes
     */
    void sync()
    {
        // Buffer for parsing size = mcm(188,204) = 9588(bytes)
        auto parse_buffer = std::make_shared<Buffer>(9588);

        packet_size_ = 0;
        size_t read_size;
        initial_sync_pos_ = 0;

        do {

            read_size = fread(parse_buffer->data(), 1, parse_buffer->capacity(), file_);

            auto buff = static_cast<uint8_t*>(parse_buffer->data());
            size_t pos = 0;

            while(pos < read_size - (204*3) && packet_size_ == 0) {
                // Sync for 188 bytes packets
                if(buff[pos] == MPEG2TSSYNCBYTE && 
                    buff[pos+188] == MPEG2TSSYNCBYTE && 
                    buff[pos+188*2] == MPEG2TSSYNCBYTE)
                {
                    packet_size_ = 188;
                } // Sync for 204 bytes packets
                else if(buff[pos] == MPEG2TSSYNCBYTE && 
                    buff[pos+204] == MPEG2TSSYNCBYTE && 
                    buff[pos+204*2] == MPEG2TSSYNCBYTE)
                {
                    packet_size_ = 204;
                }
                else  // Continue search
                    pos++;
                    
            }

            // Sync not found in the buffer
            if(packet_size_ == 0 && read_size > (204*3)) {
                // Rewind last 3 packets bytes to include them in the next search
                fseek(file_, -204*3, SEEK_CUR);
                initial_sync_pos_ += read_size-204*3;
            }
            else
                initial_sync_pos_ += pos;

        // While not sync found and not eof
        }while(packet_size_ == 0 && read_size == parse_buffer->capacity()); 

        per_buffer_packets_ = APROX_READ_SIZE / packet_size_;
        packets_read_ = 0;

        fseek(file_, initial_sync_pos_, SEEK_SET); // seek to first packet with sync

        Logger::get().debug() << logfn(MPEG2TSFileParser) << "ts sync found at byte " << initial_sync_pos_ << " with packet_size " << packet_size_ << std::endl;
    }

    // Defines the PCRs distance threshold to compute bitrate
    static const uint64_t BITRATE_COMPUTE_PCR_DISTANCE = static_cast<uint64_t>(PCRCLOCKFREQUENCY * 3);

    /** 
     * Calculates the file's bitrate based on PCR distance vs bytes.
     * The file must be TS CBR
     * The file must have at least two PCRs for the calculation to be 
     * performed
     */
    void computeBitrate()
    {
        PCRFilter pcr_filter; // Filter to analyse PCRs
        uint16_t pid;
        uint64_t pcr_distance = 0;
        size_t bytes_distance;

        std::shared_ptr<MPEG2TSBuffer> buffer = read();
        
        // Push packets to the filter until enough PCR distance is accumulated or EOF
        while(buffer && pcr_distance < BITRATE_COMPUTE_PCR_DISTANCE) {
            pcr_filter.push(buffer, (size_t)ftell(file_));
            pcr_filter.getPIDWithGreaterPCRDistance(pid, pcr_distance, bytes_distance);
            buffer = read();
        }

        if(pcr_distance == 0)
            throw Exception(fndbg(MPEG2TSFileParser) + "Unable to compute file bitrate, not enough PCRs found");

        bitrate_ = (uint64_t)(bytes_distance * 8 / (pcr_distance / PCRCLOCKFREQUENCY));
        
        estimated_buffers_per_second_ = std::max(static_cast<uint32_t>(1), static_cast<uint32_t>(bitrate_ / (per_buffer_packets_ * packet_size_* 8.0)));

        Logger::get().debug() << logfn(MPEG2TSFileParser) << "bitrate = " << bitrate_ << "(bps)" << std::endl;

        fseek(file_, initial_sync_pos_, SEEK_SET); // rewind to first sync byte
    }

    /** 
     * @returns The estimated number of buffers that represents 1 second fragment
     */
    uint32_t estimatedBuffersPerSecond() 
    {
        return estimated_buffers_per_second_;
    }

    
    /** 
     * Reads the next payload buffer from the file
     * @returns A shared pointer to the buffer
     */
    std::shared_ptr<MPEG2TSBuffer> read()
    {
        auto buffer = getBuffer();

        auto bytes = fread(buffer->data(), 1, buffer->capacity(), file_);
        auto num_ts_packets = bytes / packet_size_;

        if(num_ts_packets) {
            buffer->setNumPackets(num_ts_packets); 
            setTimestampsFromBitrate(buffer->timestamps(), packets_read_, bitrate_, num_ts_packets);
            packets_read_ += buffer->numPackets();

            return buffer;
        }

        return nullptr;
    }

private:

    // File handler
    FILE* file_;
    
    // APROX_READ_SIZE rounded to ts-packet size multiple
    size_t per_buffer_packets_;

    // 188 or 204
    size_t packet_size_;

    // Position of the file where sync was found
    size_t initial_sync_pos_;

    // Number of buffers that will be produced for 1 second of stream
    uint32_t estimated_buffers_per_second_;

    // Counter of already read ts-packets
    uint64_t packets_read_;

    // Calculated file bitrate
    uint64_t bitrate_;

    // Allocs a new buffer
    std::shared_ptr<MPEG2TSBuffer> getBuffer()
    {
        return std::make_shared<MPEG2TSBuffer>(per_buffer_packets_, packet_size_);
    }

    // Compute and sets the timestamps of the packets based on the calculated file bitrate
    void setTimestampsFromBitrate(uint64_t* timestamps, uint64_t base_packet_index, uint64_t bitrate, size_t num_packets)
    {
        auto packets = num_packets;
        auto ts = timestamps;

        while(packets--) {
            // packet timestamp in 27Mhz ticks for the base_packet_index position
            (*ts++) = static_cast<uint64_t>(base_packet_index++ * packet_size_* 8 * PCRCLOCKFREQUENCY / (double)bitrate);
        }
    }
};

}


