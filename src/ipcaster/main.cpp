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

#include <iostream>

#include "IPCaster.hpp"
#include "ConsoleOptions.hpp"

#include "base/Logger.hpp"

using namespace std;
using namespace ipcaster;

int main(int argc, const char* argv[])
{
    int ret_code = 1;
    
    try {
        // The application object
        IPCaster the_ip_caster;

        ConsoleOptions cmd_options(the_ip_caster);

        // Parse de command line args and apply the setup to the_ip_caster object
        cmd_options.parse(argc, argv);

        // Run until work is done (command line mode) or until exit command is received (server mode).
        ret_code = the_ip_caster.run();
    }
    catch(const std::exception& e) {
        Logger::get().fatal() << e.what() << '\n';
        Logger::get().fatal() << "main() - program terminated by last error" << std::endl;
    }
    
    return ret_code;
}