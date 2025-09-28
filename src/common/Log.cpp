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

#ifdef BUILD_ENGINE
// TODO: when cvar multiple registration is available, just declare the cvars in each module.
static Cvar::Cvar<bool> suppressionEnabled(
    "logs.suppression.enabled", "Whether to suppress log messages that are printed too many times", Cvar::NONE, true);
static Cvar::Range<Cvar::Cvar<int>> suppressionInterval(
    "logs.suppression.interval", "Interval in milliseconds for detecting log spam", Cvar::NONE, 2000, 1, 1000000);
static Cvar::Range<Cvar::Cvar<int>> suppressionCount(
    "logs.suppression.count", "Number of occurrences for a message to be considered log spam", Cvar::NONE, 10, 1, 1000000);
static Cvar::Range<Cvar::Cvar<int>> suppressionBufSize(
    "logs.suppression.bufferSize", "How many distinct messages to track for log suppression", Cvar::NONE, 50, 1, 1000000);

#define GET_LOG_CVAR(type, name, object) (object.Get())
#else
#define GET_LOG_CVAR(type, name, object) (GetCvarOrDie<type>(name))
template <typename T>
static T GetCvarOrDie(Str::StringRef cvar) {
    T value;
    std::string valueString = Cvar::GetValue(cvar);
    if (!Cvar::ParseCvarValue(valueString, value)) {
        Sys::Error("Failed to deserialize cvar %s with value: %s", cvar, valueString);
    }
   return value;
}
#endif

namespace Log {
    Cvar::Cvar<bool> logExtendAll(
        "logs.writeSrcLocation.all", "Always print source code location for logs", Cvar::NONE, false );
    Cvar::Cvar<bool> logExtendWarn(
        "logs.writeSrcLocation.warn", "Print source code location for Warn logs", Cvar::NONE, false );
    Cvar::Cvar<bool> logExtendNotice(
        "logs.writeSrcLocation.notice", "Print source code location for Notice logs", Cvar::NONE, false );
    Cvar::Cvar<bool> logExtendVerbose(
        "logs.writeSrcLocation.verbose", "Print source code location for Verbose logs", Cvar::NONE, false );
    Cvar::Cvar<bool> logExtendDebug(
        "logs.writeSrcLocation.debug", "Print source code location for Debug logs", Cvar::NONE, false );

    Logger::Logger(Str::StringRef name, std::string prefix, Level defaultLevel)
        : filterLevel(new Cvar::Cvar<Log::Level>(
              "logs.level." + name, "Log::Level - logs from '" + name + "' below the level specified are filtered", 0, defaultLevel)),
          enableSuppression(true)
    {
        if (!prefix.empty()) {
            // TODO allow prefixes without a space, e.g. a color code
            this->prefix = prefix + " ";
        }
    }

    std::string Logger::Prefix(Str::StringRef message) const {
        return prefix + message;
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

    //TODO add the time (broken for now because it is journaled) use Sys::Milliseconds instead
    void DispatchByLevel(std::string message, Log::Level level) {
        switch (level) {
        case Level::DEBUG:
            message.insert(0, "^5Debug: ");
            Log::Dispatch(std::move(message), debugTargets);
            break;
        case Level::VERBOSE:
            Log::Dispatch(std::move(message), verboseTargets);
            break;
        case Level::NOTICE:
            Log::Dispatch(std::move(message), noticeTargets);
            break;
        case Level::WARNING:
            message.insert(0, "^3Warn: ");
            Log::Dispatch(std::move(message), warnTargets);
            break;
        }
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
                int intervalMs = GET_LOG_CVAR(int, "logs.suppression.interval", suppressionInterval);
                int maxOccurrences = GET_LOG_CVAR(int, "logs.suppression.count", suppressionCount);
                int bufferSize = GET_LOG_CVAR(int, "logs.suppression.bufferSize", suppressionBufSize);
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
        if (level == Level::DEBUG || !GET_LOG_CVAR(bool, "logs.suppression.enabled", suppressionEnabled)) {
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
