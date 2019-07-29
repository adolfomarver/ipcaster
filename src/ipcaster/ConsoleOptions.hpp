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

#include <boost/program_options.hpp>

#include "ipcaster/base/Logger.hpp"
#include "ipcaster/IPCaster.h"
#include "ipcaster/api/HTTP.hpp"

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
            ("command", boost::program_options::value<std::string>(), "command to execute {service | play}")
            ("args", boost::program_options::value<std::vector<std::string> >(), "Arguments for command")

            ("help,h", "shows this help message")

            ("license,l", "shows the license")

            ("verbose,v", boost::program_options::value<int>()->implicit_value(4),
                  "select verbosity level (0 = QUIET, 1 = FATAL, 2 = ERROR, 3 = WARNING, 4 = INFO 5 = DEBUG0 6 = DEBUG1)")
        ;

        boost::program_options::positional_options_description p;
        p.add("command", 1).
        add("args", -1);

        boost::program_options::variables_map vm;
        auto parsed = boost::program_options::command_line_parser(argc, argv).
                  options(desc).positional(p).allow_unregistered().run();
        boost::program_options::store(parsed, vm);

        if (vm.count("help") || argc == 1) {
            std::cout << "Usage:" << std::endl << std::endl << "ipcaster [-v] [-l] [-h] [service {service_args} | play {play_args}}" << std::endl << std::endl;
            std::cout << desc << std::endl;
            std::cout << "   {service_args} [-p]" << std::endl;
            std::cout << "   [-p, --port]]\t      http listening port" << std::endl << std::endl;
            std::cout << "   {play_args} [{file} {target_ip} {target_port}] ..." << std::endl << std::endl;
            std::cout << "Examples:" << std::endl << std::endl;
            std::cout << "ipcaster service" << std::endl;
            std::cout << "ipcaster service -p 8080" << std::endl;
            std::cout << "ipcaster play file1.ts 127.0.0.1 50000" << std::endl;
            std::cout << "ipcaster play file1.ts 127.0.0.1 50000 file2.ts 127.0.0.1 50001" << std::endl;
            std::cout << "ipcaster -v 5 service" << std::endl;
            exit(0);
        }

        if (vm.count("license")) {
            printLicense();
            exit(0);
        }

        if(vm["command"].as<std::string>() == "service") {
            boost::program_options::options_description service_desc("service options");
            service_desc.add_options()
                ("port,p", boost::program_options::value<uint16_t>()->implicit_value(8080), "Listening port");

            // Collect all the unrecognized options from the first pass. This will include the
            // (positional) command name, so we need to erase that.
            std::vector<std::string> opts = boost::program_options::collect_unrecognized(parsed.options, boost::program_options::include_positional);
            opts.erase(opts.begin());
            
            // Reparse
            boost::program_options::store(boost::program_options::command_line_parser(opts).options(service_desc).run(), vm);

            uint16_t port = 8080;
            if(vm.count("port"))
                port = vm["port"].as<uint16_t>();

            ip_caster_.setServiceMode(true, port);
        }
        else if(vm["command"].as<std::string>() == "play") {
            // Collect all the unrecognized options from the first pass. This will include the
            // (positional) command name, so we need to erase that.
            std::vector<std::string> opts = boost::program_options::collect_unrecognized(parsed.options, boost::program_options::include_positional);
            opts.erase(opts.begin());
            auto streams = parsePlay(opts);
            setupStreams(streams);
        }

        if (vm.count("verbose")) {
            auto verbosity = vm ["verbose"].as<int>();
            if(verbosity < static_cast<int>(Logger::Level::QUIET) || verbosity > static_cast<int>(Logger::Level::DEBUG1)) {
                std::cout << "Invalid verbose level" << std::endl;
                exit(0);
            }

            Logger::get().setVerbosity(verbosity);
        }
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
     * Parses the parameters, translate them to json and applies the configuration to the IPCaster object
     * 
     * @param streams Strings vector reference where every element is an space separated command line argument 
     */
    std::vector<web::json::value> parsePlay(const std::vector<std::string>& streams)
    {
        std::vector<web::json::value> json_streams;

        // 3 Elements form an stream {ts file} {target ip} {target port}
        for(int i = 0;i < streams.size(); i+=3) {
            if(i+3 <= streams.size()) {
                web::json::value json_stream;

                json_stream[U("source")] = web::json::value(UTF16(checkPath(streams[i])));
                web::json::value endpoint;

                endpoint[U("ip")] = web::json::value(UTF16(checkIP(streams[i+1])));
                endpoint[U("port")] = web::json::value(checkPort(streams[i+2]));

                json_stream[U("endpoint")] = endpoint;

                json_streams.push_back(json_stream);
            }
            else {
                std::cerr << "incomplete stream declaration: " << streams[i] << std::endl;
            }
        }

        return json_streams;
    }

    /**
     * Creates the streams in the IPCaster object
     */
    void setupStreams(std::vector<web::json::value>& streams)
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