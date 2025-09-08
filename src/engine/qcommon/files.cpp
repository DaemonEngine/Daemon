/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2013 Unvanquished Developers

This file is part of the Daemon GPL Source Code (Daemon Source Code).

Daemon Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon Source Code.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#include <common/FileSystem.h>
#include "q_shared.h"
#include "qcommon.h"
#include "common/Defs.h"

extern Log::Logger fsLogs;

// There must be some limit for the APIs in this file since they use 'int' for lengths which can be overflowed by large files.
constexpr FS::offset_t MAX_FILE_LENGTH = 1000 * 1000 * 1000;

// Compatibility wrapper for the filesystem
const char TEMP_SUFFIX[] = ".tmp";

// Cvars to select the base and extra packages to use
static Cvar::Cvar<std::string> fs_basepak("fs_basepak", "base pak to load", 0, DEFAULT_BASE_PAK);
static Cvar::Cvar<std::string> fs_extrapaks("fs_extrapaks", "space-seperated list of paks to load in addition to the base pak", 0, "");

struct handleData_t {
	bool isOpen;
	bool isPakFile;
	Util::optional<std::string> renameTo;
	FS::Owner owner;

	// Normal file info
	bool forceFlush;
	FS::File file;

	// Pak file info
	std::string fileData;
	size_t filePos;
};
struct missingPak_t {
	std::string name;
	std::string version;
	uint32_t checksum;
};

static const int MAX_FILE_HANDLES = 64;
static handleData_t handleTable[MAX_FILE_HANDLES];
std::vector<missingPak_t> fs_missingPaks;

static Cvar::Cvar<bool> allowRemotePakDir("client.allowRemotePakDir", "Connect to servers that load game data from directories", Cvar::TEMPORARY, false);

static fileHandle_t FS_AllocHandle()
{
	// Don't use handle 0 because it is used to indicate failures
	for (int i = 1; i < MAX_FILE_HANDLES; i++) {
		if (!handleTable[i].isOpen) {
			handleTable[i].owner = FS::Owner::ENGINE;
			return i;
		}
	}
	Sys::Drop("FS_AllocHandle: none free");
}

static void FS_CheckHandle(fileHandle_t handle, bool write)
{
	if (handle < 0 || handle >= MAX_FILE_HANDLES)
		Sys::Drop("FS_CheckHandle: invalid handle");
	if (!handleTable[handle].isOpen)
		Sys::Drop("FS_CheckHandle: closed handle");
	if (write && handleTable[handle].isPakFile)
		Sys::Drop("FS_CheckHandle: writing to file in pak");
}

int FS_FOpenFileRead(const char* path, fileHandle_t* handle)
{
	if (!handle)
		return FS::PakPath::FileExists(path) || FS::HomePath::FileExists(path);

	*handle = FS_AllocHandle();
	int length = -1;
	std::error_code err;
	if (FS::PakPath::FileExists(path)) {
		handleTable[*handle].fileData = FS::PakPath::ReadFile(path, err);
		if (!err) {
			handleTable[*handle].filePos = 0;
			handleTable[*handle].isPakFile = true;
			handleTable[*handle].isOpen = true;
			length = handleTable[*handle].fileData.size();
		}
	} else {
		handleTable[*handle].file = FS::HomePath::OpenRead(path, err);
		if (!err) {
			length = handleTable[*handle].file.Length();
			handleTable[*handle].isPakFile = false;
			handleTable[*handle].isOpen = true;
		}
	}
	if (err) {
		Log::Debug("Failed to open '%s' for reading: %s", path, err.message());
		*handle = 0;
		length = -1;
	} else if (length > MAX_FILE_LENGTH) {
		Log::Warn("FS_FOpenFileRead: Failed to open '%s' for reading: size %d is too large", path, length);
		FS_FCloseFile(*handle);
		*handle = 0;
		length = -1;
	}
	return length;
}

