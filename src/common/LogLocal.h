/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2013-2017, Daemon Developers
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Daemon developers nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL DAEMON DEVELOPERS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
===========================================================================
*/

#ifndef COMMON_LOG_LOCAL_H_
#define COMMON_LOG_LOCAL_H_

#include <utility>
#include "Assert.h"
#include "LogBase.h"
#include "String.h"

namespace Log {

    namespace {
        // Set the default logger for this file. Does not take ownership.
        void SetLocalLogger(Logger* logger);

        // Dummy object for initializing a logger. Pass the arguments for Log::Logger's constructor
        // to create a default logger for the current .cpp file.
        struct LocalLoggerInit  {
            template<typename... T> LocalLoggerInit(T&&... args);
        };
    }

    /*
     * Convenient logging functions.
     * If a local logger for the current .cpp file has been registered via SetLocalLogger()
     * or FileLoggerInit, the functions below will use that local logger. Otherwise, they will
     *  use the global default logger (which cannot be filtered).
     */

    namespace {
        template<typename ... Args>
        void Warn(Str::StringRef format, Args&& ... args);

        template<typename ... Args>
        void Notice(Str::StringRef format, Args&& ... args);

        template<typename ... Args>
        void Verbose(Str::StringRef format, Args&& ... args);

        template<typename ... Args>
        void Debug(Str::StringRef format, Args&& ... args);

        template<typename F>
        void DoWarnCode(F&& code);

        template<typename F>
        void DoNoticeCode(F&& code);

        template<typename F>
        void DoVerboseCode(F&& code);

        template<typename F>
        void DoDebugCode(F&& code);
    } // namespace


    // Implementation of templates

    // The entities below this point are intentionally redefined in each translation unit.
    namespace detail {
        namespace {
            // 'static' keyword needed to avoid MSVC bug with precompiled headers.
            static Logger* localLogger;

            Logger& GetLocalLogger() {
                return localLogger ? *localLogger : defaultLogger;
            }
        }
    }
    namespace {
        template<typename... T> LocalLoggerInit::LocalLoggerInit(T&&... args) {
            // We never need to destroy the Logger object.
            SetLocalLogger(new Logger(std::forward<T>(args)...));
        }

        void SetLocalLogger(Logger* logger) {
            DAEMON_ASSERT(!detail::localLogger && "Local logger should only be initialized once.");
            detail::localLogger = logger;
        }

        template<typename ... Args>
        void Warn(Str::StringRef format, Args&& ... args) {
            detail::GetLocalLogger().Warn(format, std::forward<Args>(args) ...);
        }

        template<typename ... Args>
        void Notice(Str::StringRef format, Args&& ... args) {
            detail::GetLocalLogger().Notice(format, std::forward<Args>(args) ...);
        }

        template<typename ... Args>
        void Verbose(Str::StringRef format, Args&& ... args) {
            detail::GetLocalLogger().Verbose(format, std::forward<Args>(args) ...);
        }

        template<typename ... Args>
        void Debug(Str::StringRef format, Args&& ... args) {
            detail::GetLocalLogger().Debug(format, std::forward<Args>(args) ...);
        }

        template<typename F>
        void DoWarnCode(F&& code) {
            detail::GetLocalLogger().DoWarnCode(std::forward<F>(code));
        }

        template<typename F>
        void DoNoticeCode(F&& code) {
            detail::GetLocalLogger().DoNoticeCode(std::forward<F>(code));
        }

        template<typename F>
        void DoVerboseCode(F&& code) {
            detail::GetLocalLogger().DoVerboseCode(std::forward<F>(code));
        }

        template<typename F>
        void DoDebugCode(F&& code) {
            detail::GetLocalLogger().DoDebugCode(std::forward<F>(code));
        }
    } // namespace
} // namespace Log

#endif // COMMON_LOG_LOCAL_H_
