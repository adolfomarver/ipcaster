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

#include <type_traits>

#include "ipcaster/source/StreamSource.h"
#include "ipcaster/source/FileSource.hpp"
#include "ipcaster/mpeg2-ts/MPEG2TSFileParser.hpp"
#include "ipcaster/smpte2022/SMPTE2022Encapsulator.hpp"
#include "ipcaster/net/DatagramsMuxer.hpp"
#include "ipcaster/media/Timer.hpp"

namespace ipcaster
{

/** 
 * FileSource for a mpeg2-ts file with a SMPTE2022Part2Encapsulator processor and a DatagramsMuxer::Stream consumer
 */
typedef FileSource<MPEG2TSFileParser, DatagramsMuxer<Timer>::Stream, SMPTE2022Part2Encapsulator<DatagramsMuxer<Timer>::Stream>> MPEG2TSFileToUDP;

/** 
 * Abstract sources factory
 */
template<typename TSource>
class SourceFactory
{
public:
    
};

/** 
 * MPEG2TSFileToUDP sources factory
 */
template<>
class SourceFactory<MPEG2TSFileToUDP>
{
public:
    static std::shared_ptr<MPEG2TSFileToUDP> create(const std::string& file_path, DatagramsMuxer<Timer>::Stream& consumer) 
    { 
        return std::make_shared<MPEG2TSFileToUDP>(file_path, consumer);
    }
};

}


