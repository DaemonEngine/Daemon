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
#include "common/IPC/CommonSyscalls.h"
#include "VMMain.h"

// The old console command handler that should be defined in all VMs
bool ConsoleCommand();
void CompleteCommand(int);

// Command related syscall handling

namespace Cmd {

    // This is a proxy environment given to commands, to replicate the common API
    // Calls to its methods will be proxied to Cmd::GetEnv() in the engine.
    class ProxyEnvironment: public Cmd::Environment {
        public:
            void Print(Str::StringRef text) override {
                VM::SendMsg<VM::EnvPrintMsg>(text);
            }
            void ExecuteAfter(Str::StringRef text, bool parseCvars) override {
                VM::SendMsg<VM::EnvExecuteAfterMsg>(text, parseCvars);
            }
    };

    Environment* GetEnv() {
        static Environment* env = new ProxyEnvironment;
        return env;
    }

    // Commands can be statically initialized so we must store the description so that we can register them after main

    struct CommandRecord {
        const Cmd::CmdBase* cmd;
        std::string description;
    };

    using CommandMap = std::unordered_map<std::string, CommandRecord, Str::IHash, Str::IEqual>;

    CommandMap& GetCommandMap() {
        static CommandMap map;
        return map;
    }

    static bool commandsInitialized = false;

    static void AddCommandRPC(std::string name, std::string description) {
        VM::SendMsg<VM::AddCommandMsg>(name, description);
    }

    static void InitializeProxy() {
        for(auto& record: GetCommandMap()) {
            AddCommandRPC(record.first, std::move(record.second.description));
        }

        commandsInitialized = true;
    }

    void AddCommand(const std::string& name, const Cmd::CmdBase& cmd, std::string description) {
        if (commandsInitialized) {
            GetCommandMap()[name] = {&cmd, ""};
            AddCommandRPC(name, std::move(description));
        } else {
            GetCommandMap()[name] = {&cmd, std::move(description)};
        }
    }

    void RemoveCommand(const std::string& name) {
        GetCommandMap().erase(name);

        VM::SendMsg<VM::RemoveCommandMsg>(name);
    }

    // Implementation of the engine syscalls

    void ExecuteSyscall(Util::Reader& reader, IPC::Channel& channel) {
        IPC::HandleMsg<VM::ExecuteMsg>(channel, std::move(reader), [](std::string command){
            Cmd::Args args(command);

            auto map = GetCommandMap();
            auto it = map.find(args.Argv(0));

            if (it == map.end()) {
                return;
            }

            it->second.cmd->Run(args);
        });
    }

    void CompleteSyscall(Util::Reader& reader, IPC::Channel& channel) {
        IPC::HandleMsg<VM::CompleteMsg>(channel, std::move(reader), [](int argNum, std::string command, std::string prefix, Cmd::CompletionResult& res) {
            Cmd::Args args(command);

            auto map = GetCommandMap();
            auto it = map.find(args.Argv(0));

            if (it == map.end()) {
                return;
            }

            res = it->second.cmd->Complete(argNum, args, prefix);
        });
    }

    void HandleSyscall(int minor, Util::Reader& reader, IPC::Channel& channel) {
        switch (minor) {
            case VM::EXECUTE:
                ExecuteSyscall(reader, channel);
                break;

            case VM::COMPLETE:
                CompleteSyscall(reader, channel);
                break;

            default:
                Sys::Drop("Unhandled engine command syscall %i", minor);
        }
    }
}

// Simulation of the command-related trap calls
// The old VM command code registers the command as a string. When the engine calls the command
// it performs a special VM call and the VM will check the string to dispatch to the right function.
// Then to get the arguments the VM uses the trap_Argc and trap_Argv calls.

// This vector is used as a stack of Cmd::Args for when commands are called recursively.
static std::vector<const Cmd::Args*> argStack;

// The complete trap calls worked by tokenizing the args in trap_Argc and trap_Argv
// then it called the completion syscall CompleteCommand that called trap_CompleteCallback
// with the potential values. Our new system returns a vector of candidates so we fake the
// trap_CompleteCallback call to add the candidate to our vector
static Cmd::CompletionResult completeMatches;
static std::string completedPrefix;