static fileHandle_t FS_FOpenFileWrite_internal(const char* path, bool temporary)
{
	fileHandle_t handle = FS_AllocHandle();
	try {
		handleTable[handle].file = FS::HomePath::OpenWrite(temporary ? std::string(path) + TEMP_SUFFIX : path);
	} catch (std::system_error& err) {
		Log::Notice("Failed to open '%s' for writing: %s", path, err.what());
		return 0;
	}
	handleTable[handle].forceFlush = false;
	handleTable[handle].isPakFile = false;
	handleTable[handle].isOpen = true;
	if (temporary) {
		handleTable[handle].renameTo = FS::Path::Build(FS::GetHomePath(), path);
	}
	return handle;
}

fileHandle_t FS_FOpenFileWrite(const char* path)
{
	return FS_FOpenFileWrite_internal(path, false);
}

fileHandle_t FS_FOpenFileWriteViaTemporary(const char* path)
{
	return FS_FOpenFileWrite_internal(path, true);
}

fileHandle_t FS_FOpenFileAppend(const char* path)
{
	fileHandle_t handle = FS_AllocHandle();
	try {
		handleTable[handle].file = FS::HomePath::OpenAppend(path);
	} catch (std::system_error& err) {
		Log::Notice("Failed to open '%s' for appending: %s", path, err.what());
		return 0;
	}
	handleTable[handle].forceFlush = false;
	handleTable[handle].isPakFile = false;
	handleTable[handle].isOpen = true;
	return handle;
}

fileHandle_t FS_SV_FOpenFileWrite(const char* path)
{
	return FS_FOpenFileWrite_internal(path, false);
}

static int FS_SV_FOpenFileRead(const char* path, fileHandle_t* handle)
{
	if (!handle)
		return FS::HomePath::FileExists(path);

	*handle = FS_AllocHandle();
	std::error_code err;
	handleTable[*handle].file = FS::HomePath::OpenRead(path, err);
	if (err) {
		Log::Debug("Failed to open '%s' for reading: %s", path, err.message().c_str());
		*handle = 0;
		return -1;
	}
	handleTable[*handle].isPakFile = false;
	handleTable[*handle].isOpen = true;
	FS::offset_t length = handleTable[*handle].file.Length();
	if (length > MAX_FILE_LENGTH) {
		Log::Warn("FS_SV_FOpenFileRead: Failed to open '%s' for reading: size %d is too large", path, length);
		FS_FCloseFile(*handle);
		*handle = 0;
		return -1;
	}
	return length;
}

// Opens a file in <homepath>/game/
int FS_Game_FOpenFileByMode(const char* path, fileHandle_t* handle, fsMode_t mode)
{
	std::string path2 = FS::Path::Build("game", path);
	switch (mode) {
	case fsMode_t::FS_READ:
	{
		int size = FS_SV_FOpenFileRead(path2.c_str(), handle);
		return (!handle || *handle) ? size : -1;
	}

	case fsMode_t::FS_WRITE:
	case fsMode_t::FS_WRITE_VIA_TEMPORARY:
		*handle = FS_FOpenFileWrite_internal(path2.c_str(), mode == fsMode_t::FS_WRITE_VIA_TEMPORARY);
		return *handle == 0 ? -1 : 0;

	case fsMode_t::FS_APPEND:
	case fsMode_t::FS_APPEND_SYNC:
		*handle = FS_FOpenFileAppend(path2.c_str());
		handleTable[*handle].forceFlush = mode == fsMode_t::FS_APPEND_SYNC;
		return *handle == 0 ? -1 : 0;
	}
	Sys::Drop("FS_Game_FOpenFileByMode: bad mode %s", Util::enum_str(mode));
}

// Set a VM as the owner
void FS_SetOwner(fileHandle_t f, FS::Owner owner)
{
	FS_CheckHandle(f, false);
	ASSERT_EQ(handleTable[f].owner, FS::Owner::ENGINE);
	ASSERT_NQ(owner, FS::Owner::ENGINE);
	handleTable[f].owner = owner;
}

