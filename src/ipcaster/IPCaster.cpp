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

#include "IPCaster.h"

#include <iomanip>
#include <ctime>

#include "ipcaster/base/Logger.hpp"
#include "ipcaster/source/SourceFactory.hpp"
#include "ipcaster/api/Server.hpp"

using namespace ipcaster;

IPCaster::IPCaster() 
: main_loop_timeout_(100), service_mode_(false)
{

}
    
web::json::value IPCaster::createStream(web::json::value json_stream ) 
{
    std::lock_guard<std::mutex> lock(streams_mutex_);

    auto udp_stream = datagrams_muxer_.createStream(UTF8(json_stream[U("endpoint")][U("ip")].as_string()),
        static_cast<uint16_t>(json_stream[U("endpoint")][U("port")].as_integer()));

    auto source = SourceFactory<MPEG2TSFileToUDP>::create(UTF8(json_stream[U("source")].as_string()), *udp_stream);
    auto stream = std::make_shared<Stream>(json_stream, source);

    // Observe the stream to handle eof or error events
    source->attachObserver(stream);
    stream->attachObserverStrong(std::make_shared<StreamEventListener>(*this, *stream));

    streams_.push_back(stream);

    stream->start();

    Logger::get().info() << "Stream created: stream_id = " << stream->id() << " " 
        << stream->getSourceName() << " -> " << stream->getTargetName() 
        << std::endl;

    return stream->json();
}

void IPCaster::deleteStream(uint32_t stream_id, bool flush) 
{
    std::lock_guard<std::mutex> lock(streams_mutex_);

    auto stream = std::find_if(streams_.begin(), streams_.end(), [&] (std::shared_ptr<Stream>& stream) { 
        return stream->id() == stream_id;
        });

    if(stream == streams_.end())
        throw ipcaster::Exception("Stream with streamId " + std::to_string(stream_id) + " not found");

    (*stream)->stop(flush);

    streams_.erase(stream);

    Logger::get().info() << "Stream deleted: stream_id = " << stream_id <<  std::endl;
}

web::json::value IPCaster::listStreams()
{
    std::lock_guard<std::mutex> lock(streams_mutex_);

    web::json::value json_streams;

    int index = 0;

    for(auto stream : streams_)
        json_streams[index++] = stream->json();

    return json_streams;
}

void IPCaster::setServiceMode(bool enable_server_mode, uint16_t listening_port) 
{
    service_mode_ = enable_server_mode;
    service_port_ = listening_port;
    // In service mode there's no console so streaming time is not printed
    // and we don't need high frequency status refresh
    main_loop_timeout_ = std::chrono::milliseconds(service_mode_ ? 1000 : 100);
}

int IPCaster::run()
{
    if(service_mode_) {
        Logger::get().info() << "IPCaster service running." << std::endl;
        api_server_ = std::make_shared<api::Server>(std::make_shared<api::APIContext>(*this),"http://0.0.0.0:" + std::to_string(service_port_) + "/api");
    }

    while(1) {
        std::this_thread::sleep_for(main_loop_timeout_);

        // Collect global unmanaged futures already finished
        FuturesCollector::get().collect();

        // In service mode no status is printed
        if(!service_mode_)
            printStatus();

        // If not in service mode and work is done
        if(!service_mode_ && streams_.size() == 0)
            break;
    }

    printf("\n");

    return 0;
}

void IPCaster::stop()
{
    printf("stop");
}

void IPCaster::printStatus()
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