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

#include <algorithm>
#include <string>

#include "gameinfo.h"
#include "framework/CvarSystem.h"
#include "common/FileSystem.h"

Gameinfo& Gameinfo::getInstance()
{
	static Gameinfo instance;
	return instance;
}

void Gameinfo::parse(std::string fname)
{
	if (!FS::RawPath::FileExists(fname))
	{
		Sys::Error("GAMEINFO: No daemon.conf found in libpath");
	}
	FS::File f = FS::RawPath::OpenRead(fname);
	std::string content = f.ReadAll();
	f.Close();
	Cmd::Args a(content);
	if (a.Argc() & 1)
	{
		Sys::Error("GAMEINFO: Invalid daemon.conf");
	}
	std::vector<std::string> argvec = a.ArgVector();
	/* FIXME: We MUST split on lines and consider everything
	following a key to be the value (stripping leading and
	trailing spaces. Especially, game names may contain
	white space! It looks like the current implementation
	iterates every word! */
	for (int i = 0; i < argvec.size(); i += 2)
	{
		/* The key names and values MUST NOT be quoted
		in the daemon.conf */

		/* Display name, can contain special characters,
		like “Unvanquished”, “Smokin' Guns”, etc. */
		if(argvec[i] == "NAME")
		{
			_name = argvec[i+1];
		}
		/* Version string, usually in the form “0.52”, “1.2.3”, etc.

		This is not the engine version, this is the game version. */
		else if(argvec[i] == "VERSION")
		{
			_version = argvec[i+1];
		}
		/* Directory base name where user data will be stored on Windows.

		For example with “Unvanquished” it will store data in

		  C:\Users\<username>\My Games\Unvanquished

		It's common to find capitalized names and even white spaces,
		but you may prefer to avoid special characters. */
		else if(argvec[i] == "WINDOWSDIRNAME")
		{
			_windowsdirname = argvec[i+1];
		}
		/* Directory base name where user data will be stored on macOS.

		For example with “Unvanquished” it will store data in

		  /Users/<username>/Library/Application Support/Unvanquished

		It's common to find capitalized names and even white spaces,
		but you may prefer to avoid special characters.

		Some may prefer directory names in the form
		of “net.unvanquished.Unvanquished”, this is not unusual. */
		else if(argvec[i] == "MACOSDIRNAME")
		{
			_macosdirname = argvec[i+1];
		}
		/* Directory base name where user data will be stored on Linux
		and operating systems following freedesktop.org standards.

		For example with “unvanquished” it will store data in
		  /home/<username>/.local/share/unvanquished

		The name is usually lowercase without space and without
		special characters.

		Some may prefer directory names in the form
		of “net.unvanquished.Unvanquished”, this is rare. */
		else if(argvec[i] == "XDGDIRNAME")
		{
			_xdgdirname = argvec[i+1];
		}
		/* File base name for various files or directories written
		by the engine: screenshot base name, temporary file base name…

		For example with “unvanquished” it will name screenshot files
		the “unvanquished-<timestamp>.jpg way or create temporary files
		named like “/tmp/unvanquished-<random>”. */
		else if(argvec[i] == "BASENAME")
		{
			_basename = argvec[i+1];
		}
		/* Base name of the package the engine should look for to start the game.

		For example with “unvanquished” the engine will look for a package named
		like this:

		  unvanquished_<version>.dpk

		Packages base names are usually lowercase, don't contain white spaces
		a,d cannot use “_” characters except for separating the base name
		and the version string. */
		else if(argvec[i] == "BASEPAK")
		{
			_basepak = argvec[i+1];
		}
		/* Fully qualified domain name of the master server.

		Example: “master.unvanquished.net” */
		else if(argvec[i] == "MASTERSERVER1")
		{
			_masterserver1 = argvec[i+1];
			Cvar::SetDefaultValue("sv_master1", _masterserver1);
		}
		/* Fully qualified domain name of the secondary master server.

		Example: “master2.unvanquished.net” */
		else if(argvec[i] == "MASTERSERVER2")
		{
			_masterserver2 = argvec[i+1];
			Cvar::SetDefaultValue("sv_master2", _masterserver2);
		}
		/* Fully qualified domain name of the third master server. */
		else if(argvec[i] == "MASTERSERVER3")
		{
			_masterserver3 = argvec[i+1];
			Cvar::SetDefaultValue("sv_master3", _masterserver3);
		}
		/* Fully qualified domain name of the fourth master server. */
		else if(argvec[i] == "MASTERSERVER4")
		{
			_masterserver4 = argvec[i+1];
			Cvar::SetDefaultValue("sv_master4", _masterserver4);
		}
		/* Fully qualified domain name of the fifth master server. */
		else if(argvec[i] == "MASTERSERVER5")
		{
			_masterserver5 = argvec[i+1];
			Cvar::SetDefaultValue("sv_master5", _masterserver5);
		}
		/* URL to download missing packages when joining a server.

		Example: “dl.unvanquished.net/pkg”

		It is expected to be an http server running on TCP port 80.
		The protocol is ommitted. */
		else if(argvec[i] == "WWWBASEURL")
		{
			_wwwbaseurl = argvec[i+1];
		}
		/* A string used to identify against the master server.

		In case of total conversion mod, this is the string of the game host.

		Example: “UNVANQUISHED”

		It's usually upper case. */
		else if(argvec[i] == "MASTERGAMENAME")
		{
			_mastergamename = argvec[i+1];
		}
		/* A string used to filter games when listing servers from master
		servers.

		In case of total conversion mods, this is the string of the mod.

		Example: “unv” or “unvanquished”

		it's usually lower case. */
		else if(argvec[i] == "SERVERGAMENAME")
		{
			_servergamename = argvec[i+1];
		}
		/* A string used for game server urls.

		For example with “unv” you can have url in the form:

		  unv://unvanquished.net

		It's lower case and usually short but
		you can use something longer similar to
		the basename. */
		else if(argvec[i] == "URIPROTOCOL")
		{
			_uriprotocol = argvec[i+1];
		}
		else
		{
			Log::Warn("GAMEINFO: Unrecognized field '%s' in gameinfo.conf", argvec[i]);
		}
	}
}

