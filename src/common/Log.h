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

#ifndef COMMON_LOG_H_
#define COMMON_LOG_H_

#include "CPPStandard.h"

#include "engine/qcommon/q_shared.h"

#if defined( CPP_SOURCE_LOCATION )
    #include <source_location>
#endif

namespace Log {

    /*
     * There are 4 log levels that code can use:
     *   - WARNING when something is not going as expected, it is very visible.
     *   - NOTICE when we want to say something interesting, not very visible.
     *   - VERBOSE when we want to give status update, not visible by default.
     *   - DEBUG when we want to give deep info useful only to developers.
     */

    enum class Level {
        DEBUG,
        VERBOSE,
        NOTICE,
        WARNING,
    };

    // The default filtering level
    const Level DEFAULT_FILTER_LEVEL = Level::WARNING;

    struct FormatStringT {
       Str::StringRef format;

#ifdef CPP_SOURCE_LOCATION
       std::source_location loc;

       template<typename StringT>
       FormatStringT(StringT format, std::source_location loc = std::source_location::current())
          : format(format), loc(loc) {}
#else
       template<typename StringT>
       FormatStringT(StringT format) : format(format) {}
#endif
    };

    /*
     * Loggers are used to group logs by subsystems and allow logs
     * to be filtered by log level by subsystem. They are used like so
     * in a submodule "Foo" in a module "bar"
     *
     *   static Logger fooLog("bar.foo", "[foo]"); // filters with the default filtering level
     *   // and adds [foo] before all messages
     *
     *   fooLog.Warn("%s %i", string, int); // "appends" the newline automatically
     *   fooLog.Debug(<expensive formatting>); // if the log is filtered, no formatting occurs
     *
     *   // However functions calls will still be performed.
     *   // To run code depending on the logger state use the following:
     *   fooLog.DoNoticeCode([&](){
     *       ExpensiveCall();
     *       fooLog.Notice("Printing the expensive expression %s", <the expression>);
     *   });
     *
     * In addition the user/developer can control the filtering level with
     *   /set logs.level.foo.bar {warning, info, verbose, debug}
     *
     * To intentionally print a message repeatedly at high volume without getting it blocked as
     * log spam, do like thus:
     * logger.WithoutSuppression().Notice("my message that prints every frame");
     * Alternatively, use the DEBUG level, for which log suppression is disabled since
     * Debug() messages are often intentionally verbose.
     */

    class Logger {
        public:
            Logger(Str::StringRef name, std::string prefix = "", Level defaultLevel = DEFAULT_FILTER_LEVEL);

            template<typename ... Args>
            void Warn( FormatStringT format, Args&& ... args );

            template<typename ... Args>
            void Notice( FormatStringT format, Args&& ... args );

            template<typename ... Args>
            void Verbose( FormatStringT format, Args&& ... args );

            template<typename ... Args>
            void Debug( FormatStringT format, Args&& ... args );

            template<typename F>
            void DoWarnCode(F&& code);

            template<typename F>
            void DoNoticeCode(F&& code);

            template<typename F>
            void DoVerboseCode(F&& code);

            template<typename F>
            void DoDebugCode(F&& code);

            Logger WithoutSuppression();

        private:
            void Dispatch(std::string message, Log::Level level, FormatStringT format);

            std::string Prefix(std::string message) const;

            // the cvar logs.level.<name>
            std::shared_ptr<Cvar::Cvar<Level>> filterLevel;

            // a prefix appended to all the messages of this logger
            std::string prefix;

            bool enableSuppression;
    };

    /*
     * When debugging a function or before a logger is introduced for
     * a module the following functions can be used for less typing.
     * However it shouldn't stay in production code because it
     * cannot be filtered and will clutter the console.
     */

    template<typename ... Args>
    void Warn( FormatStringT format, Args&& ... args );

    template<typename ... Args>
    void Notice( FormatStringT format, Args&& ... args );

    template<typename ... Args>
    void Verbose( FormatStringT format, Args&& ... args );

    template<typename ... Args>
    void Debug( FormatStringT format, Args&& ... args );

