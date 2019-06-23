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

#include <iostream>
#include <sstream>

#include <ipcaster/base/Exception.hpp>

namespace ipcaster
{

/**
 * Singleton that provides logging functionality.
 * Four levels of logging are supported (ERROR, WARNING, INFO, DEBUG)
 * 
 * @todo Implement file async logging based on boostlog
 * @todo Study implement console output coloring
 */
class Logger
{
public:

    /** Available logging levels 
     * FATAL = Application crash or cannot continue.
     * ERROR = Error that breaks (totally or partially) some task execution.
     * WARNING = Minor problem, doesn't avoid task execution.
     * INFO = Informative messages.
     * DEBUG0, DEBUG1 = Debug messages (several incremental levels)
    */
    enum class Level { QUIET = 0, FATAL = 1, ERROR = 2, WARNING = 3, INFO = 4, DEBUG0 = 5, DEBUG1 = 6};

    /**
     * Constructor
     * Sets default level to INFO
     */
    Logger()
        : 
        fatal_(nullptr),
        error_(nullptr),
        warning_(nullptr),
        info_(nullptr),
        debug_(nullptr),
        debug1_(nullptr),
        verbosity_(Level::INFO)
    {
        setVerbosity(verbosity_);
    }

    /** @returns A reference to the Logger singleton */
    static Logger& get()
    {
        static Logger singleton;
        return singleton;
    }
    
    /** 
     * Sets the verbosity level, messages with level above it will not be logged
     * 
     * @note In the current version only console logging is implemented so
     * all levels are pointing to std::clog
     */
    void setVerbosity(Level verbosity)
    {
        verbosity_ = verbosity;

        fatal_.rdbuf((verbosity >= Level::FATAL) ? std::clog.rdbuf() : nullptr);
        error_.rdbuf((verbosity >= Level::ERROR) ? std::clog.rdbuf() : nullptr);
        warning_.rdbuf((verbosity >= Level::WARNING) ? std::clog.rdbuf() : nullptr);
        info_.rdbuf((verbosity >= Level::INFO) ? std::clog.rdbuf() : nullptr);
        debug_.rdbuf((verbosity >= Level::DEBUG0) ? std::clog.rdbuf() : nullptr);        
        debug1_.rdbuf((verbosity >= Level::DEBUG1) ? std::clog.rdbuf() : nullptr);        
    }

    /** 
     * Sets the verbosity level, messages with level above it will not be logged
     * 
     * @param verbosity Verbosity level as an int value
     * 
     * @note In the current version only console logging is implemented so
     * all levels are pointing to std::clog
     */
    void setVerbosity(int verbosity)
    {
        Level level;

        switch(verbosity) {
            case 0:
                level = Level::QUIET;
                break;
            case 1:
                level = Level::FATAL;
                break;
            case 2:
                level = Level::ERROR;
                break;
            case 3:
                level = Level::WARNING;
                break;
            case 4:
                level = Level::INFO;
                break;
            case 5:
                level = Level::DEBUG0;
                break;
            case 6:
                level = Level::DEBUG1;
                break;
        }

        setVerbosity(level);
    }

    /** @returns The current verbosity level */
    inline Level getVerbosity() { return verbosity_; }

    /** @returns An stream where FATAL level messages can be written */
    inline std::ostream& fatal() { return error_; }

    /** @returns An stream where ERROR level messages can be written */
    inline std::ostream& error() { return error_; }
    
    /** @returns An stream where WARNING level messages can be written */
    inline std::ostream& warning() { return warning_; }

    /** @returns An stream where INFO level messages can be written */
    inline std::ostream& info() { return info_; }

    /** @returns An stream where DEBUG level messages can be written */
    inline std::ostream& debug(int debug_level = 0) 
    { 
        switch(debug_level)
        {
            case 0:
                return debug_;
            case 1:
                return debug1_;
            default:
                throw Exception("Logger::debug() - invalid debug_level");

        }

        return debug_; 
    }

    /** 
     * Convert a memory address to string
     * 
     * @param addr Memory address to convert
     * 
     * @returns Address, in format 0xhex, converted to string
     */
    static std::string addrStr(void* addr) 
    {
        std::ostringstream ss;
        ss << addr;
        return ss.str();
    }

private: 

    std::ostream fatal_;
    std::ostream error_;
    std::ostream warning_;
    std::ostream info_;
    std::ostream debug_;
    std::ostream debug1_;

    // Verbosity level
    Level verbosity_; 
};

#define logfn(class_name) "[" << this << "]" << " " << #class_name << "::" << __func__ << "() - "
#define logclass(class_name) "[" << this << "]" << " " << #class_name << " - "
#define fndbg(class_name) "[" + Logger::addrStr(this) + "]" + " " + #class_name + "::" + __func__ + "() - "
#define fnstdbg(class_name) std::string(#class_name) + "::" + __func__ + "() - "

}