void FS_CheckOwnership(fileHandle_t f, FS::Owner owner)
{
	if (f == 0)
		return;
	FS_CheckHandle(f, false);
	if (handleTable[f].owner != owner)
		Sys::Drop("VM %d tried to access file handle it doesn't own", Util::ordinal(owner));
}

void FS_CloseAllForOwner(FS::Owner owner)
{
	int numClosed = 0;
	for (int f = 1; f < MAX_FILE_HANDLES; f++) {
		if (handleTable[f].owner == owner && handleTable[f].isOpen) {
			if (handleTable[f].renameTo) {
				// Delete the temp file without renaming
				std::error_code err;
				handleTable[f].file.Close(err); // ignore error
				FS_Delete((*handleTable[f].renameTo + TEMP_SUFFIX).c_str());
			} else {
				FS_FCloseFile(f);
			}
			++numClosed;
		}
	}
	fsLogs.Verbose("Closed %d outstanding handles for owner %d", numClosed, Util::ordinal(owner));
}

int FS_FCloseFile(fileHandle_t handle)
{
	if (handle == 0)
		return 0;
	FS_CheckHandle(handle, false);
	handleTable[handle].isOpen = false;
	if (handleTable[handle].isPakFile) {
		handleTable[handle].fileData.clear();
		handleTable[handle].fileData.shrink_to_fit();
		return 0;
	} else {
		try {
			handleTable[handle].file.Close();
			if (handleTable[handle].renameTo) {
				std::string renameTo = std::move(*handleTable[handle].renameTo);
				handleTable[handle].renameTo = Util::nullopt; // tidy up after abusing std::move
				try {
					FS::RawPath::MoveFile(renameTo, renameTo + TEMP_SUFFIX);
				} catch (std::system_error& err) {
					Log::Notice("Failed to replace file %s: %s", renameTo.c_str(), err.what());
					return -1;
				}
			}
			return 0;
		} catch (std::system_error& err) {
			Log::Notice("Failed to close file: %s", err.what());
			return -1;
		}
	}
}

int FS_filelength(fileHandle_t handle)
{
	FS_CheckHandle(handle, false);
	if (handleTable[handle].isPakFile)
		return handleTable[handle].fileData.size();
	else {
		std::error_code err;
		int length = handleTable[handle].file.Length(err);
		if (err) {
			Log::Notice("Failed to get file length: %s", err.message().c_str());
			return 0;
		}
		return length;
	}
}

int FS_FTell(fileHandle_t handle)
{
	FS_CheckHandle(handle, false);
	if (handleTable[handle].isPakFile)
		return handleTable[handle].filePos;
	else
		return handleTable[handle].file.Tell();
}

int FS_Seek(fileHandle_t handle, long offset, fsOrigin_t origin)
{
	FS_CheckHandle(handle, false);
	if (handleTable[handle].isPakFile) {
		switch (origin) {
			case fsOrigin_t::FS_SEEK_CUR:
			handleTable[handle].filePos += offset;
			break;

		case fsOrigin_t::FS_SEEK_SET:
			handleTable[handle].filePos = offset;
			break;

		case fsOrigin_t::FS_SEEK_END:
			handleTable[handle].filePos = handleTable[handle].fileData.size() + offset;
			break;

		default:
			Sys::Drop("Bad origin in FS_Seek");
		}
		return 0;
	} else {
		try {
			switch (origin) {
			case fsOrigin_t::FS_SEEK_CUR:
				handleTable[handle].file.SeekCur(offset);
				break;

			case fsOrigin_t::FS_SEEK_SET:
				handleTable[handle].file.SeekSet(offset);
				break;

			case fsOrigin_t::FS_SEEK_END:
				handleTable[handle].file.SeekEnd(offset);
				break;

			default:
				Sys::Drop("Bad origin in FS_Seek");
			}
			return 0;
		} catch (std::system_error& err) {
			Log::Notice("FS_Seek failed: %s", err.what());
			return -1;
		}
	}
}

