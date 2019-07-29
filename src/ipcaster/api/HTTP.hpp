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

#include <cpprest/http_listener.h>
#include <cpprest/details/http_helpers.h>

#ifdef _MSC_VER
#define UTF16(utf8string) utility::conversions::usascii_to_utf16(utf8string)
#define UTF8(utf16string) utility::conversions::utf16_to_utf8(utf16string)
#else
#define UTF16(utf8string) (utf8string)
#define UTF8(utf16string) (utf16string)
#endif

namespace ipcaster
{

using Listener = web::http::experimental::listener::http_listener;
using Request = web::http::http_request;
using Methods = web::http::methods;
using StatusCodes = web::http::status_codes;

namespace api
{

class Response
{
public:
    static web::json::value error(web::http::status_code status_code, const std::string& message)
    {
        std::string error = "{"
            "\"error\":"
                "{"
                "   \"code\": " + std::to_string(status_code) + "," +
                "   \"message\": \"" + message + "\"" +
                "}"
            "}";

        return web::json::value::parse(UTF16(error));
    }
};

}
}