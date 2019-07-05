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

#include <iostream>
#include <thread>

#include <ipcaster/net/UDPReceiver.hpp>
#include <ipcaster/base/FIFO.hpp>
#include <ipcaster/base/Buffer.hpp>
#include <ipcaster/base/Exception.hpp>
#include <ipcaster/base/Logger.hpp>

#define SR_TEST_MAX_FIFO_ELEMENTS (256)
#define SR_TEST_DATAGRAM_PAYLOAD_MAX_SIZE (204*7)
#define SR_TEST_EOF_TIMEOUT_MS (1000)

namespace ipcaster {

class ReceiverWriter
{
public:

    ReceiverWriter(uint16_t port, const std::string& target_file)
        :   receiver_(port),
            datagrams_to_write_to_file_(SR_TEST_MAX_FIFO_ELEMENTS),
            receiving_started_(false),
            exit_threads_(false),
            error_code_(0)
    {
        file_ = fopen(target_file.c_str(), "wb");

        if(!file_)
            throw Exception(fndbg(ReceiverWriter) + "couldn't open file " + target_file + " - " + strerror(errno));

        printf("[SendReceiveTest] %s created\n", target_file.c_str());

        receive_thread_ = std::thread(&ReceiverWriter::receiveThread, this);

        printf("[SendReceiveTest] Waiting for TS UDP cast at port %d\n", port);

        file_writer_thread_ = std::thread(&ReceiverWriter::fileWriterThread, this);
    }

    int waitEndReceiving()
    {
        receive_thread_.join();
        file_writer_thread_.join();

        return error_code_;
    }

private:

    UDPReceiver receiver_;
    FIFO<std::shared_ptr<Buffer>> datagrams_to_write_to_file_;

    std::thread receive_thread_;

    std::thread file_writer_thread_;

    bool receiving_started_;

    bool exit_threads_;

    int error_code_;

    FILE* file_;

    std::shared_ptr<Buffer> getBuffer()
    {
        return std::make_shared<Buffer>(SR_TEST_DATAGRAM_PAYLOAD_MAX_SIZE);
    }

    void receiveThread()
    {
        ip::udp::endpoint endpoint;

        while(!exit_threads_) {
            try {
                // Get a bunch of buffers
                auto buffer = getBuffer();

                // Create a vector of references to the buffers
                UDPReceiver::DatagramBuffer datagram_buffer(buffer->data(), buffer->capacity());
                
                // Receive the datagram
                auto received_bytes = receiver_.receive(datagram_buffer, endpoint, std::chrono::milliseconds(SR_TEST_EOF_TIMEOUT_MS));

                if(received_bytes) {
                    receiving_started_ = true;
                    buffer->setSize(received_bytes);
                    datagrams_to_write_to_file_.push(buffer);
                }
                else {
                    if(receiving_started_) {
                        exit_threads_ = true;
                        datagrams_to_write_to_file_.unblockConsumer();
                    }
                }
            }
            catch(std::exception& e) {
                std::cout << e.what();
                exit_threads_ = true;
                datagrams_to_write_to_file_.unblockConsumer();
                error_code_ = 1;
            }

        }
    }

    void fileWriterThread()
    {
        size_t datagrams_written = 0;

        try {
            while(!exit_threads_) {
                datagrams_to_write_to_file_.waitReadAvailable();
                size_t max_fifo_load = 0;
                while(auto num_dats = datagrams_to_write_to_file_.readAvailable()) {
                    if(num_dats > max_fifo_load)
                        max_fifo_load = num_dats;
                    if(1 != fwrite(datagrams_to_write_to_file_.front()->data(), datagrams_to_write_to_file_.front()->size(), 1, file_))
                        throw Exception("fwrite failed!!!\n");
                    datagrams_to_write_to_file_.pop();
                    datagrams_written++;
                }

                printf("\r[SendReceiveTest] Datagrams written %zu fifo at %.1f%% ", datagrams_written, max_fifo_load / (float)datagrams_to_write_to_file_.capacity() * 100);
            }
        }
        catch(std::exception& e) {
            std::cout << e.what();
            exit_threads_ = true;
            datagrams_to_write_to_file_.unblockConsumer();
            error_code_ = 1;
        }

        fclose(file_);

        printf("\n[SendReceiveTest] Receiving finished\n");
    }
};

class FilesComparer
{
public:

    static const size_t READ_SIZE = 1024*128;

    FilesComparer()
        :   f1_(nullptr),
            f2_(nullptr)
    {

    }


    ~FilesComparer()
    {
        cleanUp();
    }

    void compare(const std::string& file1, const std::string& file2)
    {
        cleanUp();

        f1_ = fopen(file1.c_str(), "rb");

        if(!f1_)
            throw Exception("FilesComparer::compare couldn't open " + file1);

        f2_ = fopen(file2.c_str(), "rb");

        if(!f2_) {
            throw Exception("FilesComparer::compare couldn't open " + file2);
        }

        auto buffer1 = std::unique_ptr<uint8_t[]>( new uint8_t[READ_SIZE] );
        auto buffer2 = std::unique_ptr<uint8_t[]>( new uint8_t[READ_SIZE] );

        size_t p1 = 0;
        size_t p2 = 0;

        size_t r1, r2;

        size_t pos = 0;

        do {
            r1 = fread(buffer1.get(), 1, READ_SIZE, f1_);
            r2 = fread(buffer2.get(), 1, READ_SIZE, f2_);

            auto limit = std::min(r1,r2);

            for(size_t i = 0;i < limit;i++)
                if(buffer1[i] != buffer2[i])
                    throw Exception("FilesComparer::compare failed at pos " + std::to_string(pos + i) + " byte is not equal");

            if(r1 < r2)
                throw Exception("FilesComparer::compare failed at pos " + std::to_string(pos + r1) + ". " + file1 + " is smaller than " + file2);

            if(r2 < r1)
                throw Exception("FilesComparer::compare failed at pos " + std::to_string(pos + r2) + ". " + file2 + " is smaller than " + file1);

            pos += r1;
        }
        while(r1 != 0 && r2 != 0);

        cleanUp();
    }    

private:

    FILE* f1_;

    FILE* f2_;

    void cleanUp() {
        if(f1_) {
            fclose(f1_);
            f1_ = nullptr;
        }

        if(f2_) {
            fclose(f2_);
            f2_ = nullptr;
        }
    }
};

class SendReceiveTest
{
public:
    SendReceiveTest(uint16_t receiver_port, const std::string& source_file, const std::string& target_file) 
    : receiver_writer_(receiver_port, target_file),
        source_file_(source_file),
        target_file_(target_file)
    {

    }

    int run()
    {
        if(receiver_writer_.waitEndReceiving() != 0)
            throw Exception("[SendReceiveTest] Test failed");

        FilesComparer fc;

        printf("[SendReceiveTest] Comparing files...\n");
        fc.compare(source_file_, target_file_);
        printf("[SendReceiveTest] Comparing OK. %s == %s\n", source_file_.c_str(), target_file_.c_str());
        printf("[SendReceiveTest] Test OK.\n");

        return 0;
    }

private:

    std::string source_file_;

    std::string target_file_;

    ReceiverWriter receiver_writer_;

};



} // namespace