void FS_ForceFlush(fileHandle_t handle)
{
	FS_CheckHandle(handle, true);
	handleTable[handle].forceFlush = true;
}
void FS_Flush(fileHandle_t handle)
{
	FS_CheckHandle(handle, true);
	try {
		handleTable[handle].file.Flush();
	} catch (std::system_error& err) {
		Log::Notice("FS_Flush failed: %s", err.what());
	}
}

int FS_Write(const void* buffer, int len, fileHandle_t handle)
{
	FS_CheckHandle(handle, true);
	try {
		handleTable[handle].file.Write(buffer, len);
		if (handleTable[handle].forceFlush)
			handleTable[handle].file.Flush();
		return len;
	} catch (std::system_error& err) {
		Log::Notice("FS_Write failed: %s", err.what());
		return 0;
	}
}

int FS_Read(void* buffer, int len, fileHandle_t handle)
{
	FS_CheckHandle(handle, false);
	if (len < 0)
		Sys::Drop("FS_Read: invalid length");
	if (handleTable[handle].isPakFile) {
		if (!len)
			return 0;
		if (handleTable[handle].filePos >= handleTable[handle].fileData.size())
			return 0;
		len = std::min<size_t>(len, handleTable[handle].fileData.size() - handleTable[handle].filePos);
		memcpy(buffer, handleTable[handle].fileData.data() + handleTable[handle].filePos, len);
		handleTable[handle].filePos += len;
		return len;
	} else {
		try {
			return handleTable[handle].file.Read(buffer, len);
		} catch (std::system_error& err) {
			Log::Notice("FS_Read failed: %s", err.what());
			return 0;
		}
	}
}

void FS_Printf(fileHandle_t handle, const char* fmt, ...)
{
	va_list ap;
	char buffer[MAXPRINTMSG];

	va_start(ap, fmt);
	Q_vsnprintf(buffer, sizeof(buffer), fmt, ap);
	va_end(ap);

	FS_Write(buffer, strlen(buffer), handle);
}

int FS_Delete(const char* path)
{
	try {
		FS::HomePath::DeleteFile(path);
	} catch (std::system_error& err) {
		Log::Notice("Failed to delete file '%s': %s", path, err.what());
	}
	return 0;
}

void FS_SV_Rename(const char* from, const char* to)
{
	try {
		FS::HomePath::MoveFile(to, from);
	} catch (std::system_error& err) {
		Log::Notice("Failed to move '%s' to '%s': %s", from, to, err.what());
	}
}

void FS_WriteFile(const char* path, const void* buffer, int size)
{
	try {
		FS::File f = FS::HomePath::OpenWrite(path);
		f.Write(buffer, size);
		f.Close();
	} catch (std::system_error& err) {
		Log::Notice("Failed to write file '%s': %s", path, err.what());
	}
}

int FS_ReadFile(const char* path, void** buffer)
{
	fileHandle_t handle;
	int length = FS_FOpenFileRead(path, &handle);

	if (length < 0) {
		if (buffer)
			*buffer = nullptr;
		return -1;
	}

	if (buffer) {
		char* buf = new char[length + 1];
		*buffer = buf;
		FS_Read(buf, length, handle);
		buf[length] = '\0';
	}

	FS_FCloseFile(handle);
	return length;
}

void FS_FreeFile(void* buffer)
{
	char* buf = static_cast<char*>(buffer);
	delete[] buf;
}

