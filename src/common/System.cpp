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
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#ifdef __linux__
#include <sys/random.h>
#endif
#ifdef __native_client__
#include <nacl/native_client/src/include/nacl/nacl_minidump.h>
#include <nacl/nacl_random.h>
#else
#include <dlfcn.h>
#endif
#endif
#ifdef BUILD_VM
#include "shared/CommonProxies.h"
#else
#include "qcommon/sys.h"
#endif

namespace Sys {

// https://devblogs.microsoft.com/oldnewthing/20120105-00/?p=8683
// TODO: also use in VMs when cvars can be observed from multiple modules
// This option can be turned on when debugging memory management
#ifdef BUILD_ENGINE
#ifdef USING_SANITIZER
constexpr bool defaultPedanticShutdown = true;
#else
constexpr bool defaultPedanticShutdown = false;
#endif
static Cvar::Cvar<bool> pedanticShutdown("common.pedanticShutdown", "run useless shutdown procedures before exit", Cvar::NONE, defaultPedanticShutdown);
#endif // BUILD_ENGINE

#if defined(BUILD_ENGINE) && defined(_WIN32)
bool isRunningOnWine()
{
	// See https://www.winehq.org/pipermail/wine-devel/2008-September/069387.html
	HMODULE hNTDLL = GetModuleHandle( "ntdll.dll" );
	return hNTDLL && (void*) GetProcAddress( hNTDLL, "wine_get_version" );
}

const char* getWineHostSystem()
{
	const char *system = nullptr;

	// See https://forum.winehq.org/viewtopic.php?p=84448#p84448
	HMODULE hNTDLL = GetModuleHandle( "ntdll.dll" );

	if ( hNTDLL )
	{
		using wine_get_host_version_t = long long int(*)(const char**, const char**);

		// HACK: C++ standard does not allow to cast function pointer to function pointer directly.
		// See https://stackoverflow.com/a/56622668/9131399
		wine_get_host_version_t wine_get_host_version = reinterpret_cast<wine_get_host_version_t>(
			reinterpret_cast<uintptr_t>( GetProcAddress( hNTDLL, "wine_get_host_version" ) ) );

		if ( wine_get_host_version )
		{
			wine_get_host_version( &system, nullptr );
		}
	}

	return system;
}
#endif

int SetEnv( const char* name, const char* value )
{
#ifdef _WIN32
	return putenv( va( "%s=%s", name, value ) );
#else
	return setenv( name, value, true );
#endif
}

int UnsetEnv( const char* name )
{
#ifdef _WIN32
	return putenv( name );
#else
	return unsetenv( name );
#endif
}

#ifdef _WIN32
SteadyClock::time_point SteadyClock::now() NOEXCEPT
{
	// Determine performance counter frequency
	static double nanosec_per_tic = 0.0;
	if (nanosec_per_tic == 0.0) {
		LARGE_INTEGER freq;
		QueryPerformanceFrequency(&freq);
		nanosec_per_tic = 1000000000.0 / freq.QuadPart;
	}

	LARGE_INTEGER time;
	QueryPerformanceCounter(&time);
	return time_point(duration(rep(nanosec_per_tic * time.QuadPart)));
}
#endif

void SleepFor(SteadyClock::duration time)
{
#ifdef _WIN32
	static ULONG maxRes = 0;
	static NTSTATUS(WINAPI *pNtSetTimerResolution) (ULONG resolution, BOOLEAN set_resolution, ULONG* current_resolution);
	static NTSTATUS(WINAPI *pNtDelayExecution) (BOOLEAN alertable, const LARGE_INTEGER* timeout);
	if (maxRes == 0) {
		// Load ntdll.dll functions
		std::string errorString;
		DynamicLib ntdll = DynamicLib::Open("ntdll.dll", errorString);
		if (!ntdll)
			Sys::Error("Failed to load ntdll.dll: %s", errorString);
		auto pNtQueryTimerResolution = ntdll.LoadSym<NTSTATUS WINAPI(ULONG*, ULONG*, ULONG*)>("NtQueryTimerResolution", errorString);
		if (!pNtQueryTimerResolution)
			Sys::Error("Failed to load NtQueryTimerResolution from ntdll.dll: %s", errorString);
		pNtSetTimerResolution = ntdll.LoadSym<NTSTATUS WINAPI(ULONG, BOOLEAN, ULONG*)>("NtSetTimerResolution", errorString);
		if (!pNtSetTimerResolution)
			Sys::Error("Failed to load NtSetTimerResolution from ntdll.dll: %s", errorString);
		pNtDelayExecution = ntdll.LoadSym<NTSTATUS WINAPI(BOOLEAN, const LARGE_INTEGER*)>("NtDelayExecution", errorString);
		if (!pNtDelayExecution)
			Sys::Error("Failed to load NtDelayExecution from ntdll.dll: %s", errorString);

		// Determine the maximum available timer resolution
		ULONG minRes, curRes;
		if (pNtQueryTimerResolution(&minRes, &maxRes, &curRes) != 0)
			maxRes = 10000; // Default to 1ms
	}

	// Increase the system timer resolution for the duration of the sleep
	ULONG curRes;
	pNtSetTimerResolution(maxRes, TRUE, &curRes);

	// Convert to NT units of 100ns
	using NTDuration = std::chrono::duration<int64_t, std::ratio<1, 10000000>>;
	auto ntTime = std::chrono::duration_cast<NTDuration>(time);

	// Store the delay as a negative number to indicate a relative sleep
	LARGE_INTEGER duration;
	duration.QuadPart = -ntTime.count();
	pNtDelayExecution(FALSE, &duration);

	// Restore timer resolution after sleeping
	pNtSetTimerResolution(maxRes, FALSE, &curRes);
#else
	std::this_thread::sleep_for(time);
#endif
}

SteadyClock::time_point SleepUntil(SteadyClock::time_point time)
{
	// Early exit if we are already past the deadline
	auto now = SteadyClock::now();
	if (now >= time) {
		// We were already past our deadline, which means that the previous frame
		// ran for too long. Use the current time as the base for the next frame.
		return now;
	}

	// Perform the actual sleep
	SleepFor(time - now);

	// We may have overslept, so use the target time rather than the
	// current time as the base for the next frame. That way we ensure
	// that the frame rate remains consistent.
	return time;
}

int Milliseconds() {
#ifdef BUILD_VM
	return trap_Milliseconds();
#else
	static Sys::SteadyClock::time_point baseTime = Sys::SteadyClock::now();
	return std::chrono::duration_cast<std::chrono::milliseconds>(Sys::SteadyClock::now() - baseTime).count();
#endif
}

#ifdef BUILD_VM_IN_PROCESS
std::thread::id mainThread;
#else
static const std::thread::id mainThread = std::this_thread::get_id();
#endif
bool OnMainThread()
{
	return std::this_thread::get_id() == mainThread;
}

void Drop(Str::StringRef message)
{
	if (!OnMainThread()) {
		Sys::Error(message);
	}

	// Transform into a fatal error if too many errors are generated in quick
	// succession.
	static Sys::SteadyClock::time_point lastError;
	Sys::SteadyClock::time_point now = Sys::SteadyClock::now();
	static int errorCount = 0;
	if (now - lastError < std::chrono::milliseconds(100)) {
		if (++errorCount > 3)
			Sys::Error(message);
	} else
		errorCount = 0;
	lastError = now;

	throw DropErr(true, message);
}

#ifdef _WIN32
std::string Win32StrError(uint32_t error)
{
	std::string out;
	char* message;
	if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM, nullptr, error, MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), reinterpret_cast<char *>(&message), 0, nullptr)) {
		out = message;

		// FormatMessage adds ".\r\n" to messages, but we don't want those
		if (out.back() == '\n')
			out.pop_back();
		if (out.back() == '\r')
			out.pop_back();
		if (out.back() == '.')
			out.pop_back();
		LocalFree(message);
	} else
		out = Str::Format("Unknown error 0x%08lx", error);
	return out;
}
#endif

