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

#include <chrono>
#include <vector>

#include "ipcaster/base/Buffer.hpp"
#include "ipcaster/net/IP.hpp"

namespace ipcaster
{

/**
 * Represents a datagram to be send in a (timed-based) scheduling manner
 * This class holds a strong reference to the payload data buffer as long as
 * destination ip, port, send_tick
 */
class Datagram
{
public:

    /** Constructor
     * @param target_ip Destination IPv4 address
     * @param target_port Destination port
     * @param payload shared pointer to payload buffer
     * @param send_tick Send timepoint 
     */
    Datagram (const std::string& target_ip, 
                uint16_t target_port, 
                std::shared_ptr<Buffer> payload,
                std::chrono::high_resolution_clock::time_point send_tick)
            :   target_ip_(target_ip), 
                target_port_(target_port),
                payload_(payload),
                send_tick_(send_tick)
    {
    }

    /** @returns The target ip */
    inline const char* targetIP() {return target_ip_.c_str();}

    /** Sets the target IP */    
    inline void setTargetIP(const std::string& target_ip) { target_ip_ = target_ip; }

    /** @returns The target port */    
    inline uint16_t targetPort() {return target_port_;}

    /** Sets the target port */    
    inline void setTargetPort(uint16_t target_port) { target_port_ = target_port; }

    /** @returns The payload buffer reference */    
    inline std::shared_ptr<Buffer> payload() {return payload_;}

    /** @returns An endpoint object pointing to the target address:port */    
    inline ip::udp::endpoint endpoint() {return ip::udp::endpoint(ip::address::from_string(target_ip_.c_str()),target_port_); }

    /** @returns The send timepoint */    
    std::chrono::high_resolution_clock::time_point sendTick() { return send_tick_; }

	/** Sets the send_tick */
	void setSendTick(std::chrono::high_resolution_clock::time_point send_tick) { send_tick_ = send_tick; }

private:

    std::shared_ptr<Buffer> payload_;
    std::chrono::high_resolution_clock::time_point send_tick_;
    std::string target_ip_;
    uint16_t target_port_;
};

}