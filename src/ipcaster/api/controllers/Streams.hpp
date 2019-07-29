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

#include <functional>

#include "ipcaster/api/APIContext.hpp"
#include "ipcaster/api/HTTP.hpp"
#include "ipcaster/api/services/Streams.hpp"

namespace ipcaster
{
namespace api
{
namespace controllers
{

/**
 * Controller for Streams
 */
class Streams
{
public:

    static void registerMethods(Listener& listener, APIContext& context) 
    {   
        listener.support(Methods::GET, std::bind(Streams::get, std::placeholders::_1, context));
        listener.support(Methods::POST, std::bind(Streams::post, std::placeholders::_1, context));
        listener.support(Methods::DEL, std::bind(Streams::del, std::placeholders::_1, context));
    }

    static void get(Request const& request, APIContext& context) 
    {
        try {
            request.reply(StatusCodes::OK, services::Streams::list(context));
        }
        catch(std::exception& e) {
            Logger::get().error() << logstaticfn(Streams) << e.what() << std::endl;
            request.reply(StatusCodes::InternalError, Response::error(StatusCodes::InternalError, e.what()));
        }
    }

    static void post(Request const& request, APIContext& context) 
    {
        try {
            web::json::value ret;

            auto a = request.extract_json().then([&ret, &request, &context](pplx::task<web::json::value> task) {
                ret = services::Streams::create(task.get(), context);
            }).wait();

            request.reply(StatusCodes::OK, ret);
        }
        catch(std::exception& e) {
            Logger::get().error() << logstaticfn(Streams) << e.what() << std::endl;
            request.reply(StatusCodes::BadRequest, Response::error(StatusCodes::BadRequest, e.what()));
        }
    }

    static void del(Request const& request, APIContext& context)
    {
        try {
            web::json::value ret;

            auto path = web::uri::split_path(web::uri::decode(request.relative_uri().path()));

            if (!path.empty()) {
                services::Streams::del(UTF8(path[0]), context);
                request.reply(StatusCodes::OK);
            }
            else {
                request.reply(StatusCodes::BadRequest, Response::error(StatusCodes::BadRequest, "Bad request"));
            }
        }
        catch(std::exception& e) {
            Logger::get().error() << logstaticfn(Streams) << e.what() << std::endl;
            request.reply(StatusCodes::BadRequest, Response::error(StatusCodes::BadRequest, e.what()));
        }
    }

private:

    services::Streams service_;
};

}
}
}