// Setup crash handling
#ifdef _WIN32
static const char *WindowsExceptionString(DWORD code)
{
	switch (code) {
	case EXCEPTION_ACCESS_VIOLATION:
		return "Access violation";
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
		return "Array bounds exceeded";
	case EXCEPTION_BREAKPOINT:
		return "Breakpoint was encountered";
	case EXCEPTION_DATATYPE_MISALIGNMENT:
		return "Datatype misalignment";
	case EXCEPTION_FLT_DENORMAL_OPERAND:
		return "Float: Denormal operand";
	case EXCEPTION_FLT_DIVIDE_BY_ZERO:
		return "Float: Divide by zero";
	case EXCEPTION_FLT_INEXACT_RESULT:
		return "Float: Inexact result";
	case EXCEPTION_FLT_INVALID_OPERATION:
		return "Float: Invalid operation";
	case EXCEPTION_FLT_OVERFLOW:
		return "Float: Overflow";
	case EXCEPTION_FLT_STACK_CHECK:
		return "Float: Stack check";
	case EXCEPTION_FLT_UNDERFLOW:
		return "Float: Underflow";
	case EXCEPTION_ILLEGAL_INSTRUCTION:
		return "Illegal instruction";
	case EXCEPTION_IN_PAGE_ERROR:
		return "Page error";
	case EXCEPTION_INT_DIVIDE_BY_ZERO:
		return "Integer: Divide by zero";
	case EXCEPTION_INT_OVERFLOW:
		return "Integer: Overflow";
	case EXCEPTION_INVALID_DISPOSITION:
		return "Invalid disposition";
	case EXCEPTION_NONCONTINUABLE_EXCEPTION:
		return "Noncontinuable exception";
	case EXCEPTION_PRIV_INSTRUCTION:
		return "Privileged instruction";
	case EXCEPTION_SINGLE_STEP:
		return "Single step";
	case EXCEPTION_STACK_OVERFLOW:
		return "Stack overflow";
	default:
		return "Unknown exception";
	}
}

