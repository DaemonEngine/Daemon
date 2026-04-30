/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2017-26, Daemon Developers

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

#ifndef GAMEINFO_H_
#define GAMEINFO_H_

#include <iostream>


class GameInfo
{
public:
	static GameInfo& getInstance();
	void parse(std::string fname);
	static const std::string fileName;
	std::string name() const;
	std::string name_lower() const;
	std::string name_upper() const;
	std::string version() const;
	std::string appId() const;
	std::string windowsDirName() const;
	std::string macosDirName() const;
	std::string xdgDirName() const;
	std::string baseName() const;
	std::string basePak() const;
	std::string masterServer1() const;
	std::string masterServer2() const;
	std::string masterServer3() const;
	std::string masterServer4() const;
	std::string masterServer5() const;
	std::string wwwBaseUrl() const;
	std::string masterGameName() const;
	std::string serverGameName() const;
	std::string uriProtocol() const;
private:
	GameInfo() : _name("MISSING_NAME"), _version("UNKNOWN") {};
	std::string _name;
	std::string _version;
	std::string _appId;
	std::string _windowsDirName;
	std::string _macosDirName;
	std::string _xdgDirName;
	std::string _baseName;
	std::string _basePak;
	std::string _masterServer1;
	std::string _masterServer2;
	std::string _masterServer3;
	std::string _masterServer4;
	std::string _masterServer5;
	std::string _wwwBaseUrl;
	std::string _masterGameName;
	std::string _serverGameName;
	std::string _uriProtocol;
};

#endif // GAMEINFO_H_
