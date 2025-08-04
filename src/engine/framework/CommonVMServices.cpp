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
#include "CommonVMServices.h"
#include "framework/CommandSystem.h"
#include "framework/CrashDump.h"
#include "framework/CvarSystem.h"
#include "framework/LogSystem.h"
#include "framework/VirtualMachine.h"

// Suppress warnings for unused [this] lambda captures.
#ifdef __clang__
#pragma clang diagnostic ignored "-Wunused-lambda-capture"
#endif

namespace VM {

    // Misc Related

    void CommonVMServices::HandleMiscSyscall(int minor, Util::Reader& reader, IPC::Channel& channel) {
        switch(minor) {
            case CREATE_SHARED_MEMORY:
                IPC::HandleMsg<CreateSharedMemoryMsg>(channel, std::move(reader), [this](size_t size, IPC::SharedMemory& shm) {
                    shm = IPC::SharedMemory::Create(size);
                });
                break;

            case CRASH_DUMP:
                IPC::HandleMsg<CrashDumpMsg>(channel, std::move(reader), [this](std::vector<uint8_t> dump) {
                    Sys::NaclCrashDump(dump, vmName);
                });
                break;
        }
    }

    // Command Related

    void CommonVMServices::HandleCommandSyscall(int minor, Util::Reader& reader, IPC::Channel& channel) {
        switch(minor) {
            case ADD_COMMAND:
                AddCommand(reader, channel);
                break;

            case REMOVE_COMMAND:
                RemoveCommand(reader, channel);
                break;

            case ENV_PRINT:
                EnvPrint(reader, channel);
                break;

            case ENV_EXECUTE_AFTER:
                EnvExecuteAfter(reader, channel);
                break;

            default:
                Sys::Drop("Bad command syscall number '%d' for VM '%s'", minor, vmName);
        }
    }

    class CommonVMServices::ProxyCmd: public Cmd::CmdBase {
        public:
            ProxyCmd(CommonVMServices& services, int flag): Cmd::CmdBase(flag), services(services) {
            }
            ~ProxyCmd() override = default;

            void Run(const Cmd::Args& args) const override {
                services.GetVM().SendMsg<ExecuteMsg>(args.EscapedArgs(0));
            }

            Cmd::CompletionResult Complete(int argNum, const Cmd::Args& args, Str::StringRef prefix) const override {
                Cmd::CompletionResult res;
                services.GetVM().SendMsg<CompleteMsg>(argNum, args.EscapedArgs(0), prefix, res);
                return res;
            }

        private:
            CommonVMServices& services;
    };

    void CommonVMServices::AddCommand(Util::Reader& reader, IPC::Channel& channel) {
        IPC::HandleMsg<AddCommandMsg>(channel, std::move(reader), [this](std::string name, std::string description){
            if (Cmd::CommandExists(name)) {
                Log::Warn("VM '%s' tried to register command '%s' which is already registered", vmName, name);
                return;
            }

            Cmd::AddCommand(name, *commandProxy, description);
            registeredCommands[name] = 0;
        });
    }

    void CommonVMServices::RemoveCommand(Util::Reader& reader, IPC::Channel& channel) {
        IPC::HandleMsg<RemoveCommandMsg>(channel, std::move(reader), [this](std::string name){
            if (registeredCommands.find(name) != registeredCommands.end()) {
                Cmd::RemoveCommand(name);
            }
        });
    }

    void CommonVMServices::EnvPrint(Util::Reader& reader, IPC::Channel& channel) {
        IPC::HandleMsg<EnvPrintMsg>(channel, std::move(reader), [this](std::string line){
            //TODO allow it only if we are in a command?
            Cmd::GetEnv()->Print(line);
        });
    }

    void CommonVMServices::EnvExecuteAfter(Util::Reader& reader, IPC::Channel& channel) {
        IPC::HandleMsg<EnvExecuteAfterMsg>(channel, std::move(reader), [this](std::string commandText, bool parseCvars){
            //TODO check that it isn't sending /quit or other bad commands (/lua "rootkit()")?
            Cmd::GetEnv()->ExecuteAfter(commandText, parseCvars);
        });
    }

    // Cvar related

    class CommonVMServices::ProxyCvar : public Cvar::CvarProxy {
        public:
            ProxyCvar(CommonVMServices* services, std::string name, std::string description, int flags, std::string defaultValue)
            :CvarProxy(std::move(name), flags, std::move(defaultValue)), services(services) {
                wasAdded = Register(std::move(description));
            }
            virtual ~ProxyCvar() override {
                if (wasAdded) {
                    Cvar::Unregister(name);
                }
            }

            virtual Cvar::OnValueChangedResult OnValueChanged(Str::StringRef newValue) override {
                Cvar::OnValueChangedResult result;
                services->GetVM().SendMsg<OnValueChangedMsg>(name, newValue, result.success, result.description);
                return result;
            }

