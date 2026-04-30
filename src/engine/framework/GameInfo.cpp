/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2017-2026, Daemon Developers

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

#include <algorithm>
#include <string>

#include "framework/CvarSystem.h"
#include "framework/GameInfo.h"
#include "common/FileSystem.h"

// tomlcpp.hpp requires C++20.
#include "tomlc17/tomlc17.h"

GameInfo& GameInfo::getInstance()
{
	static GameInfo instance;
	return instance;
}

const std::string GameInfo::fileName = "daemon.ini";

inline void CheckTomlProperty(std::string name, toml_datum_t datum, toml_type_t type)
{
	if (datum.type != type)
	{
		Sys::Error("Missing or invalid “%s” property in “%s”.", name, GameInfo::fileName);
	}
}

void GameInfo::parse(std::string fname)
{
	if (!FS::RawPath::FileExists(fname))
	{
		Sys::Error("Missing “%s” in libpath.", GameInfo::fileName);
	}

	toml_result_t result = toml_parse_file_ex(fname.c_str());

	if (!result.ok)
	{
		Sys::Error("Failed to read “%s”.", GameInfo::fileName);
	}

	toml_datum_t toml_name = toml_seek(result.toptab, "gameinfo.name");
	CheckTomlProperty("name", toml_name, TOML_STRING);
	_name = toml_name.u.s;
	Cvar::SetDefaultValue("sv_hostname", "Unnamed " + _name + " Server");

	toml_datum_t toml_version = toml_seek(result.toptab, "gameinfo.version");
	CheckTomlProperty("version", toml_version, TOML_STRING);
	_version = toml_version.u.s;

	toml_datum_t toml_appId = toml_seek(result.toptab, "gameinfo.appId");
	CheckTomlProperty("appId", toml_appId, TOML_STRING);
	_appId = toml_appId.u.s;

	toml_datum_t toml_windowsDirName = toml_seek(result.toptab, "gameinfo.windowsDirName");
	CheckTomlProperty("windowsDirName", toml_windowsDirName, TOML_STRING);
	_windowsDirName = toml_windowsDirName.u.s;

	toml_datum_t toml_macosDirName = toml_seek(result.toptab, "gameinfo.macosDirName");
	CheckTomlProperty("macosDirName", toml_macosDirName, TOML_STRING);
	_macosDirName = toml_macosDirName.u.s;

	toml_datum_t toml_xdgDirName = toml_seek(result.toptab, "gameinfo.xdgDirName");
	CheckTomlProperty("xdgDirName", toml_xdgDirName, TOML_STRING);
	_xdgDirName = toml_xdgDirName.u.s;

	toml_datum_t toml_baseName = toml_seek(result.toptab, "gameinfo.baseName");
	CheckTomlProperty("baseName", toml_baseName, TOML_STRING);
	_baseName = toml_baseName.u.s;

	toml_datum_t toml_basePak = toml_seek(result.toptab, "gameinfo.basePak");
	CheckTomlProperty("basePak", toml_basePak, TOML_STRING);
	_basePak = toml_basePak.u.s;
	Cvar::SetDefaultValue("fs_basepak", _basePak);

	toml_datum_t toml_masterServers = toml_seek(result.toptab, "gameinfo.masterServers");
	CheckTomlProperty("masterServers", toml_masterServers, TOML_ARRAY);

	for (int i = 0; i < toml_masterServers.u.arr.size; i++)
	{
		if ( i > MAX_MASTER_SERVERS )
		{
			Log::Warn("Only %d masters server are supported.", MAX_MASTER_SERVERS);
			break;
		}

		toml_datum_t toml_masterServer = toml_masterServers.u.arr.elem[i];
		CheckTomlProperty(va("masterServers[%d]", i), toml_masterServer, TOML_STRING);
		Cvar::SetDefaultValue(va("sv_master%d", i + 1), toml_masterServer.u.s);
	}

	toml_datum_t toml_wwwBaseUrl = toml_seek(result.toptab, "gameinfo.wwwBaseUrl");
	CheckTomlProperty("wwwBaseUrl", toml_wwwBaseUrl, TOML_STRING);
	_wwwBaseUrl = toml_wwwBaseUrl.u.s;

	toml_datum_t toml_masterGameName = toml_seek(result.toptab, "gameinfo.masterGameName");
	CheckTomlProperty("masterGameName", toml_masterGameName, TOML_STRING);
	_masterGameName = toml_masterGameName.u.s;

	toml_datum_t toml_serverGameName = toml_seek(result.toptab, "gameinfo.serverGameName");
	CheckTomlProperty("serverGameName", toml_serverGameName, TOML_STRING);
	_serverGameName = toml_serverGameName.u.s;

	toml_datum_t toml_uriProtocol = toml_seek(result.toptab, "gameinfo.uriProtocol");
	CheckTomlProperty("uriProtocol", toml_uriProtocol, TOML_STRING);
	_uriProtocol = toml_uriProtocol.u.s;

	toml_free(result);
}

std::string GameInfo::name() const
{
	return _name;
}

std::string GameInfo::name_lower() const
{
	std::string temp = _name;
	std::transform(temp.begin(), temp.end(), temp.begin(), ::tolower);
	return temp;
}

std::string GameInfo::name_upper() const
{
	std::string temp = _name;
	std::transform(temp.begin(), temp.end(), temp.begin(), ::toupper);
	return temp;
}

std::string GameInfo::version() const
{
	return _version;
}

std::string GameInfo::appId() const
{
	return _appId;
}

std::string GameInfo::windowsDirName() const
{
	return _windowsDirName;
}

std::string GameInfo::macosDirName() const
{
	return _macosDirName;
}

std::string GameInfo::xdgDirName() const
{
	return _xdgDirName;
}

std::string GameInfo::baseName() const
{
	return _baseName;
}

std::string GameInfo::basePak() const
{
	return _basePak;
}

std::string GameInfo::masterServer1() const
{
	return _masterServer1;
}

std::string GameInfo::masterServer2() const
{
	return _masterServer2;
}

std::string GameInfo::masterServer3() const
{
	return _masterServer3;
}

std::string GameInfo::masterServer4() const
{
	return _masterServer4;
}

std::string GameInfo::masterServer5() const
{
	return _masterServer5;
}

std::string GameInfo::wwwBaseUrl() const
{
	return _wwwBaseUrl;
}

std::string GameInfo::masterGameName() const
{
	return _masterGameName;
}

std::string GameInfo::serverGameName() const
{
	return _serverGameName;
}

std::string GameInfo::uriProtocol() const
{
	return _uriProtocol;
}
