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

#include "qcommon/qcommon.h"
#include "qcommon/sys.h"
#include "VirtualMachine.h"

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#undef CopyFile
#else
#include <unistd.h>
#include <signal.h>
#include <spawn.h>
#include <fcntl.h>
#include <sys/wait.h>
#ifdef __linux__
#include <sys/prctl.h>
#if defined(DAEMON_ARCH_armhf)
#include <sys/utsname.h>
#endif
#endif
#endif

// File handle for the root socket
#define ROOT_SOCKET_FD 100

// MinGW doesn't define JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE
#ifndef JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE
#define JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE 0x2000
#endif

static Cvar::Cvar<std::string> abiVersionCvar(
	"version.daemon.abi", "Virtual machine IPC ABI version", Cvar::SERVERINFO | Cvar::ROM,
	std::string(IPC::SYSCALL_ABI_VERSION) +
	(IPC::DAEMON_HAS_COMPATIBILITY_BREAKING_SYSCALL_CHANGES ? "+compatbreak" : ""));

static Cvar::Cvar<bool> workaround_naclArchitecture_arm64_disableQualification(
	"workaround.linux.arm64.naclDisableQualification",
	"Disable platform qualification when running armhf NaCl loader on arm64 Linux",
	Cvar::NONE, true);

static Cvar::Cvar<bool> workaround_naclSystem_freebsd_disableQualification(
	"workaround.naclSystem.freebsd.disableQualification",
	"Disable platform qualification when running Linux NaCl loader on FreeBSD through Linuxulator",
	Cvar::NONE, true);

static Cvar::Cvar<bool> vm_nacl_qualification(
	"vm.nacl.qualification",
	"Enable NaCl loader platform qualification",
	Cvar::INIT, true);

static Cvar::Cvar<bool> vm_nacl_bootstrap(
	"vm.nacl.bootstrap",
	"Use NaCl bootstrap helper",
	Cvar::INIT, true);

static Cvar::Cvar<int> vm_timeout(
	"vm.timeout",
	"Receive timeout in seconds",
	Cvar::NONE, 2);

