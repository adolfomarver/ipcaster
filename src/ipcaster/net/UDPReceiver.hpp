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
#include <boost/asio/deadline_timer.hpp>

#include <ipcaster/net/IP.hpp>

/**
 * class UDPReceiver 
 * 
 * Provides functionality to receive datagrams through UDP.
 *
 */
class UDPReceiver
{
public:

    using DatagramBuffer = boost::asio::mutable_buffers_1;
    using SystemError = boost::system::system_error;

    /**
     * Intializes an UDP socket IPv4 for receiving.
     *
     * @param port IP port where to listen
     * 
     * @throws UDPReceiver::SystemError Thrown on failure.
     */
    UDPReceiver(uint16_t port)
    : socket_(io_service_, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), port)),
      deadline_(io_service_)
    {
        // No deadline is required until the first socket operation is started. We
        // set the deadline to positive infinity so that the actor takes no action
        // until a specific deadline is set.
        deadline_.expires_at(boost::posix_time::pos_infin);

        // Start the persistent actor that checks for deadline expiry.
        checkDeadline();
    }

    /**
     * Receive a datagram, the function blocks until a datagram is received
     *
     * @param buffer buffers to store the received datagrams
     * 
     * @param [out] Remote source endpoint (ip and port)
     *
     * @returns The number of bytes received
     * 
     * @throws UDPReceiver::SystemError Thrown on failure.
     */
    std::size_t receive(DatagramBuffer& buffer, ip::udp::endpoint& endpoint)
    {
        return socket_.receive_from(buffer, endpoint);
    }

    /**
     * Receive a datagram or
     *
     * @param buffer buffers to store the received datagrams
     * 
     * @param [out] Remote source endpoint (ip and port)
    
     * @param time_out Time out in milliseconds, if timeout expires the function returns
     * with 0 bytes received
     * 
     * @returns The number of bytes received
     * 
     * @throws UDPReceiver::SystemError Thrown on failure.
     */
    std::size_t receive(DatagramBuffer& buffer, ip::udp::endpoint& endpoint, std::chrono::milliseconds time_out)
    {
        // Set a deadline for the asynchronous operation.
        deadline_.expires_from_now(boost::posix_time::time_duration(boost::posix_time::millisec(time_out.count())));

        // Set up the variables that receive the result of the asynchronous
        // operation. The error code is set to would_block to signal that the
        // operation is incomplete. Asio guarantees that its asynchronous
        // operations will never fail with would_block, so any other value in
        // ec indicates completion.
        boost::system::error_code ec = boost::asio::error::would_block;
        std::size_t length = 0;

        // Start the asynchronous operation itself. The handle_receive function
        // used as a callback will update the ec and length variables.
        socket_.async_receive(buffer,
            boost::bind(&UDPReceiver::handleReceive, _1, _2, &ec, &length));

        // Block until the asynchronous operation has completed.
        do io_service_.run_one(); while (ec == boost::asio::error::would_block);

        if(ec != boost::system::errc::success && ec != boost::asio::error::operation_aborted)
            throw UDPReceiver::SystemError(ec);

        return length;
    }

private:

    boost::asio::io_service io_service_;

    boost::asio::ip::udp::socket socket_;
    
    boost::asio::deadline_timer deadline_;

    static void handleReceive(
      const boost::system::error_code& ec, std::size_t length,
      boost::system::error_code* out_ec, std::size_t* out_length)
    {
        *out_ec = ec;
        *out_length = length;
    }

    void checkDeadline()
    {
        // Check whether the deadline has passed. We compare the deadline against
        // the current time since a new asynchronous operation may have moved the
        // deadline before this actor had a chance to run.
        if (deadline_.expires_at() <= boost::asio::deadline_timer::traits_type::now())
        {
        // The deadline has passed. The outstanding asynchronous operation needs
        // to be cancelled so that the blocked receive() function will return.
        //
        // Please note that cancel() has portability issues on some versions of
        // Microsoft Windows, and it may be necessary to use close() instead.
        // Consult the documentation for cancel() for further information.
        socket_.cancel();

        // There is no longer an active deadline. The expiry is set to positive
        // infinity so that the actor takes no action until a new deadline is set.
        deadline_.expires_at(boost::posix_time::pos_infin);
        }

        // Put the actor back to sleep.
        deadline_.async_wait(boost::bind(&UDPReceiver::checkDeadline, this));
    }
};

