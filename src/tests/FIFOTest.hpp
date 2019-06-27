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

#include <gtest/gtest.h>

#include "ipcaster/base/FIFO.hpp"

#define LOG std::cout

namespace ipcaster {

class FIFOTest : public ::testing::Test
{
public:
    FIFOTest() {

     // You can do set-up work for each test here.
    }

    ~FIFOTest() override {
        // You can do clean-up work that doesn't throw exceptions here.
    }

    // If the constructor and destructor are not enough for setting up
    // and cleaning up each test, you can define the following methods:

    void SetUp() override {
        // Code here will be called immediately after the constructor (right
        // before each test).
    }

    void TearDown() override {
        // Code here will be called immediately after each test (right
        // before the destructor).
    }
};

static const size_t FIFO_CAPACITY = 100;
static const size_t TEST_PERFORMANCE_ELEMENTS = 1000000;

TEST_F(FIFOTest, pushTillFull) 
{
    FIFO<int> fifo(FIFO_CAPACITY);

    // Full the FIFO
    for(size_t i=0;i<FIFO_CAPACITY;i++)
        fifo.push((int)i);

    // This thread waits 1(s) and pop an element
    std::thread ([&]{
        std::this_thread::sleep_for(std::chrono::seconds(1));
        fifo.pop();
    }).detach();

    // Wait for the thread to pop an element to complete de push
    fifo.push(0);
}

TEST_F(FIFOTest, popTillEmpty)
{
    FIFO<int> fifo(FIFO_CAPACITY);

    // Full the FIFO
    for(size_t i=0;i<FIFO_CAPACITY;i++)
        fifo.push((int)i);

    // empty the FIFO
    for(size_t i=0;i<FIFO_CAPACITY;i++)
        fifo.pop();

    // This thread waits 1(s) and push an element
    std::thread ([&]{
        std::this_thread::sleep_for(std::chrono::seconds(1));
        fifo.push(0);
    }).detach();

    // Wait for the thread to push an element to be read
    fifo.waitReadAvailable();
}

TEST_F(FIFOTest, testPerformance)
{
    FIFO<int> fifo(FIFO_CAPACITY);

    bool ret = true;

    uint32_t full_fifo_count = 0; 
    uint32_t empty_fifo_count = 0; 
    auto t_begin = std::chrono::high_resolution_clock::now();

    std::thread thread_producer([&]{  
        LOG << "[threadProducer] pushing " << TEST_PERFORMANCE_ELEMENTS << " elements..." << std::endl;
        for(size_t i=0;i<TEST_PERFORMANCE_ELEMENTS;i++) {
            if(!fifo.tryPush((int)i)) {
                full_fifo_count++;
                fifo.push((int)i);
            }                
        }
        LOG << "[threadProducer] done." << std::endl;
    });

    std::thread thread_consumer([&]{  
        LOG << "[threadConsumer] poping " << TEST_PERFORMANCE_ELEMENTS << " elements..." << std::endl;
        std::string error;
        for(size_t i=0;i<TEST_PERFORMANCE_ELEMENTS;i++) {
            if(!fifo.readAvailable()) {
                empty_fifo_count++;
                fifo.waitReadAvailable();
            }
            if(fifo.front() != (int)i) {
                error = "[threadConsumer] element pop error at index " + std::to_string(i) + "\n";
                break;
            }
            fifo.pop();
        }

        if(error.empty())
            LOG << "[threadProducer] done." << std::endl;
        else {
            ret = false;
            std::cerr << error;
        }
    });

    thread_producer.join();
    thread_consumer.join();

    auto t_delta = std::chrono::high_resolution_clock::now() - t_begin;

    LOG << "Completion time " << std::chrono::duration_cast<std::chrono::nanoseconds>(t_delta).count() / 1000000.0 << "(ms)" 
        << " empty_times " << empty_fifo_count
        << " full_times " << full_fifo_count
        << std::endl;
}

TEST_F(FIFOTest, testUnblock)
{
    FIFO<int> fifo(FIFO_CAPACITY);

    while(fifo.tryPush(0)); // full the FIFO
    LOG << "[threadMain] FIFO is full" << std::endl;

    std::thread thread_producer([&]{  
        LOG << "[threadProducer] push begin" << std::endl;
        fifo.push(0);
        LOG << "[threadProducer] unblocked!" << std::endl;
    });

    std::this_thread::sleep_for(std::chrono::seconds(1));
    LOG << "[threadMain] unblockProducer" << std::endl;
    fifo.unblockProducer();

    thread_producer.join();

    fifo.clear();

    LOG << "[threadMain] FIFO is empty" << std::endl;

    std::thread thread_consumer([&]{  
        LOG << "[threadConsumer] fifo_.waitReadAvailable()" << std::endl;
        fifo.waitReadAvailable();
        LOG << "[threadConsumer] unblocked!" << std::endl;
    });

    std::this_thread::sleep_for(std::chrono::seconds(1));
    LOG << "[threadMain] unblockConsumer" << std::endl;
    fifo.unblockConsumer();
    thread_consumer.join();
}

} // namespace