        private:
            bool wasAdded;
            CommonVMServices* services;
    };

    void CommonVMServices::HandleCvarSyscall(int minor, Util::Reader& reader, IPC::Channel& channel) {
        switch(minor) {
            case REGISTER_CVAR:
                RegisterCvar(reader, channel);
                break;

            case GET_CVAR:
                GetCvar(reader, channel);
                break;

            case SET_CVAR:
                SetCvar(reader, channel);
                break;

            case ADD_CVAR_FLAGS:
				AddCvarFlags(reader, channel);
				break;

            default:
                Sys::Drop("Bad cvar syscall number '%d' for VM '%s'", minor, vmName);
        }
    }

    void CommonVMServices::RegisterCvar(Util::Reader& reader, IPC::Channel& channel) {
        IPC::HandleMsg<RegisterCvarMsg>(channel, std::move(reader), [this](std::string name, std::string description,
                int flags, std::string defaultValue){
            // The registration of the cvar is made automatically when it is created
            registeredCvars.emplace_back(Util::make_unique<ProxyCvar>(this, name, description, flags, defaultValue));
        });
    }

    void CommonVMServices::GetCvar(Util::Reader& reader, IPC::Channel& channel) {
        IPC::HandleMsg<GetCvarMsg>(channel, std::move(reader), [&, this](const std::string& name, std::string& value){
            //TODO check it is only looking at allowed cvars?
            value = Cvar::GetValue(name);
        });
    }

    void CommonVMServices::SetCvar(Util::Reader& reader, IPC::Channel& channel) {
        // Leaving value by value for now. May revisit later.
        IPC::HandleMsg<SetCvarMsg>(channel, std::move(reader), [this](const std::string& name, std::string value){
            //TODO check it is only touching allowed cvars?
            Cvar::SetValue(name, value);
        });
    }

    void CommonVMServices::AddCvarFlags(Util::Reader& reader, IPC::Channel& channel) {
        IPC::HandleMsg<AddCvarFlagsMsg>(channel, std::move(reader), [this](const std::string& name, int flags, bool& exists){
            //TODO check it is only touching allowed cvars?
            exists = Cvar::AddFlags(name, flags);
        });
    }

    // Log Related
    void CommonVMServices::HandleLogSyscall(int minor, Util::Reader& reader, IPC::Channel& channel) {
        switch(minor) {
            case DISPATCH_EVENT:
                IPC::HandleMsg<DispatchLogEventMsg>(channel, std::move(reader), [this](std::string text, int targetControl){
                    Log::Dispatch(Log::Event(std::move(text)), targetControl);
                });
                break;

            default:
                Sys::Drop("Bad log syscall number '%d' for VM '%s'", minor, vmName);
        }
    }