char** FS_ListFiles(const char* directory, const char* extension, int* numFiles)
{
	std::vector<char*> files;
	bool dirsOnly = extension && !strcmp(extension, "/");

	try {
		for (const std::string& x: FS::PakPath::ListFiles(directory)) {
			if (extension && !Str::IsSuffix(extension, x))
				continue;
			if (dirsOnly != (x.back() == '/'))
				continue;
			char* s = new char[x.size() + 1];
			memcpy(s, x.data(), x.size());
			s[x.size() - (x.back() == '/')] = '\0';
			files.push_back(s);
		}
	} catch (std::system_error&) {}
	try {
		for (const std::string& x: FS::HomePath::ListFiles(directory)) {
			if (extension && !Str::IsSuffix(extension, x))
				continue;
			if (dirsOnly != (x.back() == '/'))
				continue;
			char* s = new char[x.size() + 1];
			memcpy(s, x.data(), x.size());
			s[x.size() - (x.back() == '/')] = '\0';
			files.push_back(s);
		}
	} catch (std::system_error&) {}

	*numFiles = files.size();
	char** list = new char*[files.size() + 1];
	std::copy(files.begin(), files.end(), list);
	list[files.size()] = nullptr;
	return list;
}

void FS_FreeFileList(char** list)
{
	if (!list)
		return;
	for (char** i = list; *i; i++)
		delete[] *i;
	delete[] list;
}

int FS_GetFileList(const char* path, const char* extension, char* listBuf, int bufSize)
{
	// Mods are not yet supported in the new filesystem
	if (!strcmp(path, "$modlist"))
		return 0;

	int numFiles = 0;
	bool dirsOnly = extension && !strcmp(extension, "/");

	try {
		for (const std::string& x: FS::PakPath::ListFiles(path)) {
			if (extension && !Str::IsSuffix(extension, x))
				continue;
			if (dirsOnly != (x.back() == '/'))
				continue;
			int length = x.size() + (x.back() != '/');
			if (bufSize < length)
				return numFiles;
			memcpy(listBuf, x.c_str(), length);
			listBuf[length - 1] = '\0';
			listBuf += length;
			bufSize -= length;
			numFiles++;
		}
	} catch (std::system_error&) {}
	try {
		for (const std::string& x: FS::HomePath::ListFiles(FS::Path::Build("game", path))) {
			if (extension && !Str::IsSuffix(extension, x))
				continue;
			if (dirsOnly != (x.back() == '/'))
				continue;
			int length = x.size() + (x.back() != '/');
			if (bufSize < length)
				return numFiles;
			memcpy(listBuf, x.c_str(), length);
			listBuf[length - 1] = '\0';
			listBuf += length;
			bufSize -= length;
			numFiles++;
		}
	} catch (std::system_error&) {}

	return numFiles;
}

int FS_GetFileListRecursive(const char* path, const char* extension, char* listBuf, int bufSize)
{
	// Mods are not yet supported in the new filesystem
	if (!strcmp(path, "$modlist"))
		return 0;

	int numFiles = 0;
	bool dirsOnly = extension && !strcmp(extension, "/");

	try {
		for (const std::string& x: FS::PakPath::ListFilesRecursive(path)) {
			if (extension && !Str::IsSuffix(extension, x))
				continue;
			if (dirsOnly != (x.back() == '/'))
				continue;
			int length = x.size() + (x.back() != '/');
			if (bufSize < length)
				return numFiles;
			memcpy(listBuf, x.c_str(), length);
			listBuf[length - 1] = '\0';
			listBuf += length;
			bufSize -= length;
			numFiles++;
		}
	} catch (std::system_error&) {}
	try {
		for (const std::string& x: FS::HomePath::ListFiles(FS::Path::Build("game", path))) {
			if (extension && !Str::IsSuffix(extension, x))
				continue;
			if (dirsOnly != (x.back() == '/'))
				continue;
			int length = x.size() + (x.back() != '/');
			if (bufSize < length)
				return numFiles;
			memcpy(listBuf, x.c_str(), length);
			listBuf[length - 1] = '\0';
			listBuf += length;
			bufSize -= length;
			numFiles++;
		}
	} catch (std::system_error&) {}

	return numFiles;
}