namespace VM {

// https://github.com/Unvanquished/Unvanquished/issues/944#issuecomment-744454772
static void CheckMinAddressSysctlTooLarge()
{
#ifdef __linux__
	static const bool problemDetected = [] {
		try {
			FS::File file = FS::RawPath::OpenRead("/proc/sys/vm/mmap_min_addr");
			char buf[20];
			buf[file.Read(&buf, sizeof(buf) - 1)] = '\0';
			int minAddr;
			return sscanf(buf, "%d", &minAddr) == 1  && minAddr > 0x10000;
		} catch (std::system_error&) {
			return false;
		}
	}();
	if (problemDetected) {
		Sys::Error("Your system is configured with a sysctl option which makes the game unable to run.\n"
		           "To permanently fix the configuration, run the following commands:\n"
		           "\n"
		           "    echo vm.mmap_min_addr=65536 | sudo dd of=/etc/sysctl.d/daemonengine-nacl-mmap.conf\n"
		           "    sudo /sbin/sysctl --system");
	}
#endif // __linux__
}

// Platform-specific code to load a module
static std::pair<Sys::OSHandle, IPC::Socket> InternalLoadModule(std::pair<IPC::Socket, IPC::Socket> pair, const char* const* args, bool reserve_mem, FS::File stderrRedirect = FS::File())
{
#ifdef _WIN32
	// Inherit the socket in the child process
	if (!SetHandleInformation(pair.second.GetHandle(), HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT))
		Sys::Drop("VM: Could not make socket inheritable: %s", Sys::Win32StrError(GetLastError()));

	// Inherit the stderr redirect in the child process
	HANDLE stderrRedirectHandle = stderrRedirect ? reinterpret_cast<HANDLE>(_get_osfhandle(fileno(stderrRedirect.GetHandle()))) : INVALID_HANDLE_VALUE;
	if (stderrRedirect && !SetHandleInformation(stderrRedirectHandle, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT))
		Sys::Drop("VM: Could not make stderr redirect inheritable: %s", Sys::Win32StrError(GetLastError()));

	// Escape command line arguments
	std::string cmdline;
	for (int i = 0; args[i]; i++) {
		if (i != 0)
			cmdline += " ";

		// Enclose parameter in quotes
		cmdline += "\"";
		int num_slashes = 0;
		for (int j = 0; true; j++) {
			char c = args[i][j];

			if (c == '\\')
				num_slashes++;
			else if (c == '\"' || c == '\0') {
				// Backlashes before a quote must be escaped
				for (int k = 0; k < num_slashes; k++)
					cmdline += "\\\\";
				num_slashes = 0;
				if (c == '\"')
					cmdline += "\\\"";
				else
					break;
			} else {
				// Backlashes before any other character must not be escaped
				for (int k = 0; k < num_slashes; k++)
					cmdline += "\\";
				num_slashes = 0;
				cmdline.push_back(c);
			}
		}
		cmdline += "\"";
	}

	// Convert command line to UTF-16 and add a NUL terminator
	std::wstring wcmdline = Str::UTF8To16(cmdline) + L"\0";

	// Create a job object to ensure the process is terminated if the parent dies
	HANDLE job = CreateJobObject(nullptr, nullptr);
	if (!job)
		Sys::Drop("VM: Could not create job object: %s", Sys::Win32StrError(GetLastError()));
	JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli{};
	jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
	if (!SetInformationJobObject(job, JobObjectExtendedLimitInformation, &jeli, sizeof(jeli)))
		Sys::Drop("VM: Could not set job object information: %s", Sys::Win32StrError(GetLastError()));

	STARTUPINFOW startupInfo{};
	PROCESS_INFORMATION processInfo;
	if (stderrRedirect) {
		startupInfo.hStdError = stderrRedirectHandle;
		startupInfo.dwFlags = STARTF_USESTDHANDLES;
	}
	startupInfo.cb = sizeof(startupInfo);
	if (!CreateProcessW(nullptr, &wcmdline[0], nullptr, nullptr, TRUE, CREATE_SUSPENDED | CREATE_BREAKAWAY_FROM_JOB | CREATE_NO_WINDOW, nullptr, nullptr, &startupInfo, &processInfo)) {
		CloseHandle(job);
		Sys::Drop("VM: Could not create child process: %s", Sys::Win32StrError(GetLastError()));
	}

	if (!AssignProcessToJobObject(job, processInfo.hProcess)) {
		TerminateProcess(processInfo.hProcess, 0);
		CloseHandle(job);
		CloseHandle(processInfo.hThread);
		CloseHandle(processInfo.hProcess);
		Sys::Drop("VM: Could not assign process to job object: %s", Sys::Win32StrError(GetLastError()));
	}

#ifdef _WIN64
	Q_UNUSED(reserve_mem);
#else
	// Attempt to reserve 1GB of address space for the NaCl sandbox
	if (reserve_mem)
		VirtualAllocEx(processInfo.hProcess, nullptr, 1 << 30, MEM_RESERVE, PAGE_NOACCESS);
#endif

	ResumeThread(processInfo.hThread);
	CloseHandle(processInfo.hThread);
	CloseHandle(processInfo.hProcess);

	return std::make_pair(job, std::move(pair.first));
#else
	Q_UNUSED(reserve_mem);

	posix_spawn_file_actions_t fileActions;
	if (0 != posix_spawn_file_actions_init(&fileActions) ||
	    0 != posix_spawn_file_actions_addclose(&fileActions, pair.first.GetHandle()) ||
	    0 != posix_spawn_file_actions_addclose(&fileActions, STDIN_FILENO) ||
	    0 != posix_spawn_file_actions_addclose(&fileActions, STDOUT_FILENO) ||
	    (stderrRedirect &&
	     0 != posix_spawn_file_actions_adddup2(&fileActions, fileno(stderrRedirect.GetHandle()), STDERR_FILENO))) {
		Sys::Error("VM: failed to construct posix_spawn_file_actions_t");
	}

	pid_t pid;
	int err = posix_spawn(&pid, args[0], &fileActions, nullptr, const_cast<char* const*>(args), nullptr);
	posix_spawn_file_actions_destroy(&fileActions);
	if (err != 0) {
		Sys::Drop("VM: Failed to spawn process: %s", strerror(err));
	}

	return std::make_pair(pid, std::move(pair.first));
#endif
}

static std::pair<Sys::OSHandle, IPC::Socket> CreateNaClVM(std::pair<IPC::Socket, IPC::Socket> pair, Str::StringRef name, bool debug, bool extract, int debugLoader) {
	CheckMinAddressSysctlTooLarge();
	const std::string& libPath = FS::GetLibPath();
#ifdef DAEMON_NACL_RUNTIME_PATH
	const char* naclPath = DAEMON_NACL_RUNTIME_PATH_STRING;
#else
	const std::string& naclPath = libPath;
#endif
	std::vector<const char*> args;
	char rootSocketRedir[32];
	std::string module, nacl_loader, irt, bootstrap, modulePath, verbosity;
	FS::File stderrRedirect;
#if !defined(_WIN32) || defined(_WIN64)
	constexpr bool win32Force64Bit = false;
#else
	// On Windows, even if we are running a 32-bit engine, we must use the
	// 64-bit nacl_loader if the host operating system is 64-bit.
	SYSTEM_INFO systemInfo;
	GetNativeSystemInfo(&systemInfo);
	const bool win32Force64Bit = systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64;
#endif

	// Extract the nexe from the pak so that nacl_loader can load it
	module = win32Force64Bit
		? name + "-amd64.nexe"
		: Str::Format("%s-%s.nexe", name, DAEMON_NACL_ARCH_STRING);
	if (extract) {
		try {
			FS::File out = FS::HomePath::OpenWrite(module);
			if (const FS::LoadedPakInfo* pak = FS::PakPath::LocateFile(module))
				Log::Notice("Extracting VM module %s from %s...", module.c_str(), pak->path.c_str());
			FS::PakPath::CopyFile(module, out);
			out.Close();
		} catch (std::system_error& err) {
			Sys::Drop("VM: Failed to extract VM module %s: %s", module, err.what());
		}
		modulePath = FS::Path::Build(FS::GetHomePath(), module);
	} else {
		modulePath = FS::Path::Build(libPath, module);
	}

	// Generate command line
	Q_snprintf(rootSocketRedir, sizeof(rootSocketRedir), "%d:%d", ROOT_SOCKET_FD, (int)(intptr_t)pair.second.GetHandle());
	irt = FS::Path::Build(naclPath, win32Force64Bit
		? "irt_core-amd64.nexe"
		: Str::Format("irt_core-%s.nexe", DAEMON_NACL_ARCH_STRING));
	nacl_loader = FS::Path::Build(naclPath, win32Force64Bit ? "nacl_loader-amd64" EXE_EXT : "nacl_loader" EXE_EXT);

	if (!FS::RawPath::FileExists(modulePath)) {
		Sys::Error("VM module file not found: %s", modulePath);
	}

	if (!FS::RawPath::FileExists(nacl_loader)) {
		Sys::Error("NaCl loader not found: %s", nacl_loader);
	}

	if (!FS::RawPath::FileExists(irt)) {
		Sys::Error("NaCl integrated runtime not found: %s", irt);
	}

#if defined(__linux__) || defined(__FreeBSD__)
	if (vm_nacl_bootstrap.Get()) {
#if defined(DAEMON_ARCH_arm64)
		bootstrap = FS::Path::Build(naclPath, "nacl_helper_bootstrap-armhf");
#else
		bootstrap = FS::Path::Build(naclPath, "nacl_helper_bootstrap");
#endif

		if (!FS::RawPath::FileExists(bootstrap)) {
			Sys::Error("NaCl bootstrap helper not found: %s", bootstrap);
		}

		args.push_back(bootstrap.c_str());
		args.push_back(nacl_loader.c_str());
		args.push_back("--r_debug=0xXXXXXXXXXXXXXXXX");
		args.push_back("--reserved_at_zero=0xXXXXXXXXXXXXXXXX");
	} else {
		Log::Warn("Not using NaCl bootstrap helper.");
		args.push_back(nacl_loader.c_str());
	}
#else
	Q_UNUSED(bootstrap);
	args.push_back(nacl_loader.c_str());
#endif

	bool enableQualification = vm_nacl_qualification.Get();

	if (enableQualification) {
#if defined(__linux__) && (defined(DAEMON_ARCH_arm64) || defined(DAEMON_ARCH_armhf))
		if (workaround_naclArchitecture_arm64_disableQualification.Get()) {
#if defined(DAEMON_ARCH_arm64)
			bool onArm64 = true;
#elif defined(DAEMON_ARCH_armhf)
			bool onArm64 = false;

			struct utsname buf;
			if (!uname(&buf)) {
				onArm64 = !strcmp(buf.machine, "aarch64");
			}
#endif

			/* This is required to run armhf NaCl loader on arm64 kernel
			otherwise nexe loading fails with this message:

			> Error while loading "sgame-armhf.nexe": CPU model is not supported

			From nacl_loader --help we can read:

			> -Q disable platform qualification (dangerous!)

			When this option is enabled, nacl_loader will print:

			> PLATFORM QUALIFICATION DISABLED BY -Q - Native Client's sandbox will be unreliable!

			But the nexe will load and run. */

			if (onArm64) {
				Log::Warn("Disabling NaCL platform qualification on arm64 kernel architecture.");
				enableQualification = false;
			}
		}
#endif

#if defined(__FreeBSD__)
		/* While it is possible to build a native FreeBSD engine, the only available NaCl loader
		is the Linux one, which can run on Linuxulator (the FreeBSD Linux compatibility layer).

		The Linux NaCl loader binary fails to qualify the platform and aborts with this message:

		> Bus error (core dumped)

		The Linux NaCl loader runs properly on Linuxulator when we disable the qualification. */

		if (workaround_naclSystem_freebsd_disableQualification.Get()) {
			Log::Warn("Disabling NaCL platform qualification on FreeBSD system.");
			enableQualification = false;
		}
#endif
	}
	else {
		Log::Warn("Not using NaCl platform qualification.");
	}

	if (!enableQualification) {
		args.push_back("-Q");
	}

	if (debug) {
		args.push_back("-g");
	}

	if (debugLoader) {
		std::error_code err;
		stderrRedirect = FS::HomePath::OpenWrite(name + ".nacl_loader.log", err);
		if (err)
			Log::Warn("Couldn't open %s: %s", name + ".nacl_loader.log", err.message());
		verbosity = "-";
		verbosity.append(debugLoader, 'v');
		args.push_back(verbosity.c_str());
	}

	args.push_back("-B");
	args.push_back(irt.c_str());
	args.push_back("-e");
	args.push_back("-i");
	args.push_back(rootSocketRedir);
	args.push_back("--");
	args.push_back(modulePath.c_str());
	args.push_back(XSTRING(ROOT_SOCKET_FD));
	args.push_back(nullptr);

	Log::Notice("Loading VM module %s...", module.c_str());

	if (debugLoader) {
		std::string commandLine;
		for (auto arg : args) {
			if (arg) {
				commandLine += " ";
				commandLine += arg;
			}
		}
		Log::Notice("Using loader args: %s", commandLine.c_str());
	}

	return InternalLoadModule(std::move(pair), args.data(), true, std::move(stderrRedirect));
}

static std::pair<Sys::OSHandle, IPC::Socket> CreateNativeVM(std::pair<IPC::Socket, IPC::Socket> pair, Str::StringRef name, bool debug) {
	const std::string& libPath = FS::GetLibPath();
	std::vector<const char*> args;

	std::string handleArg = std::to_string((int)(intptr_t)pair.second.GetHandle());

	std::string module = FS::Path::Build(libPath, name + "-native-exe" + EXE_EXT);
	if (debug) {
		args.push_back("/usr/bin/gdbserver");
		args.push_back("localhost:4014");
	}
	args.push_back(module.c_str());
	args.push_back(handleArg.c_str());
	args.push_back(nullptr);

	Log::Notice("Loading VM module %s...", module.c_str());

	return InternalLoadModule(std::move(pair), args.data(), true);
}

static IPC::Socket CreateInProcessNativeVM(std::pair<IPC::Socket, IPC::Socket> pair, Str::StringRef name, VM::VMBase::InProcessInfo& inProcess) {
	std::string filename = FS::Path::Build(FS::GetLibPath(), name + "-native-dll" + DLL_EXT);

	Log::Notice("Loading VM module %s...", filename.c_str());

	std::string errorString;
	inProcess.sharedLib = Sys::DynamicLib::Open(filename, errorString);
	if (!inProcess.sharedLib)
		Sys::Drop("VM: Failed to load shared library VM %s: %s", filename, errorString);

	auto vmMain = inProcess.sharedLib.LoadSym<void(Sys::OSHandle)>("vmMain", errorString);
	if (!vmMain)
		Sys::Drop("VM: Could not find vmMain function in %s: %s", filename, errorString);

	Sys::OSHandle vmSocketArg = pair.second.ReleaseHandle();
	inProcess.running = true;
	try {
		inProcess.thread = std::thread([vmMain, vmSocketArg, &inProcess]() {
			vmMain(vmSocketArg);

			std::lock_guard<std::mutex> lock(inProcess.mutex);
			inProcess.running = false;
			inProcess.condition.notify_one();
		});
	} catch (std::system_error& err) {
		// Close vmSocketArg using the Socket destructor
		IPC::Socket::FromHandle(vmSocketArg);
		inProcess.running = false;
		Sys::Drop("VM: Could not create thread for VM: %s", err.what());
	}

	return std::move(pair.first);
}

void VMBase::Create()
{
	type = static_cast<vmType_t>(params.vmType.Get());

	if (type < TYPE_BEGIN || type >= TYPE_END)
		Sys::Drop("VM: Invalid type %d", type);

	int loadStartTime = Sys::Milliseconds();

	// Free the VM if it exists
	Free();

	// Open the syscall log
	if (params.logSyscalls.Get()) {
		std::string filename = name + ".syscallLog";
		std::error_code err;
		syscallLogFile = FS::HomePath::OpenWrite(filename, err);
		if (err)
			Log::Warn("Couldn't open %s: %s", filename, err.message());
	}

	// Create the socket pair to get the handle for the root socket
	std::pair<IPC::Socket, IPC::Socket> pair = IPC::Socket::CreatePair();

	IPC::Socket rootSocket;
	if (type == TYPE_NACL || type == TYPE_NACL_LIBPATH) {
		std::tie(processHandle, rootSocket) = CreateNaClVM(std::move(pair), name, params.debug.Get(), type == TYPE_NACL, params.debugLoader.Get());
	} else if (type == TYPE_NATIVE_EXE) {
		std::tie(processHandle, rootSocket) = CreateNativeVM(std::move(pair), name, params.debug.Get());
	} else {
		rootSocket = CreateInProcessNativeVM(std::move(pair), name, inProcess);
	}
	rootChannel = IPC::Channel(std::move(rootSocket));

	if (type != TYPE_NATIVE_DLL && params.debug.Get())
		Log::Notice("Waiting for GDB connection on localhost:4014");

	// Only set a receive timeout for non-debug configurations, otherwise it
	// would get triggered by breakpoints.
	if (type != TYPE_NATIVE_DLL && !params.debug.Get()) {
		rootChannel.SetRecvTimeout(std::chrono::seconds(vm_timeout.Get()));
	}

	// Read the ABI version detection ABI version from the root socket.
	// If this fails, we assume the remote process failed to start
	Util::Reader reader = rootChannel.RecvMsg();

	// VM version incompatibility detection...

	uint32_t magic = reader.Read<uint32_t>();
	if (magic != IPC::ABI_VERSION_DETECTION_ABI_VERSION) {
		Sys::Drop("Couldn't load the %s gamelogic module: it is built for %s version of Daemon engine",
		          this->name, magic > IPC::ABI_VERSION_DETECTION_ABI_VERSION ? "a newer" : "an older");
	}

	std::string vmABI = reader.Read<std::string>();
	if (vmABI != IPC::SYSCALL_ABI_VERSION) {
		Sys::Drop("Couldn't load the %s gamelogic module: it uses ABI version %s but this Daemon engine uses %s",
		          this->name, vmABI, IPC::SYSCALL_ABI_VERSION);
	}

	bool vmCompatBreaking = reader.Read<bool>();
	if (vmCompatBreaking && !IPC::DAEMON_HAS_COMPATIBILITY_BREAKING_SYSCALL_CHANGES) {
		Sys::Drop("Couldn't load the %s gamelogic module: it has compatibility-breaking ABI changes but Daemon engine uses the vanilla %s ABI",
		          this->name, IPC::SYSCALL_ABI_VERSION);
	} else if (!vmCompatBreaking && IPC::DAEMON_HAS_COMPATIBILITY_BREAKING_SYSCALL_CHANGES) {
		Sys::Drop("Couldn't load the %s gamelogic module: Daemon has compatibility-breaking ABI changes but the VM uses the vanilla %s ABI",
		          this->name, IPC::SYSCALL_ABI_VERSION);
	} else if (IPC::DAEMON_HAS_COMPATIBILITY_BREAKING_SYSCALL_CHANGES) {
		Log::Notice("^6Using %s VM with unreleased ABI changes", this->name);
	}

	Log::Notice("Loaded %s VM module in %d msec", this->name, Sys::Milliseconds() - loadStartTime);
}

void VMBase::FreeInProcessVM() {
	if (inProcess.thread.joinable()) {
		bool wait = true;
		if (inProcess.running) {
			std::unique_lock<std::mutex> lock(inProcess.mutex);
			auto status = inProcess.condition.wait_for(lock, std::chrono::milliseconds(500));
			if (status == std::cv_status::timeout) {
				wait = false;
			}
		}

		if (wait) {
			Log::Notice("Waiting for the VM thread...");
			inProcess.thread.join();
		} else {
			Log::Notice("The VM thread doesn't seem to stop, detaching it (bad things WILL ensue)");
			inProcess.thread.detach();
		}
	}

	inProcess.sharedLib.Close();
	inProcess.running = false;
}

void VMBase::LogMessage(bool vmToEngine, bool start, int id)
{
	if (syscallLogFile) {
		int minor = id & 0xffff;
		int major = id >> 16;

		const char* direction = vmToEngine ? "V->E" : "E->V";
		const char* extremity = start ? "start" : "end";
		uint64_t ns = std::chrono::duration_cast<std::chrono::nanoseconds>(Sys::SteadyClock::now().time_since_epoch()).count();
		try {
			syscallLogFile.Printf("%s %s %s %s %s\n", direction, extremity, major, minor, ns);
		} catch (std::system_error& err) {
			Log::Warn("Error while writing the VM syscall log: %s", err.what());
		}
	}
}

void VMBase::Free()
{
	if (syscallLogFile) {
		std::error_code err;
		syscallLogFile.Close(err);
	}

	if (!IsActive())
		return;

	// First send a message signaling an exit to the VM
	// then delete the socket. This is needed because
	// recvmsg in NaCl doesn't return when the socket has
	// been closed.
	Util::Writer writer;
	writer.Write<uint32_t>(IPC::ID_EXIT);
	try {
		rootChannel.SendMsg(writer);
	} catch (Sys::DropErr& err) {
		// Verbose, since an error was probably already logged when sending the shutdown message
		Log::Verbose("Error sending exit message to %s: %s", name, err.what());
	}
	rootChannel = IPC::Channel();

	if (type != TYPE_NATIVE_DLL) {
#ifdef _WIN32
		// Closing the job object should kill the child process
		CloseHandle(processHandle);
#else
		int status;
		if (waitpid(processHandle, &status, WNOHANG) != 0) {
			if (WIFSIGNALED(status))
				Log::Warn("VM exited with signal %d: %s", WTERMSIG(status), strsignal(WTERMSIG(status)));
			else if (WIFEXITED(status))
				Log::Warn("VM exited with non-zero exit code %d", WEXITSTATUS(status));
		}
		kill(processHandle, SIGKILL);
		waitpid(processHandle, nullptr, 0);
#endif
		processHandle = Sys::INVALID_HANDLE;
	} else {
		FreeInProcessVM();
	}

}

VMBase::~VMBase()
{
	Free();
}

} // namespace VM