#ifdef _MSC_VER
// The standard library implements calling std::terminate with an exception filter
static LPTOP_LEVEL_EXCEPTION_FILTER originalExceptionFilter;
#endif

 ALIGN_STACK_FOR_MINGW static LONG WINAPI CrashHandler(PEXCEPTION_POINTERS ExceptionInfo)
{
	// Reset handler so that any future errors cause a crash
	SetUnhandledExceptionFilter(nullptr);

#ifdef _MSC_VER
	constexpr DWORD CPP_EXCEPTION = 0xe06d7363;
	if (ExceptionInfo->ExceptionRecord->ExceptionCode == CPP_EXCEPTION && originalExceptionFilter) {
		return originalExceptionFilter(ExceptionInfo);
	}
#endif

	Sys::Error("Crashed with exception 0x%lx: %s", ExceptionInfo->ExceptionRecord->ExceptionCode, WindowsExceptionString(ExceptionInfo->ExceptionRecord->ExceptionCode));
}
void SetupCrashHandler()
{
#ifdef _MSC_VER
	originalExceptionFilter =
#endif
	SetUnhandledExceptionFilter(CrashHandler);
}
#elif defined(__native_client__)
static void CrashHandler(const void* data, size_t n)
{
    // Note: this only works on the main thread. Otherwise we hit
    // Sys::Error("SendMsg from non-main VM thread");
    VM::CrashDump(static_cast<const uint8_t*>(data), n);
    Sys::Error("Crashed with NaCl exception");
}

void SetupCrashHandler()
{
    nacl_minidump_register_crash_handler();
    nacl_minidump_set_callback(CrashHandler);
}
#else
NORETURN static void CrashHandler(int sig)
{
	// TODO: backtrace

	Sys::Error("Crashed with signal %d: %s", sig, strsignal(sig));
}
void SetupCrashHandler()
{
	struct sigaction sa;
	sa.sa_flags = SA_RESETHAND | SA_NODEFER;
	sa.sa_handler = CrashHandler;
	sigemptyset(&sa.sa_mask);
	for (int sig: {SIGILL, SIGFPE, SIGSEGV, SIGABRT, SIGBUS, SIGTRAP})
		sigaction(sig, &sa, nullptr);
}
#endif