std::string Gameinfo::name() const
{
	return _name;
}

std::string Gameinfo::name_lower() const
{
	std::string temp = _name;
	std::transform(temp.begin(), temp.end(), temp.begin(), ::tolower);
	return temp;
}

std::string Gameinfo::name_upper() const
{
	std::string temp = _name;
	std::transform(temp.begin(), temp.end(), temp.begin(), ::toupper);
	return temp;
}

std::string Gameinfo::version() const
{
	return _version;
}

std::string Gameinfo::unnamedservername() const
{
	return _name + " " + _version + " Server";
}

std::string Gameinfo::windowsdirname() const
{
	return _windowsdirname;
}

std::string Gameinfo::macosdirname() const
{
	return _macosdirname;
}

std::string Gameinfo::xdgdirname() const
{
	return _xdgdirname;
}

std::string Gameinfo::basename() const
{
	return _basename;
}

std::string Gameinfo::basepak() const
{
	return _basepak;
}

std::string Gameinfo::masterserver1() const
{
	return _masterserver1;
}

std::string Gameinfo::masterserver2() const
{
	return _masterserver2;
}

std::string Gameinfo::masterserver3() const
{
	return _masterserver3;
}

std::string Gameinfo::masterserver4() const
{
	return _masterserver4;
}

std::string Gameinfo::masterserver5() const
{
	return _masterserver5;
}

std::string Gameinfo::wwwbaseurl() const
{
	return _wwwbaseurl;
}

std::string Gameinfo::mastergamename() const
{
	return _mastergamename;
}

std::string Gameinfo::servergamename() const
{
	return _servergamename;
}

std::string Gameinfo::uriprotocol() const
{
	return _uriprotocol;
}