    // Common common QVM syscalls
    void CommonVMServices::HandleCommonQVMSyscall(int minor, Util::Reader& reader, IPC::Channel& channel) {
        switch (minor) {
            case QVM_COMMON_ERROR:
                IPC::HandleMsg<ErrorMsg>(channel, std::move(reader), [this](const std::string& text) {
                    Sys::Drop("%s VM: %s", vmName, text);
                });
                break;

            case QVM_COMMON_SEND_CONSOLE_COMMAND:
                IPC::HandleMsg<SendConsoleCommandMsg>(channel, std::move(reader), [this](const std::string& text) {
                    Cmd::BufferCommandText(text);
                });
                break;

            case QVM_COMMON_FS_FOPEN_FILE:
                IPC::HandleMsg<FSFOpenFileMsg>(channel, std::move(reader), [this](const std::string& filename, bool open, int fsMode, int& length, int& handle) {
                    fsMode_t mode = static_cast<fsMode_t>(fsMode);
                    length = FS_Game_FOpenFileByMode(filename.c_str(), open ? &handle : nullptr, mode);
                    if (handle > 0)
                        FS_SetOwner(handle, fileOwnership);
                });
                break;

            case QVM_COMMON_FS_OPEN_PAK_FILE_READ:
                IPC::HandleMsg<FSOpenPakFileReadMsg>(channel, std::move(reader), [this](const std::string& filename, int& length, int& handle) {
                    if (!FS::PakPath::FileExists(filename)) {
                        handle = 0;
                        length = -1;
                        return;
                    }
                    length = FS_FOpenFileRead(filename.c_str(), &handle);
                    if (handle > 0)
                        FS_SetOwner(handle, fileOwnership);
                });
                break;

            case QVM_COMMON_FS_READ:
                IPC::HandleMsg<FSReadMsg>(channel, std::move(reader), [this](int handle, int len, std::string& res, int& ret) {
                    FS_CheckOwnership(handle, fileOwnership);
                    res.resize( len );
                    ret = FS_Read( &res[0], len, handle );

                    res.resize( ret );
                    res.shrink_to_fit();
                });
                break;

            case QVM_COMMON_FS_WRITE:
                IPC::HandleMsg<FSWriteMsg>(channel, std::move(reader), [this](int handle, const std::string& text, int& res) {
                    FS_CheckOwnership(handle, fileOwnership);
                    res = FS_Write(text.c_str(), text.size(), handle);
                });
                break;

            case QVM_COMMON_FS_SEEK:
                IPC::HandleMsg<VM::FSSeekMsg>(channel, std::move(reader), [this] (int f, long offset, int origin, int& res) {
                    FS_CheckOwnership(f, fileOwnership);
                    res = FS_Seek(f, offset, Util::enum_cast<fsOrigin_t>(origin));
                });
                break;

            case QVM_COMMON_FS_TELL:
                IPC::HandleMsg<VM::FSTellMsg>(channel, std::move(reader), [this] (fileHandle_t f, int& res) {
                    FS_CheckOwnership(f, fileOwnership);
                    res = FS_FTell(f);
                });
                break;
			case QVM_COMMON_FS_FILELENGTH:
				IPC::HandleMsg<VM::FSFileLengthMsg>(channel, std::move(reader), [this] (fileHandle_t f, int& res) {
					FS_CheckOwnership(f, fileOwnership);
					res = FS_filelength(f);
				});
				break;

            case QVM_COMMON_FS_FCLOSE_FILE:
                IPC::HandleMsg<FSFCloseFileMsg>(channel, std::move(reader), [this](int handle) {
                    FS_CheckOwnership(handle, fileOwnership);
                    FS_FCloseFile(handle);
                });
                break;

            case QVM_COMMON_FS_GET_FILE_LIST:
                IPC::HandleMsg<FSGetFileListMsg>(channel, std::move(reader), [this](const std::string& path, std::string extension, int len, int& intRes, std::string& res) {
                    res.resize( len );
                    intRes = FS_GetFileList(path.c_str(), extension.c_str(), &res[0], len);
                });
                break;

            case QVM_COMMON_FS_GET_FILE_LIST_RECURSIVE:
                IPC::HandleMsg<FSGetFileListRecursiveMsg>(channel, std::move(reader), [this](const std::string& path, std::string extension, int len, int& intRes, std::string& res) {
                    res.resize( len );
                    intRes = FS_GetFileListRecursive(path.c_str(), extension.c_str(), &res[0], len);
                });
                break;

            case QVM_COMMON_FS_FIND_PAK:
                IPC::HandleMsg<FSFindPakMsg>(channel, std::move(reader), [this](const std::string& pakName, bool& found) {
                    found = FS::FindPak(pakName) != nullptr;
                });
                break;

            case QVM_COMMON_FS_LOAD_PAK:
                IPC::HandleMsg<FSLoadPakMsg>(channel, std::move(reader), [this](const std::string& pakName, const std::string& prefix, bool& found) {
                    const FS::PakInfo* pak = FS::FindPak(pakName);
                    if (!pak) {
                        found = false;
                        return;
                    }
                    std::error_code err;
                    FS::PakPath::LoadPakPrefix(*FS::FindPak(pakName), prefix, err);
                    // found if no error
                    found = !err;
                });
                break;

            default:
                Sys::Drop("Bad log syscall number '%d' for VM '%s'", minor, vmName.c_str());
        }
    }

    // Misc, Dispatch

    CommonVMServices::CommonVMServices(VMBase& vm, Str::StringRef vmName, FS::Owner fileOwnership, int commandFlag)
    :vmName(vmName), fileOwnership(fileOwnership), vm(vm), commandProxy(new ProxyCmd(*this, commandFlag)) {
    }

    CommonVMServices::~CommonVMServices() {
        FS_CloseAllForOwner(fileOwnership);
        //FIXME or iterate over the commands we registered, or add Cmd::RemoveByProxy()
        Cmd::RemoveSameCommands(*commandProxy.get());
        //TODO unregister cvars
    }

    void CommonVMServices::Syscall(int major, int minor, Util::Reader reader, IPC::Channel& channel) {
        switch (major) {
            case QVM_COMMON:
                HandleCommonQVMSyscall(minor, reader, channel);
                break;

            case MISC:
                HandleMiscSyscall(minor, reader, channel);
                break;

            case COMMAND:
                HandleCommandSyscall(minor, reader, channel);
                break;

            case CVAR:
                HandleCvarSyscall(minor, reader, channel);
                break;

            case LOG:
                HandleLogSyscall(minor, reader, channel);
                break;

            case FILESYSTEM:
                FS::HandleFileSystemSyscall(minor, reader, channel, vmName);
                break;

            default:
                Sys::Drop("Unhandled common engine syscall major number %i", major);
        }
    }

    VMBase& CommonVMServices::GetVM() {
        return vm;
    }
}