#ifndef __native_client__
void DynamicLib::Close()
{
	if (!handle)
		return;

#ifdef _WIN32
	FreeLibrary(static_cast<HMODULE>(handle));
#else
	dlclose(handle);
#endif
	handle = nullptr;
}

DynamicLib DynamicLib::Open(Str::StringRef filename, std::string& errorString)
{
#ifdef _WIN32
	void* handle = LoadLibraryW(Str::UTF8To16(filename).c_str());
	if (!handle)
		errorString = Win32StrError(GetLastError());
#else
	// Handle relative paths correctly
	const char* dlopenFilename = filename.c_str();
	std::string relativePath;
	if (filename.find('/') == std::string::npos) {
		relativePath = "./" + filename;
		dlopenFilename = relativePath.c_str();
	}

	void* handle = dlopen(dlopenFilename, RTLD_NOW);
	if (!handle)
		errorString = dlerror();
#endif

	DynamicLib out;
	out.handle = handle;
	return out;
}

intptr_t DynamicLib::InternalLoadSym(Str::StringRef sym, std::string& errorString)
{
#ifdef _WIN32
	intptr_t p = reinterpret_cast<intptr_t>(GetProcAddress(static_cast<HMODULE>(handle), sym.c_str()));
	if (!p)
		errorString = Win32StrError(GetLastError());
	return p;
#else
	intptr_t p = reinterpret_cast<intptr_t>(dlsym(handle, sym.c_str()));
	if (!p)
		errorString = dlerror();
	return p;
#endif
}
#endif // __native_client__

#ifdef BUILD_ENGINE
static bool processTerminating = false;

void OSExit(int exitCode) {
	processTerminating = true;
	if (PedanticShutdown()) {
		exit(exitCode);
	} else {
#ifdef _WIN32
		// _exit runs full shutdown for DLLs including global destructors, ewww
		TerminateProcess(GetCurrentProcess(), exitCode);
		// There are rumors of TerminateProcess returning: https://crbug.com/820518. Crash to be sure
		volatile auto p = (volatile char*)1;
		*p = 123;
		ASSERT_UNREACHABLE();
#else
		_exit(exitCode);
#endif
	}
}

bool IsProcessTerminating() {
	return processTerminating;
}

bool PedanticShutdown()
{
	return pedanticShutdown.Get();
}
#endif // BUILD_ENGINE

void GenRandomBytes(void* dest, size_t size)
{
#ifdef _WIN32
	HCRYPTPROV prov;
	if (!CryptAcquireContext(&prov, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
		Sys::Error("CryptAcquireContext failed: %s", Win32StrError(GetLastError()));

	if (!CryptGenRandom(prov, size, (BYTE*)dest))
		Sys::Error("CryptGenRandom failed: %s", Win32StrError(GetLastError()));

	CryptReleaseContext(prov, 0);
#elif defined(__native_client__)
	size_t bytes_written;
	if (nacl_secure_random(dest, size, &bytes_written) != 0 || bytes_written != size)
		Sys::Error("nacl_secure_random failed");
#elif defined(__linux__)
	ssize_t ret = getrandom(dest, size, GRND_NONBLOCK);
	if (ret == -1)
	{
		if (errno == ENOSYS)
		{
			Log::Warn("getrandom syscall is not supported");

			int fd = open("/dev/urandom", O_RDONLY);
			if (fd == -1)
				Sys::Error("Failed to open /dev/urandom: %s", strerror(errno));
			if (read(fd, dest, size) != (ssize_t) size)
				Sys::Error("Failed to read from /dev/urandom: %s", strerror(errno));
			close(fd);
		}
		else
		{
			Sys::Error("getrandom syscall failed: %s", strerror(errno));
		}
	}
	else if (static_cast<size_t>(ret) != size)
	{
		Sys::Error("getrandom syscall returned insufficient data");
	}
#else
	arc4random_buf(dest, size);
#endif
}

// Do not throw an exception when out of
// memory. Instead, it is preferable to simply crash with an error.
static void ErrorOutOfMemory()
{
	Sys::Error("Out of memory");
}
static int dummy = [] {
	std::set_new_handler(ErrorOutOfMemory);
	return 0;
}();

} // namespace Sys