const char* FS_LoadedPaks()
{
	static char info[BIG_INFO_STRING];
	info[0] = '\0';
	for (const FS::LoadedPakInfo& x: FS::PakPath::GetLoadedPaks()) {
		if (!x.pathPrefix.empty())
			continue;
		if (info[0])
			Q_strcat(info, sizeof(info), " ");
		Q_strcat(info, sizeof(info), FS::MakePakName(x.name, x.version, x.realChecksum).c_str());
	}
	return info;
}

bool FS_LoadPak(const Str::StringRef name)
{
	const FS::PakInfo* pak = FS::FindPak(name);

	if (!pak) {
		Log::Warn("Pak not found: '%s'", name);
		return false;
	}

	try {
		FS::PakPath::LoadPak(*pak);
		return true;
	} catch (std::system_error& err) {
		Log::Warn("Failed to load pak '%s': %s", name, err.what());
		return false;
	}
}

void FS_LoadBasePak()
{
	Cmd::Args extrapaks(fs_extrapaks.Get());
	for (auto& x: extrapaks) {
		if (!FS_LoadPak(x)) {

			Sys::Error("Could not load extra pak '%s'", x.c_str());
		}
	}

	if (FS_LoadPak(fs_basepak.Get())) {
		return; // success
	}

	if (fs_basepak.Get() != DEFAULT_BASE_PAK) {
		Log::Notice("Could not load base pak '%s', falling back to default: '%s'", fs_basepak.Get().c_str(), DEFAULT_BASE_PAK);
		if (FS_LoadPak(DEFAULT_BASE_PAK)) {
			return; // success
		}
	}

	Sys::Error("Could not load default base pak '%s'", DEFAULT_BASE_PAK);
}

bool FS_LoadServerPaks(const char* paks, bool isDemo)
{
	Cmd::Args args(paks);
	fs_missingPaks.clear();
	for (auto& x: args) {
		std::string name, version;
		Util::optional<uint32_t> checksum;

		if (!FS::ParsePakName(x.data(), x.data() + x.size(), name, version, checksum)) {
			Sys::Drop("Invalid pak reference from server: %s", x.c_str());
		} else if (!version.empty() && !checksum) {
			if (isDemo || allowRemotePakDir.Get()) {
				const FS::PakInfo* pak = FS::FindPak(name, version);
				if (!pak) {
					Sys::Drop("Pak %s version %s not found", name, version);
				}
				try {
					FS::PakPath::LoadPakExplicitWithoutChecksum(*pak); // FIXME bogus checksum argument
				} catch (const std::system_error& e) {
					Sys::Drop("Failed to load pak %s version %s: %s", name, version, e.what());
				}
				continue;
			}
			// non-legacy paks (with non empty version) must have a checksum
			Sys::Drop("The server is configured to load game data from a directory which makes it incompatible with remote clients.");
		}

		// Keep track of all missing paks
		const FS::PakInfo* pak = FS::FindPak(name, version, *checksum);
		if (!pak)
			fs_missingPaks.push_back({std::move(name), std::move(version), *checksum});
		else {
			try {
				FS::PakPath::LoadPakExplicit(*pak, *checksum);
			} catch (std::system_error&) {
				fs_missingPaks.push_back({std::move(name), std::move(version), *checksum});
			}
		}
	}

	// Load extra paks as well for demos
	if (isDemo) {
		Cmd::Args extrapaks(fs_extrapaks.Get());
		for (auto& x: extrapaks) {
			if (!FS_LoadPak(x))
				Sys::Error("Could not load extra pak '%s'", x.c_str());
		}
	}

	return fs_missingPaks.empty();
}

#ifndef BUILD_SERVER

bool CL_WWWBadChecksum(const char *pakname);
void FS_DeletePaksWithBadChecksum() {
	for (const missingPak_t& x: fs_missingPaks) {
		if (FS::FindPak(x.name, x.version)) {
			if (CL_WWWBadChecksum(FS::MakePakName(x.name, x.version, x.checksum).c_str())) {
				std::string filename = Str::Format("pkg/%s", FS::MakePakName(x.name, x.version));
				try {
					FS::HomePath::DeleteFile(filename);
				} catch (const std::system_error& e) {
					Sys::Drop("FS_DeletePaksWithBadChecksum: couldn't delete %s: %s", filename, e.what());
				}
			}
		}
	}
}

