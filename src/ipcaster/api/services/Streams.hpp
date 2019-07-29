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

#include <cpprest/json.h>

#include "ipcaster/api/APIContext.hpp"

namespace ipcaster
{
namespace api
{
namespace services
{

/**
 * Service for Streams
 */
class Streams
{
public:
    
    static web::json::value list(APIContext& context)
    {
        web::json::value ret;
        auto streams_array = context.ipcaster().listStreams();

        if(streams_array.is_null())
            ret[U("streams")] = web::json::value::array();
        else 
            ret[U("streams")] = streams_array;

        return ret;
    }

    static web::json::value create(web::json::value request_body, APIContext& context)
    {
        return context.ipcaster().createStream(request_body);
    }

    static void del(const std::string& stream_id, APIContext& context)
    {
        context.ipcaster().deleteStream(std::stoi(stream_id));
    }
};

}
}
}