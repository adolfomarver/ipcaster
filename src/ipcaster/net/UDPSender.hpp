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

#include <memory>

#include <boost/asio.hpp>
#include <boost/bind.hpp>

/**
 * class UDPSender 
 * 
 * Provides functionality to send datagrams to UDP endpoints.
 *
 */
class UDPSender
{
public:

    using DatagramBuffer = boost::asio::const_buffer;
    using SystemError = boost::system::system_error;
    
    /**
     * Intializes an UDP socket IPv4 for sending.
     *
     * @throws UDPSender::SystemError Thrown on failure.
     */
    UDPSender()
    {
        io_service_ = std::make_unique<boost::asio::io_service>();
        socket_ = std::make_unique<boost::asio::ip::udp::socket>(*io_service_);
        socket_->open(boost::asio::ip::udp::v4());
    }

    /**
     * Sends a datagram or a collection of datagrams an endpoint
     *
     * @param endpoint Target ip and port
     * 
     * @param buffers Collection of buffers to send
     * 
     * @returns The number of bytes sent
     * 
     * @throws UDPSender::SystemError Thrown on failure.
     */
    template <typename ConstBufferSequence>
    std::size_t send(const ip::udp::endpoint& endpoint, const ConstBufferSequence& buffers)
    {
        return socket_->send_to(buffers, endpoint, 0);
    }

private:

    std::unique_ptr<boost::asio::io_service> io_service_;
    
    std::unique_ptr<boost::asio::ip::udp::socket> socket_;
};

