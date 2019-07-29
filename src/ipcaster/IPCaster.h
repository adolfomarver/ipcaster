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
#include <future>

#include <cpprest/json.h>

#include "ipcaster/net/DatagramsMuxer.hpp"
#include "ipcaster/media/Timer.hpp"

#include "FuturesCollector.hpp"
#include "Stream.hpp"

namespace ipcaster
{

namespace api
{
    class Server;
}

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

    IPCaster();
    
	// More than 1(s) at 270Mbps 1 TS packet per datagram
	static const uint32_t MAX_FIFO_DATAGRAMS_PER_STREAM = 180000; 

    /**
     * Create a stream, add it to the streams list and start it
     *
     * @param json_stream The parameters of the stream in json format.
     * 
     * @returns Json object with the new stream_id
     * 
     * @throws std::exception Thrown on failure.
     */
    web::json::value createStream(web::json::value json_stream );

    /**
     * Remove a stream. The stream is stopped and freed 
     *
     * @param stream_id Id of the stream to remove
     * 
     * @param flush If true the stream is flush when stoped
     * 
     * @throws std::exception Thrown on failure.
     */
    void deleteStream(uint32_t stream_id, bool flush = false);

    /**
     * @returns An array with the running streams
     */
    web::json::value listStreams();

    /**
     * Select the sever mode (on / off)
     * 
     * If enabled the application continue running even if there isn't any work to do,
     * waiting for new streams to be added.
     * If disabled the application is finished when there're no streams to process.
     *
     * @param enable_server_mode (true/false)
     * 
     * @param listening_port In case service_mode is enabled this variable indicate the
     * listening port
     * 
     * @pre This function must be called before IPCaster::run()
     */
    void setServiceMode(bool enable_server_mode, uint16_t listening_port = 8080); 

    /**
     * IPCaster application main loop. Does maintenance tasks until exit command is received (server mode)
     * or threre's no streams to process (command line mode)
     * 
     * @returns application's exit code.
     * 
     * @throws std::exception Thrown on failure.
     */
    int run();

    void stop();

    private:

    // Streams list
    std::list<std::shared_ptr<Stream>> streams_;

    // Mutual exclusion for the streams list operations
    std::mutex streams_mutex_;

    // Server mode on/off
    bool service_mode_;

    // Service listening port
    uint16_t service_port_;

    // One datagrams muxer per ethernet interface is required to orchestrate packet order and timing for all the SMPTE2022 streams sent to that interface
    DatagramsMuxer<Timer> datagrams_muxer_;

    // Main loop maintenance tasks review period
    std::chrono::milliseconds main_loop_timeout_;

    // REST api server
    std::shared_ptr<api::Server> api_server_;

    /**
     * Called by IPCaster::run to print the current status in the console
     */
    void printStatus();

    /**
     * Called every time an stream is created or deleted
     */
    void traceMuxerStatus();

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

}; // IPCaster

}