/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

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

In addition, the Daemon Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Daemon
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/

#include "common/Common.h"

#include "client.h"

Log::Logger downloadLogger("client.pakDownload", "", Log::Level::NOTICE);
Cvar::Cvar<int> cl_downloadCount("cl_downloadCount", "bytes of a file downloaded", Cvar::NONE, 0);

/*
=====================
CL_ClearStaticDownload
Clear download information that we keep in cls (disconnected download support)
=====================
*/
void CL_ClearStaticDownload()
{
	downloadLogger.Debug("Clearing the download info");
	cls.downloadRestart = false;
	cls.downloadTempName[ 0 ] = '\0';
	cls.downloadName[ 0 ] = '\0';
	cls.originalDownloadName[ 0 ] = '\0';
}

/*
=================
CL_DownloadsComplete

Called when all downloading has been completed
=================
*/
static void CL_DownloadsComplete()
{

	// if we downloaded files we need to restart the file system
	if ( cls.downloadRestart )
	{
		cls.downloadRestart = false;

		downloadLogger.Debug("Downloaded something, reload the paks");
		downloadLogger.Debug(" The paks to load are '%s'", Cvar_VariableString("sv_paks"));

		FS::PakPath::ClearPaks();
		FS_LoadServerPaks(Cvar_VariableString("sv_paks"), clc.demoplaying); // We possibly downloaded a pak, restart the file system to load it

		// inform the server so we get new gamestate info
		CL_AddReliableCommand( "donedl" );

		// we can reset that now
		CL_ClearStaticDownload();

		// by sending the donedl command we request a new gamestate
		// so we don't want to load stuff yet
		return;
	}

	// let the client game init and load data
	cls.state = connstate_t::CA_LOADING;

	// Pump the loop, this may change gamestate!
	Com_EventLoop();

	// if the gamestate was changed by calling Com_EventLoop
	// then we loaded everything already and we don't want to do it again.
	if ( cls.state != connstate_t::CA_LOADING )
	{
		return;
	}

	// flush client memory and start loading stuff
	// this will also (re)load the UI
	CL_ShutdownAll();
	CL_StartHunkUsers();

	// initialize the CGame
	cls.cgameStarted = true;
	CL_InitCGame();

	CL_WritePacket();
	CL_WritePacket();
	CL_WritePacket();
}

/*
=================
CL_BeginDownload

Requests a file to download from the server.  Stores it in the current
game directory.
=================
*/
static void CL_BeginDownload( const char *localName, const char *remoteName )
{
	downloadLogger.Debug("Requesting the download of '%s', with remote name '%s'", localName, remoteName);

	Q_strncpyz( cls.downloadName, localName, sizeof( cls.downloadName ) );
	Com_sprintf( cls.downloadTempName, sizeof( cls.downloadTempName ), "%s.tmp", localName );

	// Set so UI gets access to it
	Cvar_Set( "cl_downloadName", remoteName );
	Cvar_Set( "cl_downloadSize", "0" );
	cl_downloadCount.Set(0);
	Cvar_SetValue( "cl_downloadTime", cls.realtime );

	clc.downloadBlock = 0; // Starting new file
	clc.downloadCount = 0;

	CL_AddReliableCommand( va( "download %s", Cmd_QuoteString( remoteName ) ) );
}

/*
=================
CL_NextDownload

A download completed or failed
=================
*/
static void CL_NextDownload()
{
	char *s;
	char *remoteName, *localName;

	// We are looking to start a download here
	if ( *clc.downloadList )
	{
		downloadLogger.Debug("CL_NextDownload downloadList is '%s'", clc.downloadList);
		s = clc.downloadList;

		// format is:
		//  @remotename@localname@remotename@localname, etc.

		if ( *s == '@' )
		{
			s++;
		}

		remoteName = s;

		if ( ( s = strchr( s, '@' ) ) == nullptr )
		{
			CL_DownloadsComplete();
			return;
		}

		*s++ = 0;
		localName = s;

		if ( ( s = strchr( s, '@' ) ) != nullptr )
		{
			*s++ = 0;
		}
		else
		{
			s = localName + strlen( localName );  // point at the nul byte
		}

		CL_BeginDownload( localName, remoteName );

		cls.downloadRestart = true;

		// move over the rest
		memmove( clc.downloadList, s, strlen( s ) + 1 );

		return;
	}

	CL_DownloadsComplete();
}

