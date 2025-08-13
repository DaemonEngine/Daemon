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

#include "common/Common.h"
#include "framework/ApplicationInternals.h"
#include "framework/CommandSystem.h"
#include "client.h"

#if defined(_WIN32) || defined(BUILD_GRAPHICAL_CLIENT)
#include <SDL3/SDL.h>
#ifdef BUILD_GRAPHICAL_CLIENT
extern SDL_Window *window;
#else
#define window nullptr
#endif
#endif

static Cvar::Cvar<bool> client_errorPopup("client.errorPopup", "Enable the error popup window", Cvar::NONE, true);

namespace Application {

class ClientApplication : public Application {
    public:
        ClientApplication() {
            traits.supportsUri = true;
            #if defined(_WIN32) && defined(BUILD_TTY_CLIENT)
                // The windows dedicated server and tty client must enable the
                // curses interface because they have no other usable interface.
                traits.useCurses = true;
            #endif
            #ifdef BUILD_GRAPHICAL_CLIENT
                traits.isClient = true;
            #endif
            #ifdef BUILD_TTY_CLIENT
                traits.isTTYClient = true;
            #endif
        }

        void LoadInitialConfig(bool resetConfig) override {
            //TODO(kangz) move this functions and its friends to FileSystem.cpp
	        FS_LoadBasePak();

            CL_InitKeyCommands(); // for binds

	        Cmd::BufferCommandText("preset default.cfg");
	        if (!resetConfig) {
                Cmd::BufferCommandText("exec -f " CONFIG_NAME);
                if (traits.isClient) {
                    Cmd::BufferCommandText("exec -f " KEYBINDINGS_NAME);
                }
                Cmd::BufferCommandText("exec -f " AUTOEXEC_NAME);
	        }
        }

        void Initialize() override {
#if defined(__linux__) && defined(BUILD_GRAPHICAL_CLIENT)
            // identify the game by its name in certain
            // volume control / power control applets,
            // for example, the one found on KDE:
            // "Unvanquished is currently blocking sleep."
            // instead of "My SDL application ..."
            // this feature was introduced in SDL 2.0.22
            SDL_SetHint("SDL_APP_NAME", PRODUCT_NAME);
            // SDL_hints.h: #define SDL_HINT_APP_NAME "SDL_APP_NAME"
            // don't use the macro here, in case
            // SDL doesn't use current headers.
#endif

            Hunk_Init();

            Com_Init();

            CL_StartHunkUsers();
        }

        void Frame() override {
            Com_Frame();
            ::Application::Application::Frame(); // call base class
        }

        void OnDrop(bool error, Str::StringRef reason) override {
            FS::PakPath::ClearPaks();
            FS_LoadBasePak();
            SV_Shutdown(Str::Format("Server %s: %s", error ? "crashed" : "shutdown", reason).c_str());
            CL_Disconnect(true);
            CL_ShutdownAll();
            if (error)
            {
                Cvar::SetValue("com_errorMessage", Str::Format("^3%s", reason));
            }
            CL_StartHunkUsers();
        }

        void Shutdown(bool error, Str::StringRef message) override {
            #if defined(_WIN32) || defined(BUILD_GRAPHICAL_CLIENT)
                if (error && client_errorPopup.Get()) {
                    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, PRODUCT_NAME, message.c_str(), window);
                }
            #endif

            TRY_SHUTDOWN(CL_Shutdown());
            std::string serverMessage = error ? "Server fatal error: " + message : std::string(message);
            if (Sys::PedanticShutdown()) {
                TRY_SHUTDOWN(SV_Shutdown(message.c_str()));
                TRY_SHUTDOWN(NET_Shutdown());
                Hunk_Shutdown();
            } else {
                TRY_SHUTDOWN(SV_QuickShutdown(message.c_str()));
            }

            #if defined(_WIN32) || defined(BUILD_GRAPHICAL_CLIENT)
                // Always run SDL_Quit, because it restores system resolution and gamma.
                SDL_Quit();
            #endif
        }

        void OnUnhandledCommand(const Cmd::Args& args) override {
            CL_ForwardCommandToServer(args.EscapedArgs(0).c_str());
        }
};

INSTANTIATE_APPLICATION(ClientApplication)

}
