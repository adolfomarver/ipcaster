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

#include "ipcaster/base/Logger.hpp"
#include "ipcaster/api/HTTP.hpp"
#include "ipcaster/api/APIContext.hpp"

#include "ipcaster/api/controllers/Streams.hpp"

namespace ipcaster
{
namespace api
{

/**
 * An HTTPServer to manage the IPCaster REST API
 */
class Server
{
public:

    /** Constructor
     * 
     * @param api_context Reference to the context (basically the IPCaster object)
     * 
     * @param base_uri Base URI of the REST requests
     */
    Server(std::shared_ptr<APIContext> api_context, const std::string& base_uri = "http://0.0.0.0:8080")
        : api_context_(api_context)
    {
        registerRoutes(base_uri, *api_context);

        Logger::get().info() << "REST API Server listening on " << base_uri << std::endl;
    }

private:

    /** Initilizes the listeners with all the routes supported by the API
     * every route has a listener object assigned to a controller method */
    void registerRoutes(const std::string& base_uri, APIContext& api_context)
    {
        // /streams
        listeners_.push_back(std::make_shared<Listener>(UTF16(base_uri + "/streams")));
        controllers::Streams::registerMethods(*listeners_.back(), api_context);
        listeners_.back()->open().then([](pplx::task<void> t) { handleError(t); });
    }

    static void handleError(pplx::task<void>& t)
    {
        try {
            t.get();
        }
        catch (std::exception& e) {
            Logger::get().fatal() << logstaticfn(Server) << e.what() << std::endl;
            Logger::get().fatalErrorExitApp(1);
        }
    }

    std::vector<std::shared_ptr<Listener>> listeners_;
    std::shared_ptr<APIContext> api_context_;
};

}
}