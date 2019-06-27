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

#include <string>
#include <vector>
#include <json.h>

#include <boost/program_options.hpp>

#include "ipcaster/base/Logger.hpp"
#include "ipcaster/IPCaster.hpp"

namespace ipcaster
{

/**
 * Parses the application console parameters and applies the setup to the ipcaster main object.
 */
class ConsoleOptions
{
public:

    /** Constructor
     * 
     * @param ip_caster Reference to the ipcaster main object
     */
    ConsoleOptions(IPCaster& ip_caster)
    : ip_caster_(ip_caster)
    {
    }

    /**
     * Parses the parameters and applies the configuration to the IPCaster object
     * 
     * @param argc The argc param from main()
     * 
     * @param argv The argv param from main()
     */
    void parse(int argc, const char* argv[])
    {
        boost::program_options::options_description desc("Allowed options");
        desc.add_options()
            ("help,h", "shows this help message")

            ("license,l", "shows the license")

            ("verbose,v", boost::program_options::value<int>()->implicit_value(4),
                  "select verbosity level (0 = QUIET, 1 = FATAL, 2 = ERROR, 3 = WARNING, 4 = INFO 5 = DEBUG0 6 = DEBUG1)")

            ("stream,s", boost::program_options::value<std::vector<std::string>>(),
                  "stream")
        ;

        boost::program_options::positional_options_description p;
        p.add("stream", -1);

        boost::program_options::variables_map vm;
        boost::program_options::store(boost::program_options::command_line_parser(argc, argv).
                  options(desc).positional(p).run(), vm);
        boost::program_options::notify(vm);

        if (vm.count("help") || argc == 1) {
            std::cout << "Usage: ipcaster [-s {ts file} {target ip} {target port} ] ... [-s ...]\n";
            std::cout << desc << std::endl;
            std::cout << "Example: ipcaster -s stream1.ts 127.0.0.1 50000 -s stream2.ts 127.0.0.1 50001" << std::endl;
            exit(0);
        }

        if (vm.count("license")) {
            printLicense();
            exit(0);
        }

        if (vm.count("verbose")) {
            auto verbosity = vm ["verbose"].as<int>();
            if(verbosity < static_cast<int>(Logger::Level::QUIET) || verbosity > static_cast<int>(Logger::Level::DEBUG1)) {
                std::cout << "Invalid verbose level" << std::endl;
                exit(0);
            }

            Logger::get().setVerbosity(verbosity);
        }

        if (vm.count("stream"))
        {
            auto streams = parseStreams(vm["stream"].as<std::vector<std::string>>());
            setupStreams(streams);
        }
    }

    /**
     * Parses the parameters, translate them to json and applies the configuration to the IPCaster object
     * 
     * @param streams Strings vector reference where every element is an space separated command line argument 
     */
    std::vector<Json::Value> parseStreams(const std::vector<std::string>& streams)
    {
        std::vector<Json::Value> json_streams;

        // 3 Elements form an stream {ts file} {target ip} {target port}
        for(int i = 0;i < streams.size(); i+=3) {
            if(i+3 <= streams.size()) {
                Json::Value json_stream;

                json_stream["source"] = checkPath(streams[i]);
                Json::Value endpoint;

                endpoint["ip"] = checkIP(streams[i+1]);
                endpoint["port"] = checkPort(streams[i+2]);

                json_stream["endpoint"] = endpoint;

                json_streams.push_back(json_stream);
            }
            else {
                std::cerr << "incomplete stream declaration: " << streams[i] << std::endl;
            }
        }

        return json_streams;
    }

private:

    // Main IPCaster object 
    IPCaster& ip_caster_;

    /**
     * Validates is a valid path
     * @todo Check if is a valid path 
     */
    std::string checkPath(const std::string& path) { return path; }

    /**
     * Validates is a valid IPv4 address
     * @todo Check if is a valid IPv4 address
     */
    std::string checkIP(const std::string& ip_addr) { return ip_addr; }

    /**
     * Validates is a valid IP port
     * @todo Check if is a valid IP port
     */
    uint16_t checkPort(const std::string& port) { return static_cast<uint16_t>(atoi(port.c_str())); }

    /**
     * Creates the streams in the IPCaster object
     */
    void setupStreams(std::vector<Json::Value>& streams)
    {
        for(auto& stream : streams) {
            try {
                ip_caster_.createStream(stream);
            }
            catch(std::exception& e) {
                Logger::get().error() << e.what() << std::endl;
            }
        }
    }

    /** Prints the program license */
    void printLicense()
    {
        std::cout << "-----------------" << std::endl;
        std::cout << "IPCaster license: " << std::endl;
        std::cout << "-----------------" << std::endl;
        std::cout << std::endl;
        std::cout << "Copyright (C) 2019 Adofo Martinez <adolfo at ipcaster dot net>" << std::endl;
        std::cout << "" << std::endl;
        std::cout << "Licensed under the Apache License, Version 2.0 (the \"License\");" << std::endl;
        std::cout << "you may not use this file except in compliance with the License." << std::endl;
        std::cout << "You may obtain a copy of the License at" << std::endl;
        std::cout << "" << std::endl;
        std::cout << "     http://www.apache.org/licenses/LICENSE-2.0" << std::endl;
        std::cout << "" << std::endl;
        std::cout << "Unless required by applicable law or agreed to in writing, software" << std::endl;
        std::cout << "distributed under the License is distributed on an \"AS IS\" BASIS," << std::endl;
        std::cout << "WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied." << std::endl;
        std::cout << "See the License for the specific language governing permissions and" << std::endl;
        std::cout << "limitations under the License." << std::endl;
        std::cout << std::endl;
        std::cout << "--------------------------------------------------------------------" << std::endl;
        std::cout << std::endl;

        print3rdPartyLicenses();
    }

    void print3rdPartyLicenses()
    {
        std::cout << "IPCaster third party licenses:" << std::endl;
        std::cout << std::endl;

        printJsonCppLicense();
    }
    
    void printJsonCppLicense()
    {
        std::cout << "--------------------------------" << std::endl;
        std::cout << "JsonCpp library" << std::endl;
        std::cout << "--------------------------------" << std::endl;
        std::cout << std::endl;
        std::cout << "Copyright (c) 2007-2010 Baptiste Lepilleur and The JsonCpp Authors" << std::endl;
        std::cout << "Released under the terms of the MIT License (see below)." << std::endl;
        std::cout << std::endl;
        std::cout << "     http://en.wikipedia.org/wiki/MIT_License" << std::endl;
        std::cout << std::endl;
        std::cout << "--------------------------------------------------------------------" << std::endl;
        std::cout << std::endl;
    }

    

};

}