void trap_CompleteCallback( const char *complete ) {
    // The callback is called for each valid arg, without filtering so do it here
    if (Str::IsIPrefix(completedPrefix, complete)) {
        completeMatches.push_back({complete, ""});
    }
}

// A proxy command added instead of the string when the VM registers a command? It will just
// setup the args right and call the command Dispatcher
class TrapProxyCommand: public Cmd::CmdBase {
    public:
        TrapProxyCommand() : Cmd::CmdBase(0) {
        }
        void Run(const Cmd::Args& args) const override {
            // Push a pointer to args, it is fine because we remove the pointer before args goes out of scope
            argStack.push_back(&args);
            ConsoleCommand();
            argStack.pop_back();
        }

        Cmd::CompletionResult Complete(int argNum, const Cmd::Args& args, Str::StringRef prefix) const override {
            static char buffer[4096];

            completedPrefix = prefix;
            completeMatches.clear();

            //Completing an empty arg, we add a space to mimic the old autocompletion behavior
            if (args.Argc() == argNum) {
                Q_strncpyz(buffer, (args.ConcatArgs(0) + " ").c_str(), sizeof(buffer));
            } else {
                Q_strncpyz(buffer, args.ConcatArgs(0).c_str(), sizeof(buffer));
            }

            //Some completion handlers expect tokenized arguments
            argStack.push_back(&args);
            CompleteCommand(argNum);
            argStack.pop_back();

            return completeMatches;
        }
};

static TrapProxyCommand trapProxy;

void trap_AddCommand(const char *cmdName) {
    Cmd::AddCommand(cmdName, trapProxy, "an old style vm command");
}

void trap_RemoveCommand(const char *cmdName) {
    Cmd::RemoveCommand(cmdName);
}

int trap_Argc() {
    if (argStack.empty()) {
        return 0;
    }
    return argStack.back()->Argc();
}

void trap_Argv(int n, char *buffer, int bufferLength) {
    if (bufferLength <= 0 or argStack.empty()) {
        return;
    }

    if (n < argStack.back()->Argc()) {
        Q_strncpyz(buffer, argStack.back()->Argv(n).c_str(), bufferLength);
    } else {
        buffer[0] = '\0';
    }
}

const Cmd::Args& trap_Args() {
	static const Cmd::Args empty;
	if (argStack.empty()) {
		return empty;
	}
	return *argStack.back();
}

void trap_EscapedArgs( char *buffer, int bufferLength ) {
	const Cmd::Args* args = argStack.back();
	std::string res = args->EscapedArgs(0);
	Q_strncpyz( buffer, res.c_str(), bufferLength );
}

void trap_LiteralArgs( char *buffer, int bufferLength ) {
	const Cmd::Args* args = argStack.back();
	std::string res = args->ConcatArgs(0);
	Q_strncpyz( buffer, res.c_str(), bufferLength );
}

namespace Cmd {

    // We also provide an API to manipulate the stack for example when the VM asks for commands to be tokenized
    void PushArgs(Str::StringRef args) {
        argStack.push_back(new Cmd::Args(args));
    }

    void PopArgs() {
        delete argStack.back();
        argStack.pop_back();
    }

}

// Cvar related commands

namespace Cvar{

    struct CvarRecord {
        CvarProxy* cvar;

        // Cache values of registered cvars to reduce trap calls.
        // A Cvar<T> knows its value as a T, but not the string representation, so we also need
        // to store the string if we want GetValue() to be 100% correct.
        std::string currentValue;

        // Commands can be statically initialized so we must store the registration parameters
        // so that we can register them after main
        std::string description;
        int flags;
        std::string defaultValue;
    };

    using CvarMap = std::unordered_map<std::string, CvarRecord>;

    CvarMap& GetCvarMap() {
        static CvarMap map;
        return map;
    }

    static bool cvarsInitialized = false;

    void RegisterCvarRPC(const std::string& name, std::string description, int flags, std::string defaultValue) {
        VM::SendMsg<VM::RegisterCvarMsg>(name, description, flags, defaultValue);
    }

    static void InitializeProxy() {
        for(auto& record: GetCvarMap()) {
            RegisterCvarRPC(record.first, std::move(record.second.description), record.second.flags, std::move(record.second.defaultValue));
        }

        cvarsInitialized = true;
    }

