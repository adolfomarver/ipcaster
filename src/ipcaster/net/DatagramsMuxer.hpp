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

#include <iostream>
#include <mutex>
#include <list>

#include "ipcaster/base/FIFO.hpp"
#include "ipcaster/base/Logger.hpp"
#include "ipcaster/net/Datagram.hpp"
#include "ipcaster/net/UDPSender.hpp"

namespace ipcaster
{

/**
 * Performs UDP datagrams send with a (timed-based) scheduling scheme.
 * Support may streams, the datagrams of the streams are interleaved
 * before sending to comply as much as posible with the timepoints 
 * specified for the datagrams
 */
template <class Timer>
class DatagramsMuxer
{
    using Clock = std::chrono::high_resolution_clock;

public:

    /** Constructor
     * Initializes the object. Launch the sender thread
     * 
     * @param burst_period The period the thread will wait between 
     * sends in milliseconds, this will influence the size of minimum 
     * burst send by the ethernet.
     * 
     * @throws std::exception 
     */
    DatagramsMuxer(std::chrono::milliseconds burst_period = std::chrono::milliseconds(4)) :
        timer_(burst_period), 
        exit_threads_(false)
    {
        send_stats_.max_prepare_ms = 0;
        send_stats_.max_send_ms = 0;
        send_stats_.max_timer_ms = 0;
        send_stats_.min_prepare_ms = std::numeric_limits<float>::max();
        send_stats_.min_send_ms = std::numeric_limits<float>::max();
        send_stats_.min_timer_ms = std::numeric_limits<float>::max();
        send_stats_.high_burst_count_ = 0;

        thread_sender_ = std::thread(&DatagramsMuxer<Timer>::threadSender, this);
    }

    /** Destructor
     * Stops the sending thread
     * @throws std::system_error if an error occurs.
     */
    ~DatagramsMuxer()
    {
        exit_threads_ = true;

        if(thread_sender_.joinable())
            thread_sender_.join();
    }

    /** 
     * Represents a stream where timed datagrams are push from a producer.
     * Has a fifo to store the datagrams until time to send is reached.
     * The endpoint for the datagrams is also an attribute of this class.
     */
    class Stream 
    {

    public:

        /** Constructor
         * 
         * @param target_ip Target IP of the endpoint
         * 
         * @param target_port Target port of the endpoint
         * 
         * @fifo_num_datagrams Size of the fifo in datagrams
         */
        Stream(const std::string& target_ip, uint16_t target_port, size_t fifo_num_datagrams)
            :   target_ip_(target_ip), 
                target_port_(target_port), 
                fifo_(fifo_num_datagrams),
                is_sync_point_set_(false),
                is_start_point_set_(false),
                tail_send_tick_(std::chrono::time_point<Clock>())

        {
            last_popped_datagram_tick_.store(0, std::memory_order::memory_order_relaxed);
        }

        /** 
         * Enqueues a datagram in the fifo.
         * The first datagram sets time base for sending, so the first datagram tick
         * will be "equal" to the current clock time of the DatagramsMuxer and the next
         * datagrams will be send on that time base.
         * 
         * @param datagram Shared pointer to the datagram to enqueue
         */
        inline void push(std::shared_ptr<Datagram> datagram) { 

            if(!is_sync_point_set_) {
                sync_point_ = datagram->sendTick();
                is_sync_point_set_ = true;
            }

            datagram->setTargetIP(target_ip_);
            datagram->setTargetPort(target_port_);

            fifo_.push(datagram); 

            tail_send_tick_.store(datagram->sendTick());
        }

        /** 
         * Pops the next datagram of the stream if the time of the datagram has expired
         * 
         * @param now The current time of the send timer
         * 
         * @returns The front datagram of the fifo if its send_tick < now, otherwise returns a void datagram
         */
        std::shared_ptr<Datagram> popFrontDatagramElegible(const Clock::time_point& now)
        {
            std::shared_ptr<Datagram> datagram;

            if(fifo_.readAvailable()) {

                if(!is_start_point_set_) {
                    start_point_ = now;
                    is_start_point_set_ = true;
                }

                auto normalized_datagram_tick = fifo_.front()->sendTick() - sync_point_ + start_point_;
                if(normalized_datagram_tick < now) {
                    datagram = fifo_.front();
                    fifo_.pop();
                    last_popped_datagram_tick_.store(datagram->sendTick().time_since_epoch().count(), std::memory_order_relaxed);
                }
            }

            return datagram; 
        }

