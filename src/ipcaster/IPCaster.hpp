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

#include <mutex>
#include <iomanip>
#include <ctime>
#include <future>
#include <json.h>

#include "ipcaster/base/Logger.hpp"
#include "ipcaster/source/SourceFactory.hpp"
#include "ipcaster/net/DatagramsMuxer.hpp"
#include "ipcaster/media/Timer.hpp"

#include "FuturesCollector.hpp"
#include "Stream.hpp"

namespace ipcaster
{

/**
 * The entry object for the ipcaster app. 
 * 
 * - Initializes the basic objects
 * - Implements the main loop.
 * 
 */
class IPCaster
{
public:

    IPCaster() 
    : main_loop_timeout_(100), server_mode_(false)
    {

    }

	// More than 1(s) at 270Mbps 1 TS packet per datagram
	static const uint32_t MAX_FIFO_DATAGRAMS_PER_STREAM = 180000; 

    /**
     * Create a stream, add it to the streams list and start it
     *
     * @param json_stream The parameters of the stream in json format.
     * 
     * @throws std::exception Thrown on failure.
     */
    void createStream(Json::Value& json_stream ) {

        std::lock_guard<std::mutex> lock(streams_mutex_);

        auto udp_stream = datagrams_muxer_.createStream(json_stream["endpoint"]["ip"].asString(), 
            static_cast<uint16_t>(json_stream["endpoint"]["port"].asUInt()));

        auto source = SourceFactory<MPEG2TSFileToUDP>::create(json_stream["source"].asString(), *udp_stream);
        auto stream = std::make_shared<Stream>(json_stream, source);

        // Observe the stream to handle eof or error events
        source->attachObserver(stream);
        stream->attachObserverStrong(std::make_shared<StreamEventListener>(*this, *stream));

        streams_.push_back(stream);

        stream->start();

        Logger::get().info() << "Stream" << stream->id() << " " 
            << stream->getSourceName() << " -> " << stream->getTargetName() 
            << std::endl;
    }

    /**
     * Remove a stream. The stream is stopped and freed 
     *
     * @param stream_id Id of the stream to remove
     * 
     * @param flush If true the stream is flush when stoped
     * 
     * @throws std::exception Thrown on failure.
     */
    void deleteStream(uint32_t stream_id, bool flush = false) 
    {
        std::lock_guard<std::mutex> lock(streams_mutex_);

        auto stream = std::find_if(streams_.begin(), streams_.end(), [&] (std::shared_ptr<Stream>& stream) { 
            return stream->id() == stream_id;
            });

        assert(stream != streams_.end());

        (*stream)->stop(flush);

        streams_.erase(stream);

        Logger::get().info() << "Stream" << stream_id << " deleted" << std::endl;
    }

    /**
     * Select the sever mode (on / off)
     * 
     * If enabled the application continue running even if there isn't any work to do,
     * waiting for new streams to be added.
     * If disabled the application is finished when there're no streams to process.
     *
     * @param enable_server_mode (true/false)
     * 
     * @pre This function must be called before IPCaster::run()
     */
    void setServerMode(bool enable_server_mode) 
    {
        server_mode_ = enable_server_mode;
        // In server mode there's no console so streaming time is not printed
        // and we don't need high frequency status refresh
        main_loop_timeout_ = std::chrono::milliseconds(server_mode_ ? 1000 : 100);
    }

    /**
     * IPCaster application main loop. Does maintenance tasks until exit command is received (server mode)
     * or threre's no streams to process (command line mode)
     * 
     * @returns application's exit code.
     * 
     * @throws std::exception Thrown on failure.
     */
    int run()
    {
        while(1) {
            std::this_thread::sleep_for(main_loop_timeout_);

            // Collect global unmanaged futures already finished
            FuturesCollector::get().collect();

            printStatus();

            // If not in server mode and work is done
            if(!server_mode_ && streams_.size() == 0)
                break;
        }

        printf("\n");

        return 0;
    }

    private:

    // Streams list
    std::list<std::shared_ptr<Stream>> streams_;

    // Mutual exclusion for the streams list operations
    std::mutex streams_mutex_;

    // Server mode on/off
    bool server_mode_;

    // One datagrams muxer per ethernet interface is required to orchestrate packet order and timing for all the SMPTE2022 streams sent to that interface
    DatagramsMuxer<Timer> datagrams_muxer_;

    // Main loop maintenance tasks review period
    std::chrono::milliseconds main_loop_timeout_;

    /**
     * Handles the events produced by the streams objects
     */
    class StreamEventListener : public StreamObserver
    {
    public:

        /**
         * Constructor
         * 
         * @param ip_caster reference to the parent IPCaster object
         * 
         * @param stream reference to the stream to listen to
         */
        StreamEventListener(IPCaster& ip_caster, Stream& stream) 
        :   ip_caster_(ip_caster),
            stream_(stream)
        {
        }

    private:

        // Reference to the IPCaster parent object
        IPCaster& ip_caster_;

        // Reference to the stream to listen events to
        Stream& stream_;

        /**
         * The stream has finished so removes the stream from the system
         * 
         * @throws std::exception Thrown on failure.
         */
        void onStreamEnd()
        {
            // remove stream (async to not dead-lock )
            FuturesCollector::get().push(
                std::async(std::launch::async, [&] (IPCaster* ip_caster, uint32_t stream_id) { 
                    Logger::get().info() << "Stream" << stream_id << " Ended" << std::endl; 
                    ip_caster->deleteStream(stream_id);
                }, &ip_caster_, stream_.id())
            );
        }

        /**
         * The stream has an error so logs the error and removes the stream from the system
         * 
         * @throws std::exception Thrown on failure.
         */
        void onStreamException(std::exception& e)
        {
            // log & remove stream (async to not dead-lock )
            FuturesCollector::get().push(
                std::async(std::launch::async, [&] (IPCaster* ip_caster, uint32_t stream_id) { 
                    Logger::get().error() << "Stream[" << stream_id << "] Error - " << e.what() << std::endl; 
                    Logger::get().info() << "Stream[" << stream_id << "] Ended by and error" << std::endl; 
                    ip_caster->deleteStream(stream_id);
                }, &ip_caster_, stream_.id())
            );
        }
    }; // IPCaster::StreamEventListener

    /**
     * Called by IPCaster::run to print the current status in the console
     */
    void printStatus()
    {
        using Clock = std::chrono::system_clock;
        using TimePoint = std::chrono::time_point<Clock>;

        std::lock_guard<std::mutex> lock(streams_mutex_);

        auto streams = datagrams_muxer_.getStreams();

        if(streams.size()) {

            auto stream_time = streams[0]->getTime();
			time_t in_time_t = std::chrono::duration_cast<std::chrono::seconds> (stream_time).count();

            std::stringstream ss;
            ss << std::put_time(std::gmtime(&in_time_t), "%T");

            std::chrono::nanoseconds max_burst_duration;

            auto bandwidth = datagrams_muxer_.getOutputBandwidth(max_burst_duration);

            if(Logger::get().getVerbosity() >= Logger::Level::INFO) {
            printf("\rIP casting %u streams. Time %s.%d Bandwidth %.3fMbps Burst %.1f(ms)      ", static_cast<uint32_t>(streams.size()),
                ss.str().c_str(),
                static_cast<int>(stream_time.count()/100000000.0)%10, 
                bandwidth / 1000000.0,
                max_burst_duration.count() / 1000000.0);
            fflush(stdout);
            }
        }
    }

}; // IPCaster

}