/*
=================
CL_InitDownloads

After receiving a valid game state, we valid the cgame and local zip files here
and determine if we need to download them
=================
*/
void CL_InitDownloads()
{
	// TTimo
	// init some of the www dl data
	clc.bWWWDl = false;
	clc.bWWWDlAborting = false;
	CL_ClearStaticDownload();

	if ( cl_allowDownload->integer )
		FS_DeletePaksWithBadChecksum();

	// reset the redirect checksum tracking
	clc.redirectedList[ 0 ] = '\0';

	if ( cl_allowDownload->integer && FS_ComparePaks( clc.downloadList, sizeof( clc.downloadList ) ) )
	{
		downloadLogger.Debug("Need paks: '%s'", clc.downloadList);

		if ( *clc.downloadList )
		{
			// if autodownloading is not enabled on the server
			cls.state = connstate_t::CA_DOWNLOADING;
			CL_NextDownload();
			return;
		}
	}

	CL_DownloadsComplete();
}

/*
==================
CL_WWWDownload
==================
*/
void CL_WWWDownload()
{
	dlStatus_t      ret;
	static bool bAbort = false;

	if ( clc.bWWWDlAborting )
	{
		if ( !bAbort )
		{
			Log::Debug( "CL_WWWDownload: WWWDlAborting" );
			bAbort = true;
		}

		return;
	}

	if ( bAbort )
	{
		Log::Debug( "CL_WWWDownload: WWWDlAborting done" );
		bAbort = false;
	}

	ret = DL_DownloadLoop();

	if ( ret == dlStatus_t::DL_CONTINUE )
	{
		return;
	}

	if ( ret == dlStatus_t::DL_DONE )
	{
		downloadLogger.Debug("Finished WWW download of '%s', moving it to '%s'", cls.downloadTempName, cls.originalDownloadName);
		// taken from CL_ParseDownload
		clc.download = 0;

		FS_SV_Rename( cls.downloadTempName, cls.originalDownloadName );

		*cls.downloadTempName = *cls.downloadName = 0;
		Cvar_Set( "cl_downloadName", "" );

		CL_AddReliableCommand( "wwwdl done" );

		// tracking potential web redirects leading us to wrong checksum - only works in connected mode
		if ( strlen( clc.redirectedList ) + strlen( cls.originalDownloadName ) + 1 >= sizeof( clc.redirectedList ) )
		{
			// just to be safe
			Log::Warn( "redirectedList overflow (%s)\n", clc.redirectedList );
		}
		else
		{
			strcat( clc.redirectedList, "@" );
			strcat( clc.redirectedList, cls.originalDownloadName );
		}
	}
	else
	{
		// see CL_ParseDownload, same abort strategy
		Log::Notice( "Download failure while getting '%s'\n", cls.downloadName );
		CL_AddReliableCommand( "wwwdl fail" );
		clc.bWWWDlAborting = true;

		return;
	}

	clc.bWWWDl = false;
	CL_NextDownload();
}

/*
==================
CL_WWWBadChecksum

FS code calls this when doing FS_ComparePaks
we can detect files that we got from a www dl redirect with a wrong checksum
this indicates that the redirect setup is broken, and next dl attempt should NOT redirect
==================
*/
bool CL_WWWBadChecksum( const char *pakname )
{
	if ( strstr( clc.redirectedList, va( "@%s@", pakname ) ) )
	{
		Log::Warn("file %s obtained through download redirect has wrong checksum\n"
		              "\tthis likely means the server configuration is broken", pakname );

		if ( strlen( clc.badChecksumList ) + strlen( pakname ) + 1 >= sizeof( clc.badChecksumList ) )
		{
			Log::Warn("badChecksumList overflowed (%s)", clc.badChecksumList );
			return false;
		}

		strcat( clc.badChecksumList, "@" );
		strcat( clc.badChecksumList, pakname );
		Log::Debug( "bad checksums: %s", clc.badChecksumList );
		return true;
	}

	return false;
}