        /** 
         * Blocks until all the buffered data has been processed
         */
        void flush()
        {
            // Active wait is not the best way to do this, but for flush
            // a 100(ms) latency is tolerable, could be improved if
            // necesary
            while(fifo_.readAvailable()) 
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        /** @returns The total amount of stream time (in milliseconds) buffered in the fifo */
        std::chrono::milliseconds bufferedTime()
        {
            if(fifo_.readAvailable())
                return std::chrono::duration_cast<std::chrono::milliseconds>(tail_send_tick_.load(std::memory_order_relaxed) - fifo_.front()->sendTick());
            else
                return std::chrono::milliseconds(0);
        }

        /** @returns The current stream time */
        std::chrono::nanoseconds getTime()
        {
            std::chrono::nanoseconds ret;
            auto stream_pos = std::chrono::nanoseconds(last_popped_datagram_tick_.load(std::memory_order_relaxed));

            if(stream_pos > std::chrono::nanoseconds(0)) {

                Clock::time_point t1(stream_pos);

                ret =  t1 - sync_point_;
            }
            else
                ret = std::chrono::nanoseconds(0);

            return ret;
        }
        
    private:

        std::string target_ip_;

        uint16_t target_port_;

        FIFO<std::shared_ptr<Datagram>> fifo_;

        // Send tick of last datagram in the fifo
        std::atomic<Clock::time_point> tail_send_tick_;
        
        // If false the base time for the stream will be synched to the DatagramMuxer 
        // timer clock when next datagram arrives
        bool is_sync_point_set_;

        // Marks the base time for the stream (time 0)
        Clock::time_point sync_point_;

        // If false the start point has to be set
        bool is_start_point_set_;

        // The time (DatagramsMuxer clock) of the first datagram of the stream
        Clock::time_point start_point_;

        // Last popped datagram time
        std::atomic<Clock::time_point::rep> last_popped_datagram_tick_;

    }; // DatagramsMuxter::Stream

    /** @returns A vector of references to the streams of the DatagramsMuxer */
    std::vector<std::shared_ptr<Stream>> getStreams() 
    {
        return streams_;
    }

    /**
     * Creates a new stream in the DatagramsMuxer 
     * 
     * @param target_ip Destination IP for all the datagrams pushed to the stream
     * 
     * @param target_port Destination port for all the datagrams pushed to the stream
     * 
     * @param fifo_num_datagrams Number of datagram references that can be stored in the stream's fifo
     * 
     * @returns A reference to the new stream
     */
    std::shared_ptr<Stream> createStream(const std::string& target_ip, uint16_t target_port, size_t fifo_num_datagrams)
    {
        std::lock_guard<std::mutex> lock(mutex_streams_);

        streams_.push_back(std::make_shared<Stream>(target_ip, target_port, fifo_num_datagrams));

        return streams_.back();
    }

    /** @returns A string with sending statistics */
    std::string stats() 
    {
        char str[1024];

        // if stats are initialized
        if(send_stats_.max_timer_ms.load() > 0.001) {
            snprintf(str, sizeof(str), "timer(ms) [%.3f,%.3f] prepare [%.3f,%.3f] send [%.3f,%.3f] highburst %u", 
                send_stats_.min_timer_ms.load(std::memory_order_relaxed),
                send_stats_.max_timer_ms.load(std::memory_order_relaxed),
                send_stats_.min_prepare_ms.load(std::memory_order_relaxed),
                send_stats_.max_prepare_ms.load(std::memory_order_relaxed),
                send_stats_.min_send_ms.load(std::memory_order_relaxed),
                send_stats_.max_send_ms.load(std::memory_order_relaxed),
                send_stats_.high_burst_count_.load(std::memory_order_relaxed));
        }
        else {
            str[0] = 0;
        }
        
        return str;
    }

