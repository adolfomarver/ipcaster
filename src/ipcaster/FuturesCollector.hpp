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

#include <future>
#include <list>

#include "ipcaster/base/Logger.hpp"

namespace ipcaster
{

/**
 * This singleton class provide functionality to store unmanaged futures.
 * Can be used anyware in the whole application by components that execute async 
 * operations that don't need to optain / wait for any result.
 * 
 * The ipcaster main loop call FuturesCollector::collect from time to time 
 * to erase the already finished futures.
 * 
 */
class FuturesCollector
{
public: 

    /**
     * @returns A reference to the singleton 
     */
    static FuturesCollector& get()
    {
        static FuturesCollector singleton;
        return singleton;
    }

    /**
     * Enqueue a future
     * 
     * @param future Reference to a future that will be enqueued with move semantic
     */
    void push(std::future<void>&& future) 
    {
        std::lock_guard<std::mutex> lock(futures_mutex_);

        futures_.push_back(std::move(future));
    }

    /**
     * Iterates through the enqueued futures erasing the finished ones. 
     * If the futures threw exceptions these are just logged as errors.
     */
    void collect()
    {
        std::lock_guard<std::mutex> lock(futures_mutex_);

        auto future = futures_.begin();
        while(future != futures_.end()) {
            if(future->wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
                try {
                    future->get();
                }
                catch(std::exception& e) {
                    Logger::get().error() << fndbg(IPCaster) << e.what() << std::endl;
                }

                futures_.erase(future++);
            }
            else
                future++;
        }
    }

private:

    // List of futures
    std::list<std::future<void>> futures_;

    // Mutex for the list
    std::mutex futures_mutex_;
};

}