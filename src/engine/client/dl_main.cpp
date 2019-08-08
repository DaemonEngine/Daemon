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

/* Additional features that would be nice for this code:
        * Only display <gamepath>/<file>, i.e., etpro/etpro-3_0_1.pk3 in the UI.
        * Add server as referring URL
*/

#include <curl/curl.h>

#include "qcommon/q_shared.h"
#include "qcommon/qcommon.h"

extern Log::Logger downloadLogger; // cl_download.cpp

// initialize once
static int   dl_initialized = 0;

static CURLM *dl_multi = nullptr;
static CURL  *dl_request = nullptr;
static fileHandle_t dl_file;

/*
** Write to file
*/
static size_t DL_cb_FWriteFile( void *ptr, size_t size, size_t nmemb, void *stream )
{
	fileHandle_t file = ( fileHandle_t )( intptr_t ) stream;

	return FS_Write( ptr, size * nmemb, file );
}

/*
** Print progress
*/
static int DL_cb_Progress( void*, double, double dlnow, double, double )
{
	/* cl_downloadSize and cl_downloadTime are set by the Q3 protocol...
	   and it would probably be expensive to verify them here.   -zinx */

	Cvar_SetValue( "cl_downloadCount", ( float ) dlnow );
	return 0;
}

void DL_InitDownload()
{
	if ( dl_initialized )
	{
		return;
	}

	/* Make sure curl has initialized, so the cleanup doesn't get confused */
	if ( curl_global_init( CURL_GLOBAL_ALL ) == CURLE_OK && (dl_multi = curl_multi_init()) != nullptr )
	{
		downloadLogger.Debug( "Client download subsystem initialized" );
		dl_initialized = 1;
	}
	else
	{
		downloadLogger.Warn( "Error initializing libcurl" );
	}
}

static void DL_StopDownload()
{
	if ( !dl_initialized )
	{
		return;
	}

	if ( dl_request )
	{
		curl_multi_remove_handle( dl_multi, dl_request );
		curl_easy_cleanup( dl_request );
		dl_request = nullptr;
	}

	if ( dl_file )
	{
		FS_FCloseFile( dl_file );
		dl_file = 0;
	}
}

/*
================
DL_Shutdown

================
*/
void DL_Shutdown()
{
	if ( !dl_initialized )
	{
		return;
	}

	DL_StopDownload();

	curl_global_cleanup();

	dl_initialized = 0;
}

/*
===============
inspired from http://www.w3.org/Library/Examples/LoadToFile.c
setup the download, return once we have a connection
===============
*/
int DL_BeginDownload( const char *localName, const char *remoteName )
{
	char referer[ MAX_STRING_CHARS + URI_SCHEME_LENGTH ];

	DL_StopDownload();

	if ( !localName || !remoteName )
	{
		downloadLogger.Notice( "Empty download URL or empty local file name" );
		return 0;
	}

	dl_file = FS_SV_FOpenFileWrite( localName );

	if ( !dl_file )
	{
		downloadLogger.Notice( "DL_BeginDownload unable to open '%s' for writing", localName );
		return 0;
	}

	DL_InitDownload();
	if ( !dl_initialized )
	{
		return 0;
	}

	strcpy( referer, URI_SCHEME );
	Q_strncpyz( referer + URI_SCHEME_LENGTH, Cvar_VariableString( "cl_currentServerIP" ), MAX_STRING_CHARS );

	dl_request = curl_easy_init();
	if ( !dl_request )
	{
		downloadLogger.Warn( "curl_easy_init returned null" );
		return 0;
	}
#define SETOPT(option, value) \
	if (curl_easy_setopt(dl_request, option, value) != CURLE_OK) { \
		downloadLogger.Warn("Setting " #option " failed"); \
		return 0; \
	}
	SETOPT( CURLOPT_USERAGENT, va( "%s %s", PRODUCT_NAME "/" PRODUCT_VERSION, curl_version() ) );
	SETOPT( CURLOPT_REFERER, referer );
	SETOPT( CURLOPT_URL, remoteName );
	SETOPT( CURLOPT_PROTOCOLS, long(CURLPROTO_HTTP) );
	SETOPT( CURLOPT_WRITEFUNCTION, DL_cb_FWriteFile );
	SETOPT( CURLOPT_WRITEDATA, ( void * )( intptr_t ) dl_file );
	SETOPT( CURLOPT_PROGRESSFUNCTION, DL_cb_Progress );
	SETOPT( CURLOPT_NOPROGRESS, 0L );
	SETOPT( CURLOPT_FAILONERROR, 1L );

	CURLMcode err = curl_multi_add_handle( dl_multi, dl_request );
	if (err != CURLM_OK)
	{
		downloadLogger.Warn("curl_multi_add_handle error: %s", curl_multi_strerror(err));
		curl_easy_cleanup( dl_request );
		dl_request = nullptr;
		return 0;
	}

	Cvar_Set( "cl_downloadName", remoteName );

	return 1;
}

// (maybe this should be CL_DL_DownloadLoop)
dlStatus_t DL_DownloadLoop()
{
	CURLMcode  status;
	CURLMsg    *msg;
	int        dls;

	if ( !dl_request )
	{
		downloadLogger.Warn( "DL_DownloadLoop: unexpected call with dl_request == NULL" );
		return dlStatus_t::DL_DONE;
	}

	curl_multi_perform( dl_multi, &dls );

	do {
		 msg = curl_multi_info_read( dl_multi, &dls );
	} while (msg && msg->msg != CURLMSG_DONE);

	if ( !msg )
	{
		return dlStatus_t::DL_CONTINUE;
	}

	if ( msg->data.result != CURLE_OK )
	{
#ifdef __MACOS__ // ���
		const char* err = "unknown curl error.";
#else
		const char* err = curl_easy_strerror( msg->data.result );
#endif
		downloadLogger.Notice( "DL_DownloadLoop: request terminated with failure status '%s'", err );
		DL_StopDownload();
		return dlStatus_t::DL_FAILED;
	}
	long httpStatus = -1;
	curl_easy_getinfo(dl_request, CURLINFO_RESPONSE_CODE, &httpStatus);

	DL_StopDownload();

	if ( httpStatus != 200 )
	{
		// We don't follow redirects, so report a failure if we get one
		// (they're not considered an error for CURLOPT_FAILONERROR purposes).
		downloadLogger.Notice( "Download returned HTTP %d", httpStatus );
		return dlStatus_t::DL_FAILED;
	}

	return dlStatus_t::DL_DONE;
}