    /**
     * @param [out] max_burst Maximum recent burst duration 
     * @returns The current output bandwidth 
     */
    uint64_t getOutputBandwidth(Clock::duration& max_burst)
    {
        std::vector<std::pair<Clock::time_point, size_t>> burst_sizes;

        {
            std::lock_guard<std::mutex> lock(mutex_burst_sizes_);
            burst_sizes = last_bursts_sizes_;
        }

        uint64_t bitrate = 0;
        max_burst = std::chrono::nanoseconds(0);
        Clock::time_point prev_burst_t(max_burst);

        if(burst_sizes.size() > 1) {

            size_t bytes = 0;

            for(auto& burst : burst_sizes) {
                if(prev_burst_t.time_since_epoch().count() > 0) {
                    auto burst_delta = burst.first - prev_burst_t;
                    if(burst_delta > max_burst)
                        max_burst = burst_delta;
                }
                bytes += burst.second;
                prev_burst_t = burst.first;
            }

            auto nanoseconds = (burst_sizes.back().first - burst_sizes.front().first);

            return bytes * 8 * 1000000000 / nanoseconds.count();
        }

        return bitrate;
    }

private:

    // Main loop thread of the DatagramsMuxer
    std::thread thread_sender_;

    // When true the thread_sender exits 
    bool exit_threads_;

    // Streams added to the DatagramsMuxer
    std::vector<std::shared_ptr<Stream>> streams_;

    // Mutex for the streams_ vector
    std::mutex mutex_streams_;

    // The thread_sender_ waits on this timer to time the datagram burst sending
    Timer timer_;

    // Sender UDP socket
    UDPSender sender_;

    // For send timming statistics purposes
    Clock::time_point t_last_burst_;

    // Last bursts sizes and times useful for output bitrate estimation
    std::vector<std::pair<Clock::time_point, size_t>> last_bursts_sizes_;

    std::mutex mutex_burst_sizes_;

    /** 
     * A vector of datagrams references and their endpoints that will
     * be send in a burst (at a time)
     */
    struct Burst
    {
        std::vector<ip::udp::endpoint> endpoints;
        std::vector<std::shared_ptr<Datagram>> datagrams;
        size_t size;
        inline void clear() { datagrams.clear(); endpoints.clear(); size = 0; }
    };

    /** 
     * Main loop of the sending process:
     * - Waits for an interrupt from the timer.
     * - Prepare a burst with the datagrams, from all streams, which 
     * send_tick < now.
     * - Sends the datagrams to their endpoints
     */
    void threadSender()
    {
        Burst burst;

        burst.clear();

        while(!exit_threads_) {

            auto now = timer_.wait();

            prepareBurst(now, burst);
            auto t_prepare = Clock::now();

            sendBurst(burst);
            auto t_send = Clock::now();

            if(burst.datagrams.size() > 0)
                keepSendStats(now, t_last_burst_, t_prepare, t_send, burst);

            burst.clear();
            t_last_burst_ = now;
        }
    }

    /** 
     * Build the burst with the datagrams that already expired
     * @todo Work in the multiplexing algorithm to improve efficiency
     */
    void prepareBurst(const Clock::time_point& now, Burst& burst)
    {
        bool datagrams_added = false;

        do
        {
            datagrams_added = false;

            // Check front packet of every stream, if send_tick < now  then (is elegible) move it to burst
            for(auto& stream : streams_) {
                if(auto datagram = stream->popFrontDatagramElegible(now)) {
                    burst.endpoints.emplace_back(datagram->endpoint());
                    burst.datagrams.push_back(datagram);
                    datagrams_added = true;
                    burst.size += datagram->payload()->size();
                }
            }

        }while(datagrams_added);
    }

    /** Sends a group of datagrams to their endpoints */
    void sendBurst(Burst& burst)
    {
        auto num_datagrams = burst.datagrams.size();

        for(size_t i=0; i<num_datagrams; i++) {
            sender_.send(burst.endpoints[i], 
                boost::asio::buffer((const void*)burst.datagrams[i]->payload()->data(), 
                burst.datagrams[i]->payload()->size()));
        }
    }