    /*
     * For messages which are not true log messages, but rather are produced by
     * direct interaction of the user with the console command system.
     * This should not be used outside of the command system implementation.
     */
    void CommandInteractionMessage(std::string message);

    /*
     * A log Event, sent to the log system along a list of targets to output
     * it to. Event are not all generated by the loggers (e.g. kill messages)
     */

    struct Event {
        Event(std::string text)
            : text(std::move(text)) {}
        std::string text;
    };

    /*
     * The list of potential targets for a log event.
     * TODO: avoid people having to do (1 << TARGET1) | (1 << TARGET2) ...
     */

    enum TargetId {
        GRAPHICAL_CONSOLE,
        TTY_CONSOLE,
        LOGFILE,
        MAX_TARGET_ID
    };

    //Internals

    // Functions used for Cvar<Log::Level>
    bool ParseCvarValue(std::string value, Log::Level& result);
    std::string SerializeCvarValue(Log::Level value);

    // Sends the message to the appropriate targets for the specified level.
    void DispatchByLevel(std::string message, Log::Level level);

    // Forwards to DispatchByLevel if the log message is determined to be non-spammy.
    // The format string is used to classify whether it is the same message repeated excessively.
    void DispatchWithSuppression(std::string message, Log::Level level, FormatStringT format);

    // Engine calls available everywhere

    void Dispatch(Log::Event event, int targetControl);

    // Implementation of templates

    // Logger

    template<typename ... Args>
    void Logger::Warn( FormatStringT format, Args&& ... args ) {
        if ( filterLevel->Get() <= Level::WARNING ) {
            this->Dispatch( Prefix( Str::Format( format.format, std::forward<Args>( args ) ... ) ), Level::WARNING, format );
        }
    }

    template<typename ... Args>
    void Logger::Notice( FormatStringT format, Args&& ... args ) {
        if ( filterLevel->Get() <= Level::NOTICE ) {
            this->Dispatch( Prefix( Str::Format( format.format, std::forward<Args>( args ) ... ) ), Level::NOTICE, format );
        }
    }

    template<typename ... Args>
    void Logger::Verbose( FormatStringT format, Args&& ... args ) {
        if ( filterLevel->Get() <= Level::VERBOSE ) {
            this->Dispatch( Prefix( Str::Format( format.format, std::forward<Args>( args ) ... ) ), Level::VERBOSE, format );
        }
    }

    template<typename ... Args>
    void Logger::Debug( FormatStringT format, Args&& ... args ) {
        if ( filterLevel->Get() <= Level::DEBUG ) {
            this->Dispatch( Prefix( Str::Format( format.format, std::forward<Args>( args ) ... ) ), Level::DEBUG, format );
        }
    }

    template<typename F>
    inline void Logger::DoWarnCode(F&& code) {
        if (filterLevel->Get() <= Level::WARNING) {
            code();
        }
    }

    template<typename F>
    inline void Logger::DoNoticeCode(F&& code) {
        if (filterLevel->Get() <= Level::NOTICE) {
            code();
        }
    }

    template<typename F>
    inline void Logger::DoVerboseCode(F&& code) {
        if (filterLevel->Get() <= Level::VERBOSE) {
            code();
        }
    }

    template<typename F>
    inline void Logger::DoDebugCode(F&& code) {
        if (filterLevel->Get() <= Level::DEBUG) {
            code();
        }
    }

    // Quick Logs
    extern Logger defaultLogger;

    template<typename ... Args>
    void Warn( FormatStringT format, Args&& ... args ) {
        defaultLogger.Warn( format, std::forward<Args>( args ) ... );
    }

    template<typename ... Args>
    void Notice( FormatStringT format, Args&& ... args ) {
        defaultLogger.Notice( format, std::forward<Args>( args ) ... );
    }

    template<typename ... Args>
    void Verbose( FormatStringT format, Args&& ... args ) {
        defaultLogger.Verbose( format, std::forward<Args>( args ) ... );
    }

    template<typename ... Args>
    void Debug( FormatStringT format, Args&& ... args ) {
        defaultLogger.Debug( format, std::forward<Args>( args ) ... );
    }
}

namespace Cvar {
    template<>
    std::string GetCvarTypeName<Log::Level>();
}

#endif //COMMON_LOG_H_
