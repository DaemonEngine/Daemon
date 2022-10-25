/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2013-2022, Daemon Developers
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

#ifndef FRAMEWORK_APPLICATION_INTERNALS_H_
#define FRAMEWORK_APPLICATION_INTERNALS_H_

#ifdef PRODUCE_TEST_APPLICATION
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "common/Cvar.h"
#include "System.h"
#endif

#include "Application.h"

namespace Application {

#ifdef PRODUCE_TEST_APPLICATION

template<typename ApplicationT>
class TestApplication : public ApplicationT
{
    static Cvar::Cvar<std::string> gtestFlags;

    void Frame() override
    {
        Log::Notice("Running unit tests. You can set Googletest flags via the cvar `testing.flags`."
                    " Try `-set testing.flags --help`.");
        std::string flagBuf("programname " + gtestFlags.Get() + " ");
        std::vector<char*> argv;
        for (size_t start = 0; start != flagBuf.npos; ) {
            size_t end = flagBuf.find_first_of(" ", start);
            flagBuf[end] = '\0';
            argv.push_back(&flagBuf[start]);
            start = flagBuf.find_first_not_of(" ", end + 1);
        }
        int argc = int(argv.size());
        testing::InitGoogleMock(&argc, argv.data());
        if (argc > 1) {
            Sys::Error("Unknown Googletest flag: '%s'", argv[1]);
        }
        bool success = 0 == RUN_ALL_TESTS();
        if (success) {
            Sys::Quit("Passed all tests");
        }
        else {
            Sys::Error("One or more tests failed");
        }
    }
};

#define INSTANTIATE_APPLICATION(classname) \
    template<> Cvar::Cvar<std::string> TestApplication<classname>::gtestFlags( \
        "testing.flags", "Space-separated flags for Googletest", Cvar::NONE, ""); \
    Application& GetApp() { \
        static TestApplication<classname> app; \
        return app; \
    }

#else // !PRODUCE_TEST_APPLICATION

#define INSTANTIATE_APPLICATION(classname) \
    Application& GetApp() { \
        static classname app; \
        return app; \
    }

#endif // PRODUCE_TEST_APPLICATION

#define TRY_SHUTDOWN(code) try {code;} catch (Sys::DropErr&) {}

}

#endif // FRAMEWORK_APPLICATION_INTERNALS_H_