    /** Stores send timming statistics */
    void keepSendStats( const Clock::time_point& now,
                        const Clock::time_point& t_last_burst,
                        const Clock::time_point& t_prepare,
                        const Clock::time_point& t_send,
                        const Burst& burst)
    {
        // Avoid first call because t_last_burst is not initialized
        if(send_stats_.max_timer_ms < 0.001) {
            send_stats_.max_timer_ms = 0.001;
            return;
        }

        auto timer_delta = now - t_last_burst_;
        auto prepare_time = t_prepare - now;
        auto send_time = t_send - t_prepare;

        float timer_delta_ms = std::chrono::duration_cast<std::chrono::nanoseconds>(timer_delta).count() / 1000000.0;
        float prepare_time_ms = std::chrono::duration_cast<std::chrono::nanoseconds>(prepare_time).count() / 1000000.0;
        float send_time_ms = std::chrono::duration_cast<std::chrono::nanoseconds>(send_time).count() / 1000000.0;

        if(timer_delta_ms > send_stats_.max_timer_ms)
            send_stats_.max_timer_ms.store(timer_delta_ms, std::memory_order_relaxed);
        
        if(timer_delta_ms < send_stats_.min_timer_ms)
            send_stats_.min_timer_ms.store(timer_delta_ms, std::memory_order_relaxed);

        if(prepare_time_ms > send_stats_.max_prepare_ms)
            send_stats_.max_prepare_ms.store(prepare_time_ms, std::memory_order_relaxed);
        
        if(prepare_time_ms < send_stats_.min_prepare_ms)
            send_stats_.min_prepare_ms.store(prepare_time_ms, std::memory_order_relaxed);

        if(send_time_ms > send_stats_.max_send_ms)
            send_stats_.max_send_ms.store(send_time_ms, std::memory_order_relaxed);
        
        if(send_time_ms < send_stats_.min_send_ms)
            send_stats_.min_send_ms.store(send_time_ms, std::memory_order_relaxed);

        if(std::chrono::milliseconds((uint32_t)timer_delta_ms) >= timer_.period() + std::chrono::milliseconds(2)) {
            send_stats_.high_burst_count_.fetch_add(1, std::memory_order_relaxed);
            Logger::get().debug(1) << logclass(DatagramsMuxer) << "High burst period! - " << burstTrace(timer_delta_ms, prepare_time_ms, send_time_ms) << std::endl;
        }

        keepBitrateStats(now, burst);
    }

    /** Stores burst timing and size in a list to bandwith later estimation */
    void keepBitrateStats(const Clock::time_point& now, const Burst& burst)
    {
        std::lock_guard<std::mutex> lock(mutex_burst_sizes_);

        if(last_bursts_sizes_.size() > 1) {
            auto list_duration = last_bursts_sizes_.back().first - last_bursts_sizes_.front().first;
            // We already have enough data then drop the older register
            if(list_duration >= std::chrono::seconds(1))
                last_bursts_sizes_.erase(last_bursts_sizes_.begin());
        }

        last_bursts_sizes_.push_back(std::pair<Clock::time_point, size_t>(now, burst.size));
    }

    /** 
     * Converts burst timming data to string
     * @returns A string with burst statistics 
     * */
    std::string burstTrace(float timer_delta_ms, float prepare_time_ms, float send_time_ms) 
    {
        char str[1024];

        snprintf(str, sizeof(str), "timer(ms) %.3f prepare %.3f send %.3f", 
            timer_delta_ms,
            prepare_time_ms,
            send_time_ms);

        return str;
    }

    // Holds send statistics
    struct {
        std::atomic<float> max_timer_ms;
        std::atomic<float> min_timer_ms;
        std::atomic<float> max_prepare_ms;
        std::atomic<float> min_prepare_ms;
        std::atomic<float> max_send_ms;
        std::atomic<float> min_send_ms;
        std::atomic<uint32_t> high_burst_count_;
    } send_stats_;

}; // DatagramsMuxer

}
