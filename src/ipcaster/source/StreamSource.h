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

#include "ipcaster/base/Observer.hpp"

namespace ipcaster
{

/** 
 * Interface to be implemented by the observers of the StreamSource class
 */
class StreamSourceObserver
{
public:
    /** Called when the there's no more stream to read */
    virtual void onStreamSourceEOF() = 0;
    /** Called if threre was an exception while reading the stream */
    virtual void onStreamSourceException(std::exception& e) = 0;
};

/** 
 * Base class for the StreamSource.
 * Defines functionalities that must support all the StreamSource derived objects
 */
class StreamSource : public Subject<StreamSourceObserver>
{
public:

    /** Starts the media pushing */
    virtual void start() = 0;

    /** Stops the media pushing 
     * 
     * @param flush If true the stop function blocks until all the buffered
     * media has been flushed to the consumer 
     */
    virtual void stop(bool flush = false) = 0;

    /** @returns The name of source */
    virtual std::string getSourceName() = 0;
};

}