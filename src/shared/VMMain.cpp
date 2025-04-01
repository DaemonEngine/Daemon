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

#include "VMMain.h"
#include "CommonProxies.h"
#include "common/IPC/CommonSyscalls.h"
#ifndef _WIN32
#include <unistd.h>
#endif

IPC::Channel VM::rootChannel;

#ifdef BUILD_VM_IN_PROCESS
// Special exception type used to cleanly exit a thread for in-process VMs
// Using an anonymous namespace so the compiler knows that the exception is
// only used in the current file.
namespace {
class ExitException {};
}

namespace Sys {
extern std::thread::id mainThread;
}
#endif

// Common initialization code for both VM types
static void CommonInit(Sys::OSHandle rootSocket)
{
	VM::rootChannel = IPC::Channel(IPC::Socket::FromHandle(rootSocket));

	// Send ABI version information, also acts as a sign that the module loaded
	Util::Writer writer;
	writer.Write<uint32_t>(IPC::ABI_VERSION_DETECTION_ABI_VERSION);
	writer.Write<std::string>(IPC::SYSCALL_ABI_VERSION);
	writer.Write<bool>(IPC::DAEMON_HAS_COMPATIBILITY_BREAKING_SYSCALL_CHANGES);
	VM::rootChannel.SendMsg(writer);

	// Start the main loop
	while (true) {
		Util::Reader reader = VM::rootChannel.RecvMsg();
		uint32_t id = reader.Read<uint32_t>();
		if (id == IPC::ID_EXIT) {
			return;
		}
		VM::VMHandleSyscall(id, std::move(reader));
	}
}

static void SendErrorMsg(Str::StringRef message)
{
	// Only try sending an ErrorMsg once
	static std::atomic_flag errorEntered;
	if (!errorEntered.test_and_set()) {
		// Disable checks for sending sync messages when handling async messages.
		// At this point we don't really care since this is an error.
		VM::rootChannel.canSendSyncMsg = true;

		// Try to tell the engine about the error, but ignore errors doing so.
		try {
			VM::SendMsg<VM::ErrorMsg>(message);
		} catch (...) {}
	}
}

#ifdef __native_client__
// HACK: when we get a fatal exception in the terminate handler and call abort() to trigger
// a crash dump (as there doesn't seem to be an API for requesting a minidump directly),
// the error message is passed through this variable.
static char realErrorMessage[256];
#endif

void Sys::Error(Str::StringRef message)
{
	if (!OnMainThread()) {
		// On a non-main thread we can't rely on IPC, so we may not be able to communicate the
		// error message. So try to trigger a crash dump instead (exiting with abort() triggers
		// one but exiting with _exit() doesn't). This will give something to work with when
		// debugging. (For the main thread case a message is usually enough to diagnose the problem
		// so we don't generate a crash dump; those consume disk space after all.)
		// Also note that throwing ExitException would only work as intended on the main thread.
		std::abort();
	}

#ifdef __native_client__
	if (realErrorMessage[0]) {
		message = realErrorMessage;
	}
#endif

	SendErrorMsg(message);

#ifdef BUILD_VM_IN_PROCESS
	// Then engine will close the root socket when it wants us to exit, which
	// will trigger an error in the IPC functions. If we reached this point then
	// we try to exit the thread semi-cleanly by throwing an exception.
	throw ExitException();
#else
	// The SendMsg should never return since the engine should kill our process.
	// Just in case it doesn't, exit here.
	_exit(255);
#endif
}

#ifdef BUILD_VM_IN_PROCESS

// Entry point called in a new thread inside the existing process
extern "C" DLLEXPORT ALIGN_STACK_FOR_MINGW void vmMain(Sys::OSHandle rootSocket)
{
	Sys::mainThread = std::this_thread::get_id();
	try {
		try {
			CommonInit(rootSocket);
		} catch (ExitException&) {
			return;
		} catch (Sys::DropErr& err) {
			Sys::Error(err.what());
		} catch (std::exception& err) {
			Sys::Error("Unhandled exception (%s): %s", typeid(err).name(), err.what());
		} catch (...) {
			Sys::Error("Unhandled exception of unknown type");
		}
	} catch (...) {}
}

#else

// The terminate handler feature lets us print the exception message WITHOUT unwinding the stack,
// so that the full stack where the exception arose can be seen in an NaCl crash dump, a
// traditional Unix core dump (for native exe), a debugger, etc.
NORETURN static void TerminateHandler()
{
#ifdef __native_client__
	// Using a lambda triggers -Wformat-security...
#	define DispatchError(...) snprintf(realErrorMessage, sizeof(realErrorMessage), __VA_ARGS__)
#else
	auto DispatchError = [](const char* msg, const auto&... fmtArgs) {
		Sys::Error(msg, fmtArgs...);
	};
#endif

	if (Sys::OnMainThread()) {
		try {
			throw; // A terminate handler is only called if there is an active exception
		} catch (std::exception& err) {
			DispatchError("Unhandled exception (%s): %s", typeid(err).name(), err.what());
		} catch (...) {
			DispatchError("Unhandled exception of unknown type");
		}
	}
	std::abort();
}

// Entry point called in a new process
int main(int argc, char** argv)
{
	// The socket handle is sent as the first argument
	if (argc != 2) {
		fprintf(stderr, "This program is not meant to be invoked directly, it must be invoked by the engine's VM loader.\n");
		exit(1);
	}
	char* end;
#ifdef _WIN32
	Sys::OSHandle rootSocket = reinterpret_cast<Sys::OSHandle>(static_cast<intptr_t>(strtol(argv[1], &end, 10)));
#else
	Sys::OSHandle rootSocket = static_cast<Sys::OSHandle>(strtol(argv[1], &end, 10));
#endif
	if (argv[1] == end || *end != '\0') {
		fprintf(stderr, "Parameter is not a valid handle number\n");
		exit(1);
	}

	std::set_terminate(TerminateHandler);
	// Set up crash handling for this process. This will allow crashes to be
	// sent back to the engine and reported to the user.
	Sys::SetupCrashHandler();

	try {
		CommonInit(rootSocket);
	} catch (Sys::DropErr& err) {
		Sys::Error(err.what());
	}
	// Other exceptions go to TerminateHandler()
}

#endif