/*class FIFOTest
{
public:
    FIFOTest()
        : fifo_(FIFO_CAPACITY)
    {
        std::cout << "testPushFull() >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << std::endl;
        testPushFull() ? 
            std::cout << "testPushFull() OK >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << std::endl :
            std::cerr << "testPushFull() FAILED !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;

        std::cout << "testPerformance() >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << std::endl;;
        testPerformance() ? 
            std::cout << std::endl << "testPerformance() OK >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << std::endl :
            std::cerr << std::endl << "testPerformance() FAILED !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;

        std::cout << "testUnblock() >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << std::endl;;
        testUnblock() ? 
            std::cout << std::endl << "testUnblock() OK >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << std::endl :
            std::cerr << std::endl << "testUnblock() FAILED !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
    }

    bool testPushFull()
    {
        fifo_.clear();

        std::cout << "[threadMain] pushing till FIFO_CAPACITY...";
        for(size_t i=0;i<FIFO_CAPACITY;i++)
            fifo_.push((int)i);
        std::cout << " OK" << std::endl;

        std::cout << "[threadMain] launch threadConsumer" << std::endl;
        std::thread ([&]{
            std::cout << "[threadConsumer] wait for 1(s)..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout << "[threadConsumer] wait done. pop an element" << std::endl;
            fifo_.pop();
        }).detach();

        std::cout << "[threadMain] pushing FIFO_CAPACITY+1 (waiting for a free element)..." << std::endl;
        fifo_.push(0);
        std::cout << "[threadMain] push done" << std::endl;

        return true;
    }

    static const size_t TEST_PERFORMANCE_ELEMENTS = 1000000;

    bool testPerformance()
    {
        fifo_.clear();

        bool ret = true;

        uint32_t full_fifo_count = 0; 
        uint32_t empty_fifo_count = 0; 
        auto t_begin = std::chrono::high_resolution_clock::now();

        std::thread thread_producer([&]{  
            std::cout << "[threadProducer] pushing " << TEST_PERFORMANCE_ELEMENTS << " elements..." << std::endl;
            for(size_t i=0;i<TEST_PERFORMANCE_ELEMENTS;i++) {
                if(!fifo_.tryPush((int)i)) {
                    full_fifo_count++;
                    fifo_.push((int)i);
                }                
            }
            std::cout << "[threadProducer] done." << std::endl;
        });

        std::thread thread_consumer([&]{  
            std::cout << "[threadConsumer] poping " << TEST_PERFORMANCE_ELEMENTS << " elements..." << std::endl;
            std::string error;
            for(size_t i=0;i<TEST_PERFORMANCE_ELEMENTS;i++) {
                if(!fifo_.readAvailable()) {
                    empty_fifo_count++;
                    fifo_.waitReadAvailable();
                }
                if(fifo_.front() != (int)i) {
                    error = "[threadConsumer] element pop error at index " + std::to_string(i) + "\n";
                    break;
                }
                fifo_.pop();
            }

            if(error.empty())
                std::cout << "[threadProducer] done." << std::endl;
            else {
                ret = false;
                std::cerr << error;
            }
        });

        thread_producer.join();
        thread_consumer.join();

        auto t_delta = std::chrono::high_resolution_clock::now() - t_begin;

        std::cout << "Completion time " << std::chrono::duration_cast<std::chrono::nanoseconds>(t_delta).count() / 1000000.0 << "(ms)" 
            << " empty_times " << empty_fifo_count
            << " full_times " << full_fifo_count
            << std::endl;

        return ret;
    }

    bool testUnblock()
    {
        fifo_.clear();

        bool ret = true;

        while(fifo_.tryPush(0)); // full the FIFO
        std::cout << "[threadMain] FIFO is full" << std::endl;

        std::thread thread_producer([&]{  
            std::cout << "[threadProducer] push begin" << std::endl;
            fifo_.push(0);
            std::cout << "[threadProducer] unblocked!" << std::endl;
        });

        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "[threadMain] unblockProducer" << std::endl;
        fifo_.unblockProducer();

        thread_producer.join();

        fifo_.clear();

        std::cout << "[threadMain] FIFO is empty" << std::endl;

        std::thread thread_consumer([&]{  
            std::cout << "[threadConsumer] fifo_.waitReadAvailable()" << std::endl;
            fifo_.waitReadAvailable();
            std::cout << "[threadConsumer] unblocked!" << std::endl;
        });

        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "[threadMain] unblockConsumer" << std::endl;
        fifo_.unblockConsumer();
        thread_consumer.join();

        return ret;
    }


private:

    static const size_t FIFO_CAPACITY = 100;
    FIFO<int> fifo_;
};*/