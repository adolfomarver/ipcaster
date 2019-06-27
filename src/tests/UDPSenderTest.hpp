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

#include "ipcaster/net/UDPSender.hpp"

class UDPSenderTest
{
public:
    UDPSenderTest()
    {
        UDPSender::udp::endpoint endpoint(UDPSender::address::from_string(TARGET_IP), TARGET_PORT);

        std::string s = "Hello";
        std::vector<UDPSender::DatagramBuffer> datagrams;
        for(int i=0;i<10;i++)
            datagrams.push_back(boost::asio::buffer(s.c_str(), s.length()+1));
        do {
            sender_.send(endpoint, datagrams);
        }while(1);
    }

private:
    UDPSender sender_;

    //inline static const std::string TARGET_IP = "127.0.0.1";
    inline static const std::string TARGET_IP = "192.168.11.11";
    static const uint16_t TARGET_PORT = 50001;
};