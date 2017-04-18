/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2017, Daemon Developers

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


class Gameinfo
{
public:
	static Gameinfo& getInstance();
	void parse(std::string fname);
	std::string name();
	std::string name_lower();
	std::string name_upper();
	std::string version();
	std::string default_basepak();
	std::string masterserver1();
	std::string masterserver2();
	std::string masterserver3();
	std::string masterserver4();
	std::string masterserver5();
	std::string www_baseurl();
	std::string unnamed_server();
	std::string gamename_string();
private:
	Gameinfo() : _name("MISSING_NAME"), _version("UNKNOWN") {};
	std::string _name;
	std::string _version;
	std::string _default_basepak;
	std::string _masterserver1;
	std::string _masterserver2;
	std::string _masterserver3;
	std::string _masterserver4;
	std::string _masterserver5;
	std::string _www_baseurl;
	std::string _gamename_string;
};

#endif // GAMEINFO_H_
