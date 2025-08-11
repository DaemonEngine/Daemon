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

#include <common/FileSystem.h>
#include "qcommon/q_shared.h"
#include "qcommon/qcommon.h"
#include "LogSystem.h"

namespace Log {

    static Cvar::Cvar<bool> suppressionEnabled(
        "logs.suppression.enabled", "Whether to suppress log messages that are printed too many times", Cvar::NONE, true);
    static Cvar::Range<Cvar::Cvar<int>> suppressionInterval(
        "logs.suppression.interval", "Interval in milliseconds for detecting log spam", Cvar::NONE, 2000, 1, 1000000);
    static Cvar::Range<Cvar::Cvar<int>> suppressionCount(
        "logs.suppression.count", "Number of occurrences for a message to be considered log spam", Cvar::NONE, 10, 1, 1000000);
    static Cvar::Range<Cvar::Cvar<int>> suppressionBufSize(
        "logs.suppression.bufferSize", "How many distinct messages to track for log suppression", Cvar::NONE, 50, 1, 1000000);

#ifdef CPP_SOURCE_LOCATION
    static Cvar::Cvar<bool> logExtendAll(
        "logs.writeSrcLocation.all", "Always print source code location for logs", Cvar::NONE, false );
    static Cvar::Cvar<bool> logExtendWarn(
        "logs.writeSrcLocation.warn", "Print source code location for Warn logs", Cvar::NONE, false );
    static Cvar::Cvar<bool> logExtendNotice(
        "logs.writeSrcLocation.notice", "Print source code location for Notice logs", Cvar::NONE, false );
    static Cvar::Cvar<bool> logExtendVerbose(
        "logs.writeSrcLocation.verbose", "Print source code location for Verbose logs", Cvar::NONE, false );
    static Cvar::Cvar<bool> logExtendDebug(
        "logs.writeSrcLocation.debug", "Print source code location for Debug logs", Cvar::NONE, false );
#endif

    static Target* targets[MAX_TARGET_ID];


    //TODO make me reentrant // or check it is actually reentrant when using for (Event e : events) do stuff
    //TODO think way more about thread safety
    void Dispatch(Log::Event event, int targetControl) {
        static std::vector<Log::Event> buffers[MAX_TARGET_ID];
        static std::recursive_mutex bufferLocks[MAX_TARGET_ID];

        if (Sys::IsProcessTerminating()) {
            return;
        }

        for (int i = 0; i < MAX_TARGET_ID; i++) {
            if ((targetControl >> i) & 1) {
                std::lock_guard<std::recursive_mutex> guard(bufferLocks[i]);
                auto& buffer = buffers[i];

                buffer.push_back(event);

                bool processed = false;
                if (targets[i]) {
                    processed = targets[i]->Process(buffer);
                }

                if (processed || buffer.size() > 512) {
                    buffer.clear();
                }
            }
        }
    }

    void RegisterTarget(TargetId id, Target* target) {
        targets[id] = target;
    }

    Target::Target() = default;

    void Target::Register(TargetId id) {
        Log::RegisterTarget(id, this);
    }

    //Log Targets
    //TODO: move them in their respective modules
    //TODO this one isn't multithreaded at all, need a rewrite of the consoles
    class TTYTarget : public Target {
        public:
            TTYTarget() {
                this->Register(TTY_CONSOLE);
            }

            virtual bool Process(const std::vector<Log::Event>& events) override {
                for (auto& event : events)  {
                    CON_Print(event.text.c_str());
                    CON_Print("\n");
                }
                return true;
            }
    };

    static TTYTarget tty;

    //TODO add a Callback on these that will make the logFile open a new file or something?
    static Cvar::Cvar<bool> useLogFile("logs.logFile.active", "are the logs sent in the logfile", Cvar::NONE, true);
    static Cvar::Cvar<std::string> logFileName("logs.logFile.filename", "the name of the logfile", Cvar::INIT | Cvar::TEMPORARY, "daemon.log");
    static Cvar::Cvar<bool> overwrite("logs.logFile.overwrite", "if true the logfile is deleted at each run else the logs are just appended", Cvar::INIT | Cvar::TEMPORARY, true);
    static Cvar::Cvar<bool> forceFlush("logs.logFile.forceFlush", "are all the logs flushed immediately (more accurate but slower)", Cvar::INIT | Cvar::TEMPORARY, false);
    class LogFileTarget: public Target {
        public:
            LogFileTarget() {
                this->Register(LOGFILE);
            }

            virtual bool Process(const std::vector<Log::Event>& events) override {
                //If we have no log file drop the events
                if (not useLogFile.Get()) {
                    return true;
                }

                if (logFile) {
                    for (auto& event : events) {
                        logFile.Printf("%s\n", event.text);
                    }
                    return true;
                } else {
                    return false;
                }
            }

            FS::File logFile;
    };

    static LogFileTarget logfile;

    void OpenLogFile() {
        //If we have no log file do nothing here
        if (not useLogFile.Get()) {
            return;
        }

        try {
            if (overwrite.Get()) {
                logfile.logFile = FS::HomePath::OpenWrite(logFileName.Get());
            } else {
                logfile.logFile = FS::HomePath::OpenAppend(logFileName.Get());
            }

            if (forceFlush.Get()) {
                logfile.logFile.SetLineBuffered(true);
            }
        } catch (std::system_error& err) {
            Sys::Error("Could not open log file %s: %s", logFileName.Get(), err.what());
        }
    }

    void FlushLogFile() {
        std::error_code err;
        logfile.logFile.Flush(err);
        if (err) {
            Log::Warn("Error flushing log file");
        }
    }
}
