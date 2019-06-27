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

#include <string>
#include <cstdio>
#include <cerrno>
#include <memory>
#include <ios>
#include <thread>

#include "ipcaster/base/Exception.hpp"
#include "ipcaster/base/Buffer.hpp"
#include "ipcaster/base/FIFO.hpp"
#include "ipcaster/source/StreamSource.h"

namespace ipcaster
{

/**
 * Represents an abstract media stream source from a file.
 * Handles file opening / reading / buffering / pushing.
 * The file's content is push to a consumer
 * Producer/Consumer pattern with threaded buffered IO is used to improve performance.
 * 
 * @param FileParser Internal parser class that will be used to parse the media
 * 
 * @param Consumer class of the Consumer where the media will be pushed
 * 
 * @param Processor class of the object that process the media before it's pushed 
 * to the Consumer
 */
template<class FileParser, class Consumer, class Processor>
class FileSource : public StreamSource
{
public:

    /** Constructor
     * Opens the file and try to parse it
     * 
     * @param file Complete path of the file
     * 
     * @param consumer Reference to the consumer object
     * 
     * @throws std::exception If an error occurs.
     */
    FileSource(const std::string& file, Consumer& consumer)
        : parser_(file), processor_(consumer)
    {
        fifo_ = std::make_unique<FIFO<std::shared_ptr<Buffer>>>(parser_.estimatedBuffersPerSecond());

        source_name_ = file;
    }

    /** 
     * Launches the producer and consumer threads and start pushing media to the 
     * consumer
     * 
     * @throws std::exception If an error occurs.
     */
    void start()
    {
        if(thread_consumer_.joinable())
            throw Exception("FileSource::start() - already started");

        exit_threads_ = false;
        eof_reached_ = false;

        thread_consumer_ = std::thread(&FileSource<FileParser, Consumer, Processor>::threadConsumer, this);
        thread_producer_ = std::thread(&FileSource<FileParser, Consumer, Processor>::threadProducer, this);

        Logger::get().debug() << logfn(FileSource) << "OK"<< std::endl;
    }

    /** 
     * Stops the pushing
     * 
     * @param flush If true the stop function blocks until all the buffered
     * 
     * @throws std::exception If an error occurs.
     */
    void stop(bool flush = false)
    {
        Logger::get().debug() << logfn(FileSource) << "In..." << std::endl;

        if(!thread_producer_.joinable())
            throw Exception("FileSource::stop() - not started");

        exit_threads_ = true;
        fifo_->unblockProducer();
        fifo_->unblockConsumer();

        thread_producer_.join();
        thread_consumer_.join();

        if(flush)
            processor_.flush();

        Logger::get().debug() << logfn(FileSource) << "OK" << std::endl;
    }

    /** @returns The name of source */
    std::string getSourceName()
    {
        return source_name_;
    }

private:

    // This object process the media from the file before pushing it to the consumer
    Processor processor_;

    // Underlying specific file parser object
    FileParser parser_;

    // FIFO where producer thread pushes the stream buffers and consumer thread pops them
    std::unique_ptr<FIFO<std::shared_ptr<Buffer>>> fifo_;

    // Threads handlers
    std::thread thread_producer_;
    std::thread thread_consumer_;
    bool exit_threads_;

    // User understandable source's name
    std::string source_name_;

    // The end of the file has been reached
    bool eof_reached_;

    /** 
     * Producer loop.
     * Reads from the file parser, until eof or error, and push to the fifo 
     */
    void threadProducer()
    {
        try {
            std::shared_ptr<Buffer> buffer = parser_.read();

            while(buffer && !exit_threads_) {

                fifo_->push(buffer); 

                try {
                    buffer = parser_.read();
                }
                catch(std::exception& e) {
                    notifyException(e);
                    buffer = nullptr;
                }
            }

            if(!buffer) {
                eof_reached_ = true;
                fifo_->unblockConsumer();
            }
        }
        catch(std::exception& e) {
            notifyException(e); 
        }
    }

    /** 
     * Consumer loop.
     * Pops media from the fifo and push it to the processor.
     * The processor will push the processed media to the consumer
     */
    void threadConsumer()
    {
        try {

            while(!exit_threads_) {
                if(fifo_->waitReadAvailable()) {
                    processor_.push(fifo_->front());
                    fifo_->pop();
                }
                else if(eof_reached_) {
                    exit_threads_ = true;
                    notifyEOF();     
                }
            }
        }
        catch(std::exception& e) {
            notifyException(e);
        }
    }

    /** Notifies EOF to the observers */
    void notifyEOF()
    {
        for(auto& observer : observers_) {
            if(auto ob = observer.observer_weak.lock()) 
                ob->onStreamSourceEOF();
        }
    }

    /** Notifies an exception to the observers */
    void notifyException(std::exception& e)
    {
        for(auto& observer : observers_) {
            if(auto ob = observer.observer_weak.lock())
                ob->onStreamSourceException(e);
        }
    }
};

}

