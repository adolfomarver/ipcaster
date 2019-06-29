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
#include <json.h>

#include "ipcaster/base/Observer.hpp"
#include "ipcaster/source/StreamSource.h"

namespace ipcaster
{

/**
 * This interface declare the observable events for the Stream sub-classes
 */
class StreamObserver
{
public:

    /**
     * Called when the stream has ended
     */
    virtual void onStreamEnd() = 0;

    /**
     * Called when the stream had an exception while running
     */
    virtual void onStreamException(std::exception& e) = 0;
};

/**
 * Base class for the ipcaster streams
 * Provide basic common functionalies
 */
class Stream :  public Subject<StreamObserver>,
                public StreamSourceObserver
{
public:

    /**
     * Constructor
     * 
     * Initialize references and generate an UID for the stream
     * 
     * @param stream_json Parameters of the stream in json format
     * 
     * @param source Shared pointer to the source of the stream
     */
    Stream(Json::Value& stream_json, std::shared_ptr<StreamSource> source)
        :   stream_json_(std::move(stream_json_)),
            source_(source)
    {
        stream_json_ = stream_json;

        id_ = IDSingleton::next();
        stream_json_["id"] = id_;
    }

    /**
     * @returns The current parameters of the stream
     */
    inline const Json::Value& json() const { return stream_json_; }

    /**
     * @returns The id of the stream
     */
    inline uint32_t id() const { return id_; }

    /**
     * Start the stream
     */
    void start() { source_->start(); }

    /**
     * Stop the stream
     * 
     * @param flush If true the stop function blocks until all the data 
     * has been flushed to the consumer
     */
    void stop(bool flush = false) { source_-> stop(flush); }

    /** @returns The source's name */
    std::string getSourceName() { return source_->getSourceName();}

    /** 
     * @returns The target's name 
     * @throws std::exception 
     */
    std::string getTargetName() 
    { 
        return stream_json_["endpoint"]["ip"].asString() + ":" + stream_json_["endpoint"]["port"].asString();
    }

private:

    // Stream ID
    uint32_t id_;

    // Current parameters and status of the stream
    Json::Value stream_json_;

    // Strong reference to the source of the stream
    std::shared_ptr<StreamSource> source_;

    /**
     * Singleton that generates unique ids for the streams
     */
    class IDSingleton
    {
    public:
        /**
        * Get the next UID
        * @par Thread safe
        */
        static uint32_t next() {
            static std::atomic<uint32_t> uid{0};
            return uid.fetch_add(1, std::memory_order_relaxed);
        }
    };

    /**
     * Called by the Source
     */
    void onStreamSourceEOF()
    {
        notifyEOF();
    }

    /**
     * Notify the end of the stream to the our observers
     */
    void notifyEOF()
    {
        for(auto& observer : observers_) {
            if(auto ob = observer.observer_weak.lock())
                ob->onStreamEnd();
        }
    }

    /**
     * Called by the Source
     */
    void onStreamSourceException(std::exception& e)
    {
        notifyException(e);
    }

    /**
     * Notify an exception while processing to our observers
     */
    void notifyException(std::exception& e)
    {
        for(auto& observer : observers_) {
            if(auto ob = observer.observer_weak.lock())
                ob->onStreamException(e);
        }
    }
}; // Stream

}