    bool Register(CvarProxy* cvar, const std::string& name, std::string description, int flags, const std::string& defaultValue) {
        if (cvarsInitialized) {
            GetCvarMap()[name] = {cvar, defaultValue, "", 0, defaultValue};
            RegisterCvarRPC(name, std::move(description), flags, defaultValue);
        } else {
            GetCvarMap()[name] = {cvar, defaultValue, std::move(description), flags, defaultValue};
        }
        return true;
    }

    std::string GetValue(const std::string& name) {
        const CvarMap& map = GetCvarMap();
        auto it = map.find(name);
        if (it != map.end()) { // The cache will be used for cvars registered in this VM
            return it->second.currentValue;
        }

        std::string value;
        VM::SendMsg<VM::GetCvarMsg>(name, value);
        return value;
    }

    void SetValue(const std::string& name, const std::string& value) {
        VM::SendMsg<VM::SetCvarMsg>(name, value);
    }

    bool AddFlags(const std::string& name, int flags) {
        bool exists;
        VM::SendMsg<VM::AddCvarFlagsMsg>(name, flags, exists);
        return exists;
    }

    // Syscalls called by the engine

    void CallOnValueChangedSyscall(Util::Reader& reader, IPC::Channel& channel) {
        IPC::HandleMsg<VM::OnValueChangedMsg>(channel, std::move(reader), [](std::string name, std::string value, bool& success, std::string& description) {
            CvarMap& map = GetCvarMap();
            auto it = map.find(name);

            if (it == map.end()) {
                Log::Warn("Cvar '%' not registered here", name);
                success = true;
                description = "";
                return;
            }

            auto res = it->second.cvar->OnValueChanged(value);
            success = res.success;
            description = res.description;

            if (success) {
                it->second.currentValue = value; // Update cache
            }
        });
    }

    void HandleSyscall(int minor, Util::Reader& reader, IPC::Channel& channel) {
        switch (minor) {
            case VM::ON_VALUE_CHANGED:
                CallOnValueChangedSyscall(reader, channel);
                break;

            default:
                Sys::Drop("Unhandled engine cvar syscall %i", minor);
        }
    }
}

void trap_Cvar_Set(const char *varName, const char *value) {
    if (!value) {
        value = "";
    }
    Cvar::SetValue(varName, value);
}

int trap_Cvar_VariableIntegerValue(const char *varName) {
    std::string value = Cvar::GetValue(varName);
    int res;
    if (Str::ParseInt(res, value)) {
        return res;
    }
    return 0;
}

float trap_Cvar_VariableValue(const char *varName) {
    std::string value = Cvar::GetValue(varName);
    float res;
    if (Str::ToFloat(value, res)) {
        return res;
    }
    return 0.0;
}

void trap_Cvar_VariableStringBuffer(const char *varName, char *buffer, int bufsize) {
    std::string value = Cvar::GetValue(varName);
    Q_strncpyz(buffer, value.c_str(), bufsize);
}

void trap_Cvar_AddFlags(const char* varName, int flags) {
    Cvar::AddFlags(varName, flags);
}

// Log related commands

namespace Log {

    void Dispatch(Event event, int targetControl) {
        VM::SendMsg<VM::DispatchLogEventMsg>(event.text, targetControl);
    }

}

// Common functions for all syscalls

static Sys::SteadyClock::time_point baseTime;
int trap_Milliseconds()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(Sys::SteadyClock::now() - baseTime).count();
}


namespace VM {

    void CrashDump(const uint8_t* data, size_t size) {
        SendMsg<CrashDumpMsg>(std::vector<uint8_t>{data, data + size});
    }

    void InitializeProxies(int milliseconds) {
        baseTime = Sys::SteadyClock::now() - std::chrono::milliseconds(milliseconds);
        Cvar::InitializeProxy();

#ifdef BUILD_VM_NATIVE_EXE
        // Set up the crash handler now that we have cvars to know if we want to use it.
        // The handler wouldn't have worked before StaticInitMsg anyway since it needs IPC to log anything.
        if (useNativeExeCrashHandler.Get()) {
            Sys::SetupCrashHandler();
        }
#endif

        Cmd::InitializeProxy();
    }

