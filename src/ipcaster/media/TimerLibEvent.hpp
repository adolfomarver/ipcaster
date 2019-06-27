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
#include <mutex>
#include <condition_variable>

#include <event2/event.h>

#include "ipcaster/base/Exception.hpp"
#include "ipcaster/base/Logger.hpp"

namespace ipcaster
{

/**
 * Implements an accurate waitable Timer 
 * This implementation rely on libEvent2
 */
class TimerLibEvent
{
public: 

    /** Constructor
     * Launches the timer thread
     * 
     * @param period Period of the timer in nanoseconds
     * 
     * @throws std::exception
     */
    TimerLibEvent(const std::chrono::nanoseconds& period) 
    : period_(period), interrupt_received_(false), event_base_(nullptr), timer_alive_(true)
    {
        timer_thread_ = std::thread(&TimerLibEvent::timerThread, this);
    }

    /** Destructor 
     * Stops the timer and exits the timer thread
     * @throws std::system_error if an error occurs.
     */
    ~TimerLibEvent() 
    {
        if(event_base_)
            event_base_loopbreak(event_base_);

        if(timer_thread_.joinable())
            timer_thread_.join();
    }

    /** 
    * Initializes the timer and run libEvent loop until object is deleted
    */
    void timerThread()
    {
        using namespace std::chrono;

        try {
            event_base_ = event_base_new();
            struct timeval timeout = { 0, duration_cast<microseconds>(period_).count() };

            // set a persistent timeout 
            auto timeout_event = event_new(event_base_, -1, EV_PERSIST, &TimerLibEvent::onTimeout, this);
            if (!timeout_event)
                throw Exception(fndbg(TimerLibEvent) + " event_new failed ");
            event_add(timeout_event, &timeout);

            // run the event loop until interrupted 
            event_base_dispatch(event_base_);

            // clean up 
            event_free(timeout_event);
            event_base_free(event_base_);

            // Simulate a timer interrupt to unblock any thread blocked in the wait() call
            std::unique_lock<std::mutex> lock(timer_mutex_);
            timer_alive_ = false;
            interrupt_received_ = true;
            on_timer_cv_.notify_one();
        }
        catch(std::exception &e) {
            Logger::get().fatal() << e.what();
        }
    }

    /** 
    * Waits until timer is triggered
    * If the timer had been already triggered the received flag is reset
    * and no wait is done.
    * 
    * @returns The current std::high_resolution_clock time.
    * 
    * @note Concurrency is not supported so there should only be one thread
    * calling wait at a time.
    */
    std::chrono::high_resolution_clock::time_point wait() 
    {
        std::unique_lock<std::mutex> lock(timer_mutex_);

        if(timer_alive_) {
            on_timer_cv_.wait(lock, [&]{return interrupt_received_;});
            interrupt_received_ = false;
        }

        return std::chrono::high_resolution_clock::now();
    }

    /** @returns The period of the timer */
    inline std::chrono::nanoseconds period() { return period_; }

private:

    event_base* event_base_;

    std::thread timer_thread_;

    std::chrono::nanoseconds period_;

    std::condition_variable on_timer_cv_;
    bool interrupt_received_;
    std::mutex timer_mutex_;

    // Avoids waiting when timer is already killed
    bool timer_alive_;

    /** Called by libEvent when timer period has elapsed */
    static void onTimeout(evutil_socket_t fd, short events, void *arg)
    {
        auto timer = static_cast<TimerLibEvent*>(arg);        
        timer->interrupt_received_ = true;
        timer->on_timer_cv_.notify_one();
    }
};

}