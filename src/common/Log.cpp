/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2013-2016, Daemon Developers
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

#include "Common.h"

namespace Log {

    Logger::Logger(Str::StringRef name, std::string prefix, Level defaultLevel)
        : filterLevel(new Cvar::Cvar<Log::Level>(
              "logs.logLevel." + name, "Log::Level - logs from '" + name + "' below the level specified are filtered", 0, defaultLevel)),
          prefix(prefix), enableSuppression(true) {
    }

    std::string Logger::Prefix(std::string message) const {
        if (prefix.empty()) {
            return message;
        } else {
            return prefix + " " + message;
        }
    }

    void Logger::Dispatch(std::string message, Log::Level level, Str::StringRef format) {
        if (enableSuppression) {
            Log::DispatchWithSuppression(std::move(message), level, format);
        } else {
            Log::DispatchByLevel(std::move(message), level);
        }
    }

    Logger Logger::WithoutSuppression() {
        Logger unsuppressed = *this;
        unsuppressed.enableSuppression = false;
        return unsuppressed;
    }

    Logger defaultLogger(VM_STRING_PREFIX "default", "", Level::NOTICE);

    bool ParseCvarValue(std::string value, Log::Level& result) {
        if (value == "warning" or value == "warn") {
            result = Log::Level::WARNING;
            return true;
        }

        if (value == "info" or value == "notice") {
            result = Log::Level::NOTICE;
            return true;
        }

        if (value == "verbose") {
            result = Log::Level::VERBOSE;
            return true;
        }

        if (value == "debug" or value == "all") {
            result = Log::Level::DEBUG;
            return true;
        }

        return false;
    }

    std::string SerializeCvarValue(Log::Level value) {
        switch(value) {
            case Log::Level::WARNING:
                return "warning";
            case Log::Level::NOTICE:
                return "notice";
            case Log::Level::VERBOSE:
                return "verbose";
            case Log::Level::DEBUG:
                return "debug";
            default:
                return "";
        }
    }

    static const int debugTargets = (1 << GRAPHICAL_CONSOLE) | (1 << TTY_CONSOLE) | (1 << LOGFILE);
    static const int verboseTargets = (1 << GRAPHICAL_CONSOLE) | (1 << TTY_CONSOLE) | (1 << LOGFILE);
    static const int noticeTargets = (1 << GRAPHICAL_CONSOLE) | (1 << TTY_CONSOLE) | (1 << LOGFILE);
    static const int warnTargets = (1 << GRAPHICAL_CONSOLE) | (1 << TTY_CONSOLE) | (1 << LOGFILE);

    //TODO add the time (broken for now because it is journaled) use Sys_Milliseconds instead (Utils::Milliseconds ?)
    void DispatchByLevel(std::string message, Log::Level level) {
        switch (level) {
        case Level::DEBUG:
            Log::Dispatch({"^5Debug: " + message}, debugTargets);
            break;
        case Level::VERBOSE:
            Log::Dispatch({std::move(message)}, verboseTargets);
            break;
        case Level::NOTICE:
            Log::Dispatch({std::move(message)}, noticeTargets);
            break;
        case Level::WARNING:
            Log::Dispatch({"^3Warn: " + message}, warnTargets);
        }
    }

    template <typename T>
    static T GetCvarOrDie(Str::StringRef cvar) {
        T value;
        std::string valueString = Cvar::GetValue(cvar);
        if (!Cvar::ParseCvarValue(valueString, value)) {
            Sys::Error("Failed to deserialize cvar %s with value: %s", cvar, valueString);
        }
        return value;
    }
    namespace {
        // Log-spam suppression: if more than MAX_OCCURRENCES log messages with the same format string
        // are sent in less than INTERVAL_MS milliseconds, they will stop being printed.
        // (Unless the spammy message desists long enough to be flushed out of the buffer: then it can
        // be printed again.)
        class LogSpamSuppressor {
            struct MessageStatistics {
                std::string messageFormat;
                int numOccurrences;
                int intervalStartTime = -2000000;
            };
            std::vector<MessageStatistics> buf;
            std::mutex mutex;
        public:
            enum Result {
                OK, // not log spam
                LAST_CHANCE, // any more messages after this will be considered log spam
                KNOWN_SPAM
            };
            Result UpdateAndEvaluate(Str::StringRef messageFormat) {
                std::lock_guard<std::mutex> lock(mutex);
                int intervalMs = GetCvarOrDie<int>("logs.suppression.interval");
                int maxOccurrences = GetCvarOrDie<int>("logs.suppression.count");
                int bufferSize = GetCvarOrDie<int>("logs.suppression.bufferSize");
                buf.resize(bufferSize);
                MessageStatistics* oldest = &buf[0];
                int now = Sys::Milliseconds();
                // Search for an existing entry. An entry is considered expired
                // if it is both older than INTERVAL_MS and has less than MAX_OCCURRENCES.
                for (MessageStatistics& stats : buf) {
                    if ((stats.numOccurrences >= maxOccurrences || stats.intervalStartTime > now - intervalMs)
                        && stats.messageFormat == messageFormat) {
                        ++stats.numOccurrences;
                        if (stats.numOccurrences < maxOccurrences) {
                            return OK;
                        }
                        stats.intervalStartTime = now;
                        return stats.numOccurrences == maxOccurrences ? LAST_CHANCE : KNOWN_SPAM;
                    }
                    if (stats.intervalStartTime < oldest->intervalStartTime) {
                        oldest = &stats;
                    }
                }
                // Replace oldest entry if there is not already one for this message
                oldest->intervalStartTime = now;
                oldest->messageFormat = messageFormat;
                oldest->numOccurrences = 1;
                return OK;
            }
        };
    } // namespace

    void DispatchWithSuppression(std::string message, Log::Level level, Str::StringRef format) {
        static LogSpamSuppressor suppressor;
        if (!GetCvarOrDie<bool>("logs.suppression.enabled")) {
            DispatchByLevel(std::move(message), level);
            return;
        }
        switch (suppressor.UpdateAndEvaluate(format)) {
        case LogSpamSuppressor::LAST_CHANCE:
            message += " [further messages like this will be suppressed]";
            DAEMON_FALLTHROUGH;
        case LogSpamSuppressor::OK:
            DispatchByLevel(std::move(message), level);
            break;
        case LogSpamSuppressor::KNOWN_SPAM:
            break;
        }
    }

    void CommandInteractionMessage(std::string message) {
        DispatchByLevel(std::move(message), Log::Level::NOTICE);
    }
} // namespace Log

namespace Cvar {
    template<>
    std::string GetCvarTypeName<Log::Level>() {
        return "log level";
    }
}