    static void HandleMiscSyscall(int minor, Util::Reader& reader, IPC::Channel& channel) {
        switch (minor) {
            case VM::GET_NETCODE_TABLES:
                IPC::HandleMsg<GetNetcodeTablesMsg>(channel, std::move(reader), [](NetcodeTable& playerStateTable, int& playerStateSize) {
                    GetNetcodeTables(playerStateTable, playerStateSize);
                });
                break;
            default:
                Sys::Drop("Unhandled misc engine->VM syscall %d", minor);
        }
    }

    void HandleCommonSyscall(int major, int minor, Util::Reader reader, IPC::Channel& channel) {
        switch (major) {
            case VM::MISC:
                HandleMiscSyscall(minor, reader, channel);
                break;

            case VM::COMMAND:
                Cmd::HandleSyscall(minor, reader, channel);
                break;

            case VM::CVAR:
                Cvar::HandleSyscall(minor, reader, channel);
                break;

            default:
                Sys::Drop("Unhandled common VM syscall major number %i", major);
        }
    }
}

// Definition of some additional trap_* that are common to all VMs

void trap_SendConsoleCommand(const char *text)
{
	VM::SendMsg<VM::SendConsoleCommandMsg>(text);
}

// Open a file in <homepath>/game/
int trap_FS_FOpenFile(const char *qpath, fileHandle_t *f, fsMode_t mode)
{
	int length, handle;
	VM::SendMsg<VM::FSFOpenFileMsg>(qpath, f != nullptr, Util::ordinal(mode), length, handle);
	if (f)
		*f = handle;
	return length;
}

// Open for reading
int trap_FS_OpenPakFile(Str::StringRef path, fileHandle_t &f)
{
	int length;
	VM::SendMsg<VM::FSOpenPakFileReadMsg>(path, length, f);
	return length;
}

int trap_FS_Read(void *buffer, int len, fileHandle_t f)
{
	std::string res;
	int ret;
	VM::SendMsg<VM::FSReadMsg>(f, len, res, ret);
	memcpy(buffer, res.c_str(), std::min((int)res.size() + 1, len));
	return ret;
}

int trap_FS_Write(const void *buffer, int len, fileHandle_t f)
{
	int res;
	std::string text((const char*)buffer, len);
	VM::SendMsg<VM::FSWriteMsg>(f, text, res);
	return res;
}

int trap_FS_Seek( fileHandle_t f, int offset, fsOrigin_t origin )
{
    int res;
	VM::SendMsg<VM::FSSeekMsg>(f, offset, Util::ordinal(origin), res);
    return res;
}

int trap_FS_Tell( fileHandle_t f )
{
	int ret;
	VM::SendMsg<VM::FSTellMsg>(f, ret);
	return ret;
}

int trap_FS_FileLength( fileHandle_t f )
{
	int ret;
	VM::SendMsg<VM::FSFileLengthMsg>(f, ret);
	return ret;
}

void trap_FS_FCloseFile(fileHandle_t f)
{
	VM::SendMsg<VM::FSFCloseFileMsg>(f);
}

// TODO: in many cases we want only VFS (pakpath) files, not VFS + gamepath which this gives
int trap_FS_GetFileList(const char *path, const char *extension, char *listbuf, int bufsize)
{
	int res;
	std::string text;
	VM::SendMsg<VM::FSGetFileListMsg>(path, extension, bufsize, res, text);
	memcpy(listbuf, text.c_str(), std::min((int)text.size() + 1, bufsize));
	return res;
}

int trap_FS_GetFileListRecursive(const char *path, const char *extension, char *listbuf, int bufsize)
{
	int res;
	std::string text;
	VM::SendMsg<VM::FSGetFileListRecursiveMsg>(path, extension, bufsize, res, text);
	memcpy(listbuf, text.c_str(), std::min((int)text.size() + 1, bufsize));
	return res;
}

bool trap_FindPak(const char *name)
{
	bool res;
	VM::SendMsg<VM::FSFindPakMsg>(name, res);
	return res;
}

bool trap_FS_LoadPak( const char *pak, const char* prefix )
{
	bool res;
	VM::SendMsg<VM::FSLoadPakMsg>(pak, prefix, res);
	return res;
}