bool FS_ComparePaks(char* neededpaks, int len)
{
	*neededpaks = '\0';
	for (const missingPak_t& x: fs_missingPaks) {
		Q_strcat(neededpaks, len, "@");
		Q_strcat(neededpaks, len, FS::MakePakName(x.name, x.version, x.checksum).c_str());
		Q_strcat(neededpaks, len, "@");
		std::string pakName = Str::Format("pkg/%s", FS::MakePakName(x.name, x.version));
		if (FS::HomePath::FileExists(pakName))
			Q_strcat(neededpaks, len, Str::Format("pkg/%s", FS::MakePakName(x.name, x.version, x.checksum)).c_str());
		else
			Q_strcat(neededpaks, len, pakName.c_str());
	}
	return !fs_missingPaks.empty();
}
#endif // !BUILD_SERVER

class WhichCmd: public Cmd::StaticCmd {
public:
	WhichCmd()
		: Cmd::StaticCmd("which", Cmd::BASE, "shows which pak a file is in") {}

	void Run(const Cmd::Args& args) const override
	{
		if (args.Argc() != 2) {
			PrintUsage(args, "<file>", "");
			return;
		}

		const std::string& filename = args.Argv(1);
		const FS::LoadedPakInfo* pak = FS::PakPath::LocateFile(filename);
		if (pak)
			Print( "File \"%s\" found in \"%s\"", filename, pak->path);
		else
			Print("File not found: \"%s\"", filename);
	}

	Cmd::CompletionResult Complete(int argNum, const Cmd::Args&, Str::StringRef prefix) const override
	{
		if (argNum == 1) {
			return FS::PakPath::CompleteFilename(prefix, "", "", true, false);
		}

		return {};
	}
};
static WhichCmd WhichCmdRegistration;

class ListPathsCmd: public Cmd::StaticCmd {
public:
	ListPathsCmd()
		: Cmd::StaticCmd("listPaths", Cmd::BASE, "list filesystem search paths") {}

	void Run(const Cmd::Args&) const override
	{
		Print("Home path: %s", FS::GetHomePath());
		for (auto& x: FS::PakPath::GetLoadedPaks())
			Print("Loaded pak: %s", x.path);
	}
};
static ListPathsCmd ListPathsCmdRegistration;

class DirCmd: public Cmd::StaticCmd {
public:
	DirCmd(): Cmd::StaticCmd("dir", Cmd::BASE, "list all files in a given directory with the option to pass a filter") {}

	void Run(const Cmd::Args& args) const override
	{
		bool filter = false;
		if (args.Argc() != 2 && args.Argc() != 3) {
			PrintUsage(args, "<path> [filter]", "");
			return;
		}

		if ( args.Argc() == 3) {
			filter = true;
		}

		Print("In Paks:");
		Print("--------");
		try {
			for (auto& filename : FS::PakPath::ListFiles(args.Argv(1))) {
				if (filename.size() && (!filter || Com_Filter(args.Argv(2).c_str(), filename.c_str(), false))) {
					Print(filename.c_str());
				}
			}
		} catch (std::system_error&) {
			Print("^1ERROR^*: Path does not exist");
		}

		Print("\n");
		Print("In Homepath");
		Print("-----------");
		try {
			for (auto& filename : FS::RawPath::ListFiles(FS::Path::Build(FS::GetHomePath(),args.Argv(1)))) {
				if (filename.size() && (!filter || Com_Filter(args.Argv(2).c_str(), filename.c_str(), false))) {
					Print(filename.c_str());
				}
			}
		} catch (std::system_error&) {
			Print("^1ERROR^*: Path does not exist");
		}

	}
};
static DirCmd DirCmdRegistration;