/*
=====================
CL_ParseDownload

A download message has been received from the server
=====================
*/
void CL_ParseDownload( msg_t *msg )
{
	int           size;
	unsigned char data[ MAX_MSGLEN ];
	int           block;

	if ( !*cls.downloadTempName )
	{
		Log::Notice( "Server sending download, but no download was requested\n" );
		// Eat the packet anyway
		block = MSG_ReadShort( msg );
		if (block == -1) {
			MSG_ReadString( msg );
			MSG_ReadLong( msg );
			MSG_ReadLong( msg );
		} else if (block != 0) {
			size = MSG_ReadShort( msg );
			if ( size < 0 || size > (int) sizeof( data ) )
			{
				Sys::Drop( "CL_ParseDownload: Invalid size %d for download chunk.", size );
			}
			MSG_ReadData( msg, data, size );
		}

		CL_AddReliableCommand( "stopdl" );
		return;
	}

	// read the data
	block = MSG_ReadShort( msg );

	// TTimo - www dl
	// if we haven't acked the download redirect yet
	if ( block == -1 )
	{
		if ( !clc.bWWWDl )
		{
			// server is sending us a www download
			Q_strncpyz( cls.originalDownloadName, cls.downloadName, sizeof( cls.originalDownloadName ) );
			Q_strncpyz( cls.downloadName, MSG_ReadString( msg ), sizeof( cls.downloadName ) );
			clc.downloadSize = MSG_ReadLong( msg );
			int basePathLen = MSG_ReadLong( msg );

			downloadLogger.Debug("Server sent us a new WWW DL '%s', size %i, prefix len %i",
			                     cls.downloadName, clc.downloadSize, basePathLen);

			Cvar_SetValue( "cl_downloadSize", clc.downloadSize );
			clc.bWWWDl = true; // activate wwwdl client loop
			CL_AddReliableCommand( "wwwdl ack" );
			cls.state = connstate_t::CA_DOWNLOADING;

			// make sure the server is not trying to redirect us again on a bad checksum
			if ( strstr( clc.badChecksumList, va( "@%s", cls.originalDownloadName ) ) )
			{
				Log::Notice( "refusing redirect to %s by server (bad checksum)\n", cls.downloadName );
				CL_AddReliableCommand( "wwwdl fail" );
				clc.bWWWDlAborting = true;
				return;
			}

			if ( !DL_BeginDownload( cls.downloadTempName, cls.downloadName, basePathLen ) )
			{
				// setting bWWWDl to false after sending the wwwdl fail doesn't work
				// not sure why, but I suspect we have to eat all remaining block -1 that the server has sent us
				// still leave a flag so that CL_WWWDownload is inactive
				// we count on server sending us a gamestate to start up clean again
				CL_AddReliableCommand( "wwwdl fail" );
				clc.bWWWDlAborting = true;
				Log::Notice( "Failed to initialize download for '%s'\n", cls.downloadName );
			}

			return;
		}
		else
		{
			// server keeps sending that message till we ack it, eat and ignore
			//MSG_ReadLong( msg );
			MSG_ReadString( msg );
			MSG_ReadLong( msg );
			MSG_ReadLong( msg );
			return;
		}
	}

	if ( !block )
	{
		// block zero is special, contains file size
		clc.downloadSize = MSG_ReadLong( msg );

		downloadLogger.Debug("Starting new direct download of size %i for '%s'", clc.downloadSize, cls.downloadTempName);
		Cvar_SetValue( "cl_downloadSize", clc.downloadSize );

		if ( clc.downloadSize < 0 )
		{
			Sys::Drop( "%s", MSG_ReadString( msg ) );
		}
	}

	size = MSG_ReadShort( msg );

	if ( size < 0 || size > (int) sizeof( data ) )
	{
		Sys::Drop( "CL_ParseDownload: Invalid size %d for download chunk.", size );
	}

	downloadLogger.Debug("Received block of size %i", size);

	MSG_ReadData( msg, data, size );

	if ( clc.downloadBlock != block )
	{
		downloadLogger.Debug( "CL_ParseDownload: Expected block %i, got %i", clc.downloadBlock, block );
		return;
	}

	// open the file if not opened yet
	if ( !clc.download )
	{
		clc.download = FS_SV_FOpenFileWrite( cls.downloadTempName );

		if ( !clc.download )
		{
			Log::Notice( "Could not create %s\n", cls.downloadTempName );
			CL_AddReliableCommand( "stopdl" );
			CL_NextDownload();
			return;
		}
	}

	if ( size )
	{
		FS_Write( data, size, clc.download );
	}

	CL_AddReliableCommand( va( "nextdl %d", clc.downloadBlock ) );
	clc.downloadBlock++;

	clc.downloadCount += size;

	// So UI gets access to it
	cl_downloadCount.Set(clc.downloadCount);

	if ( !size )
	{
		downloadLogger.Debug("Received EOF, closing '%s'", cls.downloadTempName);
		// A zero length block means EOF
		if ( clc.download )
		{
			FS_FCloseFile( clc.download );
			clc.download = 0;

			// rename the file
			FS_SV_Rename( cls.downloadTempName, cls.downloadName );
		}

		*cls.downloadTempName = *cls.downloadName = 0;
		Cvar_Set( "cl_downloadName", "" );

		// send intentions now
		// We need this because without it, we would hold the last nextdl and then start
		// loading right away.  If we take a while to load, the server is happily trying
		// to send us that last block over and over.
		// Write it twice to help make sure we acknowledge the download
		CL_WritePacket();
		CL_WritePacket();

		// get another file if needed
		CL_NextDownload();
	}
}
