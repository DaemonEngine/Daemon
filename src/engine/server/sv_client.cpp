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

// sv_client.c -- server code for dealing with clients

#include "server.h"
#include "CryptoChallenge.h"
#include "framework/Network.h"
#include "qcommon/sys.h"
#include <common/FileSystem.h>

// HTTP download params
static Cvar::Cvar<bool> sv_wwwDownload("sv_wwwDownload", "have clients download missing paks via HTTP", Cvar::NONE, true);
static Cvar::Cvar<std::string> sv_wwwBaseURL("sv_wwwBaseURL", "where clients download paks (must NOT be HTTPS, must contain PAKSERVER)", Cvar::NONE, WWW_BASEURL);
static Cvar::Cvar<std::string> sv_wwwFallbackURL("sv_wwwFallbackURL", "alternative download site to sv_wwwBaseURL", Cvar::NONE, "");

static void SV_CloseDownload( client_t *cl );

void SV_GetChallenge( const netadr_t& from )
{
	auto challenge = ChallengeManager::GenerateChallenge( from );
	Net::OutOfBandPrint( netsrc_t::NS_SERVER, from, "challengeResponse %s", challenge );
}

/*
==================
SV_DirectConnect

A "connect" OOB command has been received
==================
*/
void SV_DirectConnect( const netadr_t& from, const Cmd::Args& args )
{
	if ( args.Argc() < 2 )
	{
		return;
	}

	Log::Debug( "SVC_DirectConnect ()" );

	InfoMap userinfo = InfoStringToMap(args.Argv(1));

	// DHM - Nerve :: Update Server allows any protocol to connect
	// NOTE TTimo: but we might need to store the protocol around for potential non http/ftp clients
	int version = atoi( userinfo["protocol"].c_str() );

	if ( version != PROTOCOL_VERSION )
	{
		Net::OutOfBandPrint( netsrc_t::NS_SERVER, from, "print\nServer uses protocol version %i (yours is %i).", PROTOCOL_VERSION, version );
		Log::Debug( "    rejected connect from version %i", version );
		return;
	}

	int qport = atoi( userinfo["qport"].c_str() );

	auto clients_begin = svs.clients;
	auto clients_end = clients_begin + sv_maxClients.Get();

	client_t* reconnecting = std::find_if(clients_begin, clients_end,
		[&from, qport](const client_t& client)
		{
			return NET_CompareBaseAdr( from, client.netchan.remoteAddress )
		     && ( client.netchan.qport == qport || from.port == client.netchan.remoteAddress.port );
		}
	);

	if ( reconnecting != clients_end &&
		svs.time - reconnecting->lastConnectTime < sv_reconnectlimit->integer * 1000 )
	{
		Log::Debug( "%s: reconnect rejected: too soon", NET_AdrToString( from ) );
		return;
	}


	if ( NET_IsLocalAddress( from ) )
	{
		userinfo["ip"] = "loopback";
	}
	else
	{
		// see if the challenge is valid (local clients don't need to challenge)
		Challenge::Duration ping_duration;
		if ( !ChallengeManager::MatchString( from, userinfo["challenge"], &ping_duration ) )
		{
			Net::OutOfBandPrint( netsrc_t::NS_SERVER, from, "print\n[err_dialog]No or bad challenge for address." );
			return;
		}

		userinfo["ip"] = NET_AdrToString( from );
	}

	client_t *new_client = nullptr;

	// if there is already a slot for this IP address, reuse it
	if ( reconnecting != clients_end )
	{
		Log::Notice( "%s:reconnect", NET_AdrToString( from ) );
		new_client = reconnecting;
	}
	else
	{
		// find a client slot
		// if "sv_privateClients" is set > 0, then that number
		// of client slots will be reserved for connections that
		// have "password" set to the value of "sv_privatePassword"
		// Info requests will report the maxclients as if the private
		// slots didn't exist, to prevent people from trying to connect
		// to a full server.
		// This is to allow us to reserve a couple slots here on our
		// servers so we can play without having to kick people.
		// check for privateClient password

		auto allowed_clients_begin = clients_begin;
		if ( userinfo["password"] != sv_privatePassword->string )
		{
			// skip past the reserved slots
			allowed_clients_begin += std::min(sv_privateClients.Get(), sv_maxClients.Get());
		}

		new_client = std::find_if(allowed_clients_begin, clients_end,
			[](const client_t& client) {
				return client.state == clientState_t::CS_FREE;
		});

		if ( new_client == clients_end )
		{
			// This is a bizarre special case, in which if you have a local address and EVERY
			// non-private client is a bot (and there is at least 1), you can boot one of them off.
			if ( NET_IsLocalAddress( from ) && sv_privateClients.Get() < sv_maxClients.Get() )
			{
				bool all_bots = std::all_of(allowed_clients_begin, clients_end,
					[](const client_t& client) { return SV_IsBot(&client); }
				);

				if ( all_bots )
				{
					SV_DropClient( &svs.clients[ sv_maxClients.Get() - 1 ], "only bots on server" );
					new_client = &svs.clients[ sv_maxClients.Get() - 1 ];
				}
				else
				{
					Sys::Error( "server is full on local connect" );
				}
			}
			else
			{
				Net::OutOfBandPrint( netsrc_t::NS_SERVER, from, "print\n%s", sv_fullmsg->string );
				Log::Debug( "Rejected a connection." );
				return;
			}
		}
	}

	// build a new connection
	// accept the new client
	// this is the only place a client_t is ever initialized
	ResetStruct( *new_client );
	int clientNum = new_client - svs.clients;

	Log::Notice( "Client %i connecting", clientNum );

	new_client->gentity = SV_GentityNum( clientNum );
	new_client->gentity->r.svFlags = 0;

	// save the address
	Netchan_Setup( netsrc_t::NS_SERVER, &new_client->netchan, from, qport );
	// init the netchan queue

	// Save the pubkey
	Q_strncpyz( new_client->pubkey, userinfo["pubkey"].c_str(), sizeof( new_client->pubkey ) );
	userinfo.erase("pubkey");
	// save the userinfo
	Q_strncpyz( new_client->userinfo, InfoMapToString(userinfo).c_str(), sizeof( new_client->userinfo ) );

	// get the game a chance to reject this connection or modify the userinfo
	char reason[ MAX_STRING_CHARS ];
	if ( gvm.GameClientConnect( reason, sizeof( reason ), clientNum, true, false ) )
	{
		Net::OutOfBandPrint( netsrc_t::NS_SERVER, from, "print\n[err_dialog]%s", reason );
		Log::Debug( "Game rejected a connection: %s.", reason );
		return;
	}

	SV_UserinfoChanged( new_client );

	// send the connect packet to the client
	Net::OutOfBandPrint( netsrc_t::NS_SERVER, from, "connectResponse" );

	Log::Debug( "Going from CS_FREE to CS_CONNECTED for %s", new_client->name );

	new_client->state = clientState_t::CS_CONNECTED;
	new_client->nextSnapshotTime = svs.time;
	new_client->lastPacketTime = svs.time;
	new_client->lastConnectTime = svs.time;

	// when we receive the first packet from the client, we will
	// notice that it is from a different serverid and that the
	// gamestate message was not just sent, forcing a retransmit
	new_client->gamestateMessageNum = -1;

	// if this was the first client on the server, or the last client
	// the server can hold, send a heartbeat to the master.
	int count = std::count_if(clients_begin, clients_end,
		[](const client_t& client) {
			return client.state >= clientState_t::CS_CONNECTED;
	});

	if ( count == 1 || count == sv_maxClients.Get() )
	{
		SV_Heartbeat_f();
	}
}

/*
=====================
SV_FreeClient

Destructor for data allocated in a client structure
=====================
*/
void SV_FreeClient( client_t *client )
{
	SV_Netchan_FreeQueue( client );
	SV_CloseDownload( client );
}

/*
=====================
SV_DropClient

Called when the player is totally leaving the server, either willingly
or unwillingly.  This is NOT called if the entire server is quiting
or crashing -- SV_FinalCommand() will handle that
=====================
*/
void SV_DropClient( client_t *drop, const char *reason )
{
	if ( drop->state == clientState_t::CS_ZOMBIE )
	{
		return; // already dropped
	}
	bool isBot = SV_IsBot( drop );

	Log::Debug( "Going to CS_ZOMBIE for %s", drop->name );
	drop->state = clientState_t::CS_ZOMBIE; // become free in a few seconds

	// call the prog function for removing a client
	// this will remove the body, among other things
	gvm.GameClientDisconnect( drop - svs.clients );

	if ( isBot )
	{
		SV_BotFreeClient( drop - svs.clients );
	}
	else
	{
		// tell everyone why they got dropped
		// Gordon: we want this displayed elsewhere now
		SV_SendServerCommand( nullptr, "print %s\"^* \"%s\"\n\"", Cmd_QuoteString( drop->name ), Cmd_QuoteString( reason ) );

		// add the disconnect command
		SV_SendServerCommand( drop, "disconnect %s\n", Cmd_QuoteString( reason ) );
	}

	// nuke user info
	SV_SetUserinfo( drop - svs.clients, "" );

	SV_FreeClient( drop );

	// if this was the last client on the server, send a heartbeat
	// to the master so it is known the server is empty
	// send a heartbeat now so the master will get up to date info
	// if there is already a slot for this IP address, reuse it
	int i;
	for ( i = 0; i < sv_maxClients.Get(); i++ )
	{
		if ( svs.clients[ i ].state >= clientState_t::CS_CONNECTED )
		{
			break;
		}
	}

	if ( i == sv_maxClients.Get() )
	{
		SV_Heartbeat_f();
	}
}

/*
================
SV_SendClientGameState

Sends the first message from the server to a connected client.
This will be sent on the initial connection and upon each new map load.

It will be resent if the client acknowledges a later message but has
the wrong gamestate.
================
*/
void SV_SendClientGameState( client_t *client )
{
	int           start;
	entityState_t *base;
	msg_t         msg;
	byte          msgBuffer[ MAX_MSGLEN ];

	Log::Debug( "SV_SendClientGameState() for %s", client->name );
	Log::Debug( "Going from CS_CONNECTED to CS_PRIMED for %s", client->name );
	client->state = clientState_t::CS_PRIMED;

	// when we receive the first packet from the client, we will
	// notice that it is from a different serverid and that the
	// gamestate message was not just sent, forcing a retransmit
	client->gamestateMessageNum = client->netchan.outgoingSequence;

	MSG_Init( &msg, msgBuffer, sizeof( msgBuffer ) );

	// NOTE, MRE: all server->client messages now acknowledge
	// let the client know which reliable clientCommands we have received
	MSG_WriteLong( &msg, client->lastClientCommand );

	// send any server commands waiting to be sent first.
	// we have to do this cause we send the client->reliableSequence
	// with a gamestate and it sets the clc.serverCommandSequence at
	// the client side
	SV_UpdateServerCommandsToClient( client, &msg );

	// send the gamestate
	MSG_WriteByte( &msg, svc_gamestate );
	MSG_WriteLong( &msg, client->reliableSequence );

	// write the configstrings
	for ( start = 0; start < MAX_CONFIGSTRINGS; start++ )
	{
		if ( sv.configstrings[ start ][ 0 ] )
		{
			MSG_WriteByte( &msg, svc_configstring );
			MSG_WriteShort( &msg, start );
			MSG_WriteBigString( &msg, sv.configstrings[ start ] );
		}
	}

	// write the baselines
	entityState_t nullstate{};

	for ( start = 0; start < MAX_GENTITIES; start++ )
	{
		base = &sv.svEntities[ start ].baseline;

		if ( !base->number )
		{
			continue;
		}

		MSG_WriteByte( &msg, svc_baseline );
		MSG_WriteDeltaEntity( &msg, &nullstate, base, true );

		if ( MAX_MSGLEN - msg.cursize < 128 ) {
			// We have too many entities to put them all into one msg_t, so split it here
			MSG_WriteByte( &msg, svc_gamestatePartial );
			SV_SendMessageToClient( &msg, client );

			MSG_Init( &msg, msgBuffer, sizeof( msgBuffer ) );
			MSG_WriteLong( &msg, client->lastClientCommand );
			MSG_WriteByte( &msg, svc_gamestate );
		}
	}

	MSG_WriteByte( &msg, svc_EOF );

	MSG_WriteLong( &msg, client - svs.clients );

	// NERVE - SMF - debug info
	Log::Debug( "Sending %i bytes in gamestate to client: %i", msg.cursize, client - svs.clients );

	// deliver this to the client
	SV_SendMessageToClient( &msg, client );
}

/*
==================
SV_ClientEnterWorld
==================
*/
void SV_ClientEnterWorld( client_t *client, usercmd_t *cmd )
{
	int            clientNum;
	sharedEntity_t *ent;

	Log::Debug( "Going from CS_PRIMED to CS_ACTIVE for %s", client->name );
	client->state = clientState_t::CS_ACTIVE;

	// set up the entity for the client
	clientNum = client - svs.clients;
	ent = SV_GentityNum( clientNum );
	ent->s.number = clientNum;
	client->gentity = ent;

	client->deltaMessage = -1;
	client->nextSnapshotTime = svs.time; // generate a snapshot immediately
	client->lastUsercmd = *cmd;

	// call the game begin function
	gvm.GameClientBegin( client - svs.clients );
}

/*
============================================================

CLIENT COMMAND EXECUTION

============================================================
*/

/*
==================
SV_CloseDownload

clear/free any download vars
==================
*/
static void SV_CloseDownload( client_t *cl )
{
	int i;

	// EOF
	if ( cl->download )
	{
		delete cl->download;
		cl->download = nullptr;
	}

	*cl->downloadName = 0;

	// Free the temporary buffer space
	for ( i = 0; i < MAX_DOWNLOAD_WINDOW; i++ )
	{
		if ( cl->downloadBlocks[ i ] )
		{
			Z_Free( cl->downloadBlocks[ i ] );
			cl->downloadBlocks[ i ] = nullptr;
		}
	}
}

/*
==================
SV_StopDownload_f

Abort a download if in progress
==================
*/
void SV_StopDownload_f( client_t *cl, const Cmd::Args& )
{
	if ( *cl->downloadName )
	{
		Log::Debug( "clientDownload: %d: file \"%s^*\" aborted", ( int )( cl - svs.clients ), cl->downloadName );
	}

	SV_CloseDownload( cl );
}

/*
==================
SV_DoneDownload_f

Downloads are finished
==================
*/
void SV_DoneDownload_f( client_t *cl, const Cmd::Args& )
{
	if ( cl->state == clientState_t::CS_ACTIVE )
	{
		return;
	}

	Log::Debug( "clientDownload: %s^* Done", cl->name );
	// resend the game state to update any clients that entered during the download
	SV_SendClientGameState( cl );
}

/*
==================
SV_NextDownload_f

The argument will be the last acknowledged block from the client, it should be
the same as cl->downloadClientBlock
==================
*/
void SV_NextDownload_f( client_t *cl, const Cmd::Args& args )
{
	int block;
	if (args.Argc() < 2 or not Str::ParseInt(block, args.Argv(1))) {
		return;
	}

	if ( block == cl->downloadClientBlock )
	{
		Log::Debug( "clientDownload: %d: client acknowledge of block %d", ( int )( cl - svs.clients ), block );

		// Find out if we are done.  A zero-length block indicates EOF
		if ( cl->downloadBlockSize[ cl->downloadClientBlock % MAX_DOWNLOAD_WINDOW ] == 0 )
		{
			Log::Notice( "clientDownload: %d : file \"%s\" completed", ( int )( cl - svs.clients ), cl->downloadName );
			SV_CloseDownload( cl );
			return;
		}

		cl->downloadSendTime = svs.time;
		cl->downloadClientBlock++;
		return;
	}

	// We aren't getting an acknowledge for the correct block, drop the client
	// FIXME: this is bad... the client will never parse the disconnect message
	//          because the cgame isn't loaded yet
	SV_DropClient( cl, "broken download" );
}

/*
==================
SV_BeginDownload_f
==================
*/
void SV_BeginDownload_f( client_t *cl, const Cmd::Args& args )
{
	// Kill any existing download
	SV_CloseDownload( cl );

	if (args.Argc() < 2) {
		return;
	}

	//bani - stop us from printing dupe messages
	if (args.Argv(1) != cl->downloadName)
	{
		cl->downloadnotify = DLNOTIFY_ALL;
	}

	// cl->downloadName is non-zero now, SV_WriteDownloadToClient will see this and open
	// the file itself
	Q_strncpyz( cl->downloadName, args.Argv(1).c_str(), sizeof( cl->downloadName ) );
}

/*
==================
SV_WWWDownload_f
==================
*/
void SV_WWWDownload_f( client_t *cl, const Cmd::Args& args )
{
	if (args.Argc() < 2) {
		return;
	}

	const char *subcmd = args.Argv(1).c_str();

	// only accept wwwdl commands for clients which we first flagged as wwwdl ourselves
	if ( !cl->bWWWDl )
	{
		Log::Notice( "SV_WWWDownload: unexpected wwwdl '%s^*' for client '%s^*'", subcmd, cl->name );
		SV_DropClient( cl, va( "SV_WWWDownload: unexpected wwwdl %s", subcmd ) );
		return;
	}

	if ( !Q_stricmp( subcmd, "ack" ) )
	{
		if ( cl->bWWWing )
		{
			Log::Warn("dupe wwwdl ack from client '%s^*'", cl->name );
		}

		cl->bWWWing = true;
		return;
	}

	// below for messages that only happen during/after download
	if ( !cl->bWWWing )
	{
		Log::Notice( "SV_WWWDownload: unexpected wwwdl '%s^*' for client '%s^*'", subcmd, cl->name );
		SV_DropClient( cl, va( "SV_WWWDownload: unexpected wwwdl %s", subcmd ) );
		return;
	}

	if ( !Q_stricmp( subcmd, "done" ) )
	{
		*cl->downloadName = 0;
		cl->bWWWing = false;
		return;
	}
	else if ( !Q_stricmp( subcmd, "fail" ) )
	{
		*cl->downloadName = 0;
		cl->bWWWing = false;
		cl->bFallback = true;
		// send a reconnect
		SV_SendClientGameState( cl );
		return;
	}
	else if ( !Q_stricmp( subcmd, "chkfail" ) )
	{
		Log::Warn("client '%s^*' reports that the redirect download for '%s^*' had wrong checksum.\n\tYou should check your download redirect configuration.",
				 cl->name, cl->downloadName );
		*cl->downloadName = 0;
		cl->bWWWing = false;
		cl->bFallback = true;
		// send a reconnect
		SV_SendClientGameState( cl );
		return;
	}

	Log::Notice("SV_WWWDownload: unknown wwwdl subcommand '%s^*' for client '%s^*'", subcmd, cl->name );
	SV_DropClient( cl, va( "SV_WWWDownload: unknown wwwdl subcommand '%s^*'", subcmd ) );
}

// abort an attempted download
void SV_BadDownload( client_t *cl, msg_t *msg )
{
	MSG_WriteByte( msg, svc_download );
	MSG_WriteShort( msg, 0 );  // client is expecting block zero
	MSG_WriteLong( msg, -1 );  // illegal file size

	*cl->downloadName = 0;
}

/*
==================
SV_CheckFallbackURL

sv_wwwFallbackURL can be used to redirect clients to a web URL in case direct ftp/http didn't work (or is disabled on client's end)
return true when a redirect URL message was filled up
when the cvar is set to something, the download server will effectively never use a legacy download strategy
==================
*/
static bool SV_CheckFallbackURL( client_t *cl, const char* pakName, int downloadSize, msg_t *msg )
{
	if ( sv_wwwFallbackURL.Get().empty() )
	{
		return false;
	}

	Log::Notice( "clientDownload: sending client '%s^*' to fallback URL '%s'", cl->name, sv_wwwFallbackURL.Get() );

	Q_strncpyz(cl->downloadURL, va("%s/%s", sv_wwwFallbackURL.Get().c_str(), pakName), sizeof(cl->downloadURL));

	MSG_WriteByte( msg, svc_download );
	MSG_WriteShort( msg, -1 );  // block -1 means ftp/http download
	MSG_WriteString( msg, va( "%s/%s", sv_wwwFallbackURL.Get().c_str(), pakName ) );
	MSG_WriteLong( msg, downloadSize );
	MSG_WriteLong( msg, sv_wwwFallbackURL.Get().size() + 1 );

	return true;
}

/*
==================
SV_WriteDownloadToClient

Check to see if the client wants a file, open it if needed and start pumping the client
Fill up msg with data
==================
*/
void SV_WriteDownloadToClient( client_t *cl, msg_t *msg )
{
	int      curindex;
	int      rate;
	int      blockspersnap;
	char     errorMessage[ 1024 ];

	const FS::PakInfo* pak;
	bool success;
	bool bTellRate = false; // verbosity

	if ( !*cl->downloadName )
	{
		return; // Nothing being downloaded
	}

	if ( cl->bWWWing )
	{
		return; // The client acked and is downloading with ftp/http
	}

	if ( !cl->download )
	{
		// We open the file here

		//bani - prevent duplicate download notifications
		if ( cl->downloadnotify & DLNOTIFY_BEGIN )
		{
			cl->downloadnotify &= ~DLNOTIFY_BEGIN;
			Log::Notice( "clientDownload: %d : beginning \"%s\"", ( int )( cl - svs.clients ), cl->downloadName );
		}

		if ( !sv_allowDownload->integer )
		{
			Log::Notice( "clientDownload: %d : \"%s\" download disabled", ( int )( cl - svs.clients ), cl->downloadName );

			Com_sprintf( errorMessage, sizeof( errorMessage ),
							"Could not download \"%s\" because autodownloading is disabled on the server.\n\n"
							"You will need to get this file elsewhere before you can connect to this server.\n",
							cl->downloadName );

			SV_BadDownload( cl, msg );
			MSG_WriteString( msg, errorMessage );  // (could SV_DropClient instead?)

			return;
		}

		// www download redirect protocol
		// NOTE: this is called repeatedly while a client connects. Maybe we should sort of cache the message or something
		// FIXME: we need to abstract this to an independent module for maximum configuration/usability by server admins
		// FIXME: I could rework that, it's crappy
		if ( sv_wwwDownload.Get() )
		{
			std::string name, version;
			Util::optional<uint32_t> checksum;
			int downloadSize = 0;

			success = FS::ParsePakName(cl->downloadName, cl->downloadName + strlen(cl->downloadName), name, version, checksum);

			if (success) {
				// legacy pk3s have empty version but no checksum
				// looking for that speical version ensures the client load the legacy pk3 if server is using it, even if client has non-legacy dpk
				// dpks have version and can have checksum
				pak = checksum ? FS::FindPak(name, version) : FS::FindPak(name, version, *checksum);

				if (pak) {
					try {
						const FS::offset_t length{FS::RawPath::OpenRead(pak->path).Length()};

						if (length > std::numeric_limits<decltype(downloadSize)>::max()) {
							throw std::system_error{Util::ordinal(std::errc::value_too_large), std::system_category(),
								"Pak file '" + pak->path + "' size '" + std::to_string(length) + "' is larger than max client download size"};
						}

						downloadSize = length;
					} catch (std::system_error& ex) {
						Log::Warn("Client '%s^*': couldn't extract file size for %s - %s", cl->name, cl->downloadName, ex.what());
						success = false;
					}
				} else
					success = false;
			}

			std::string pakName = FS::MakePakName(name, version);

			if ( !cl->bFallback )
			{
				if ( success )
				{
					Q_strncpyz( cl->downloadURL, va("%s/%s", sv_wwwBaseURL.Get().c_str(), pakName.c_str()),
								sizeof( cl->downloadURL ) );

					//bani - prevent multiple download notifications
					if ( cl->downloadnotify & DLNOTIFY_REDIRECT )
					{
						cl->downloadnotify &= ~DLNOTIFY_REDIRECT;
						Log::Notice( "Redirecting client '%s^*' to %s", cl->name, cl->downloadURL );
					}

					// once cl->downloadName is set (and possibly we have our listening socket), let the client know
					cl->bWWWDl = true;
					MSG_WriteByte( msg, svc_download );
					MSG_WriteShort( msg, -1 );  // block -1 means ftp/http download
					// download URL, size of the download file
					MSG_WriteString( msg, cl->downloadURL );
					MSG_WriteLong( msg, downloadSize );
					// Base URL length. The base prefix is expected to end with '/'
					MSG_WriteLong( msg, sv_wwwBaseURL.Get().size() + 1 );
					return;
				}
				else
				{
					// that should NOT happen - even regular download would fail then anyway
					Log::Warn("Client '%s^*': couldn't extract file size for %s", cl->name, cl->downloadName );
				}
			}
			else
			{
				cl->bFallback = false;
				cl->bWWWDl = true;

				if ( SV_CheckFallbackURL( cl, pakName.c_str(), downloadSize, msg ) )
				{
					return;
				}

				Log::Warn("Client '%s^*': falling back to regular downloading for failed file %s", cl->name,
						 cl->downloadName );
			}
		}

		// find file
		cl->bWWWDl = false;
		std::string name, version;
		Util::optional<uint32_t> checksum;

		success = FS::ParsePakName(cl->downloadName, cl->downloadName + strlen(cl->downloadName), name, version, checksum);

		if (success) {
			// legacy paks have empty version but no checksum
			// looking for that special version ensures the client load the legacy pk3 if server is using it, even if client has non-legacy dpk
			// dpks have version and can have checksum
			pak = checksum ? FS::FindPak(name, version) : FS::FindPak(name, version, *checksum);

			if (pak) {
				try {
					cl->download = new FS::File(FS::RawPath::OpenRead(pak->path));

					const FS::offset_t length{cl->download->Length()};

					if (length > std::numeric_limits<decltype(cl->downloadSize)>::max()) {
						throw std::system_error{Util::ordinal(std::errc::value_too_large), std::system_category(),
							"Pak file '" + pak->path + "' size '" + std::to_string(length) + "' is larger than max client download size"};
					}

					cl->downloadSize = length;
				} catch (std::system_error& ex) {
					Log::Notice("clientDownload: %d : \"%s\" file download failed - %s", (int)(cl - svs.clients), cl->downloadName, ex.what());
					success = false;
				}
			} else {
				success = false;
			}
		}

		if ( !success )
		{
			Log::Notice( "clientDownload: %d : \"%s\" file not found on server", ( int )( cl - svs.clients ), cl->downloadName );
			Com_sprintf( errorMessage, sizeof( errorMessage ), "File \"%s\" not found on server for autodownloading.\n",
			             cl->downloadName );
			SV_BadDownload( cl, msg );
			MSG_WriteString( msg, errorMessage );  // (could SV_DropClient instead?)
			return;
		}

		// is valid source, init
		cl->downloadCurrentBlock = cl->downloadClientBlock = cl->downloadXmitBlock = 0;
		cl->downloadCount = 0;
		cl->downloadEOF = false;

		bTellRate = true;
	}

	// Perform any reads that we need to
	while ( cl->downloadCurrentBlock - cl->downloadClientBlock < MAX_DOWNLOAD_WINDOW && cl->downloadSize != cl->downloadCount )
	{
		curindex = ( cl->downloadCurrentBlock % MAX_DOWNLOAD_WINDOW );

		if ( !cl->downloadBlocks[ curindex ] )
		{
			cl->downloadBlocks[ curindex ] = ( byte* ) Z_Malloc( MAX_DOWNLOAD_BLKSIZE );
		}

		try {
			cl->downloadBlockSize[ curindex ] = cl->download->Read(cl->downloadBlocks[ curindex ], MAX_DOWNLOAD_BLKSIZE);
		} catch (std::system_error&) {
			// EOF right now
			cl->downloadCount = cl->downloadSize;
			break;
		}

		cl->downloadCount += cl->downloadBlockSize[ curindex ];

		// Load in next block
		cl->downloadCurrentBlock++;
	}

	// Check to see if we have eof condition and add the EOF block
	if ( cl->downloadCount == cl->downloadSize &&
	     !cl->downloadEOF && cl->downloadCurrentBlock - cl->downloadClientBlock < MAX_DOWNLOAD_WINDOW )
	{
		cl->downloadBlockSize[ cl->downloadCurrentBlock % MAX_DOWNLOAD_WINDOW ] = 0;
		cl->downloadCurrentBlock++;

		cl->downloadEOF = true; // We have added the EOF block
	}

	// Loop up to window size times based on how many blocks we can fit in the
	// client snapMsec and rate

	// based on the rate, how many bytes can we fit in the snapMsec time of the client
	rate = cl->rate;

	// show_bug.cgi?id=509
	// for autodownload, we use a separate max rate value
	// we do this every time because the client might change its rate during the download
	if ( sv_dl_maxRate->integer < rate )
	{
		rate = sv_dl_maxRate->integer;

		if ( bTellRate )
		{
			Log::Notice( "'%s' downloading at sv_dl_maxrate (%d)", cl->name, sv_dl_maxRate->integer );
		}
	}
	else if ( bTellRate )
	{
		Log::Notice( "'%s' downloading at rate %d", cl->name, rate );
	}

	if ( !rate )
	{
		blockspersnap = 1;
	}
	else
	{
		blockspersnap = ( ( rate * cl->snapshotMsec ) / 1000 + MAX_DOWNLOAD_BLKSIZE ) / MAX_DOWNLOAD_BLKSIZE;
	}

	if ( blockspersnap < 0 )
	{
		blockspersnap = 1;
	}

	while ( blockspersnap-- )
	{
		// Write out the next section of the file, if we have already reached our window,
		// automatically start retransmitting

		if ( cl->downloadClientBlock == cl->downloadCurrentBlock )
		{
			return; // Nothing to transmit
		}

		if ( cl->downloadXmitBlock == cl->downloadCurrentBlock )
		{
			// We have transmitted the complete window, should we start resending?

			//FIXME:  This uses a hardcoded one second timeout for lost blocks
			//the timeout should be based on client rate somehow
			if ( svs.time - cl->downloadSendTime > 1000 )
			{
				cl->downloadXmitBlock = cl->downloadClientBlock;
			}
			else
			{
				return;
			}
		}

		// Send current block
		curindex = ( cl->downloadXmitBlock % MAX_DOWNLOAD_WINDOW );

		MSG_WriteByte( msg, svc_download );
		MSG_WriteShort( msg, cl->downloadXmitBlock );

		// block zero is special, contains file size
		if ( cl->downloadXmitBlock == 0 )
		{
			MSG_WriteLong( msg, cl->downloadSize );
		}

		MSG_WriteShort( msg, cl->downloadBlockSize[ curindex ] );

		// Write the block
		if ( cl->downloadBlockSize[ curindex ] )
		{
			MSG_WriteData( msg, cl->downloadBlocks[ curindex ], cl->downloadBlockSize[ curindex ] );
		}

		Log::Debug( "clientDownload: %d: writing block %d", ( int )( cl - svs.clients ), cl->downloadXmitBlock );

		// Move on to the next block
		// It will get sent with next snap shot.  The rate will keep us in line.
		cl->downloadXmitBlock++;

		cl->downloadSendTime = svs.time;
	}
}

/*
=================
SV_Disconnect_f

The client is going to disconnect, so remove the connection immediately  FIXME: move to game?
=================
*/
static void SV_Disconnect_f( client_t *cl, const Cmd::Args& )
{
	SV_DropClient( cl, "disconnected" );
}

/*
=================
SV_UserinfoChanged

Pull specific info from a newly changed userinfo string
into a more C friendly form.
=================
*/
void SV_UserinfoChanged( client_t *cl )
{
	const char *val;
	int  i;

	// name for C code
	Q_strncpyz( cl->name, Info_ValueForKey( cl->userinfo, "name" ), sizeof( cl->name ) );

	// rate command

	// if the client is on the same subnet as the server and we aren't running an
	// Internet server, assume that they don't need a rate choke
	if ( Sys_IsLANAddress( cl->netchan.remoteAddress )
		&& sv_networkScope.Get() <= 1
		&& sv_lanForceRate->integer == 1 )
	{
		cl->rate = 99999; // lans should not rate limit
	}
	else
	{
		val = Info_ValueForKey( cl->userinfo, "rate" );

		if ( strlen( val ) )
		{
			i = atoi( val );
			cl->rate = i;

			if ( cl->rate < 1000 )
			{
				cl->rate = 1000;
			}
			else if ( cl->rate > 90000 )
			{
				cl->rate = 90000;
			}
		}
		else
		{
			cl->rate = 5000;
		}
	}

	// snaps command
	val = Info_ValueForKey( cl->userinfo, "snaps" );

	if ( strlen( val ) )
	{
		i = atoi( val );

		if ( i < 1 )
		{
			i = 1;
		}
		else if ( i > sv_fps->integer )
		{
			i = sv_fps->integer;
		}

		cl->snapshotMsec = 1000 / i;
	}
	else
	{
		cl->snapshotMsec = 50;
	}

	// TTimo
	// maintain the IP information
	// this is set in SV_DirectConnect (directly on the server, not transmitted), may be lost when client updates its userinfo
	// the banning code relies on this being consistently present
	// zinx - modified to always keep this consistent, instead of only
	// when "ip" is 0-length, so users can't supply their own IP address
	//Log::Debug("Maintain IP address in userinfo for '%s'", cl->name);
	Info_SetValueForKey( cl->userinfo, "ip", NET_AdrToString( cl->netchan.remoteAddress ), false );
}

/*
==================
SV_UpdateUserinfo_f
==================
*/
static void SV_UpdateUserinfo_f( client_t *cl, const Cmd::Args& args )
{
	if (args.Argc() < 2) {
		return;
	}

	Q_strncpyz(cl->userinfo, args.Argv(1).c_str(), sizeof(cl->userinfo)); // FIXME QUOTING INFO

	SV_UserinfoChanged( cl );
	// call prog code to allow overrides
	gvm.GameClientUserInfoChanged( cl - svs.clients );
}

struct ucmd_t
{
	const char *name;
	void ( *func )( client_t *cl, const Cmd::Args& args );
	bool allowedpostmapchange;
};

static ucmd_t ucmds[] =
{
	{ "userinfo",   SV_UpdateUserinfo_f,  false },
	{ "disconnect", SV_Disconnect_f,      true  },
	{ "download",   SV_BeginDownload_f,   false },
	{ "nextdl",     SV_NextDownload_f,    false },
	{ "stopdl",     SV_StopDownload_f,    false },
	{ "donedl",     SV_DoneDownload_f,    false },
	{ "wwwdl",      SV_WWWDownload_f,     false },
	{ nullptr,         nullptr, false}
};

/*
==================
SV_ExecuteClientCommand

Also called by bot code
==================
*/

Log::Logger clientCommands("server.clientCommands");
void SV_ExecuteClientCommand( client_t *cl, const char *s, bool clientOK, bool premaprestart )
{
	ucmd_t   *u;
	bool bProcessed = false;

	Log::Debug( "EXCL: %s", s );
	Cmd::Args args(s);

	if (args.Argc() == 0) {
		return;
	}

    clientCommands.Debug("Client %s sent command '%s'", cl->name, s);
	for (u = ucmds; u->name; u++) {
		if (args.Argv(0) == u->name) {
			if (premaprestart && !u->allowedpostmapchange) {
				continue;
			}

			u->func(cl, args);
			bProcessed = true;
			break;
		}
	}

	if ( clientOK )
	{
		// pass unknown strings to the game
		if ( !u->name && sv.state == serverState_t::SS_GAME )
		{
			gvm.GameClientCommand( cl - svs.clients, s );
		}
	}
	else if ( !bProcessed )
	{
		Log::Debug( "client text ignored for %s^*: %s", cl->name, args.Argv(0).c_str());
	}
}

/*
===============
SV_ClientCommand
===============
*/
static bool SV_ClientCommand( client_t *cl, msg_t *msg, bool premaprestart )
{
	bool   clientOk = true;
	bool   floodprotect = true;

	auto seq = MSG_ReadLong( msg );
	auto s = MSG_ReadString( msg );

	// see if we have already executed it
	if ( cl->lastClientCommand >= seq )
	{
		return true;
	}

	Log::Debug( "clientCommand: %s^* : %i : %s", cl->name, seq, s );

	// drop the connection if we have somehow lost commands
	if ( seq > cl->lastClientCommand + 1 )
	{
		Log::Notice( "Client %s lost %i clientCommands", cl->name, seq - cl->lastClientCommand + 1 );
		SV_DropClient( cl, "Lost reliable commands" );
		return false;
	}

	// Gordon: AHA! Need to steal this for some other stuff BOOKMARK
	// NERVE - SMF - some server game-only commands we cannot have flood protect
	if ( !Q_strncmp( "team", s, 4 ) || !Q_strncmp( "setspawnpt", s, 10 ) || !Q_strncmp( "score", s, 5 ) || !Q_stricmp( "forcetapout", s ) )
	{
//      Log::Debug( "Skipping flood protection for: %s", s );
		floodprotect = false;
	}

	// malicious users may try using too many string commands
	// to lag other players.  If we decide that we want to stall
	// the command, we will stop processing the rest of the packet,
	// including the usercmd.  This causes flooders to lag themselves
	// but not other people
	// We don't do this when the client hasn't been active yet, since it is
	// by protocol to spam a lot of commands when downloading
	if ( !com_cl_running->integer && cl->state >= clientState_t::CS_ACTIVE && // (SA) this was commented out in Wolf.  Did we do that?
	     sv_floodProtect->integer && svs.time < cl->nextReliableTime && floodprotect )
	{
		// ignore any other text messages from this client but let them keep playing
		// TTimo - moved the ignored verbose to the actual processing in SV_ExecuteClientCommand, only printing if the core doesn't intercept
		clientOk = false;
	}

	// don't allow another command for 800 msec
	if ( floodprotect && svs.time >= cl->nextReliableTime )
	{
		cl->nextReliableTime = svs.time + 800;
	}

	SV_ExecuteClientCommand( cl, s, clientOk, premaprestart );

	cl->lastClientCommand = seq;
	Com_sprintf( cl->lastClientCommandString, sizeof( cl->lastClientCommandString ), "%s", s );

	return true; // continue processing
}

//==================================================================================

/*
==================
SV_ClientThink

Also called by bot code
==================
*/
void SV_ClientThink( client_t *cl, usercmd_t *cmd )
{
	cl->lastUsercmd = *cmd;

	if ( cl->state != clientState_t::CS_ACTIVE )
	{
		return; // may have been kicked during the last usercmd
	}

	gvm.GameClientThink( cl - svs.clients );
}

/*
==================
SV_UserMove

The message usually contains all the movement commands
that were in the last three packets, so that the information
in dropped packets can be recovered.

On very fast clients, there may be multiple usercmd packed into
each of the backup packets.
==================
*/
static void SV_UserMove( client_t *cl, msg_t *msg, bool delta )
{
	int       i;
	int       cmdCount;
	usercmd_t cmds[ MAX_PACKET_USERCMDS ];
	usercmd_t *cmd, *oldcmd;

	if ( delta )
	{
		cl->deltaMessage = cl->messageAcknowledge;
	}
	else
	{
		cl->deltaMessage = -1;
	}

	cmdCount = MSG_ReadByte( msg );

	if ( cmdCount < 1 )
	{
		Log::Notice( "cmdCount < 1" );
		return;
	}

	if ( cmdCount > MAX_PACKET_USERCMDS )
	{
		Log::Notice( "cmdCount > MAX_PACKET_USERCMDS" );
		return;
	}

	usercmd_t nullcmd{};
	oldcmd = &nullcmd;

	for ( i = 0; i < cmdCount; i++ )
	{
		cmd = &cmds[ i ];
		MSG_ReadDeltaUsercmd( msg, oldcmd, cmd );
//      MSG_ReadDeltaUsercmd( msg, oldcmd, cmd );
		oldcmd = cmd;
	}

	// save time for ping calculation
	cl->frames[ cl->messageAcknowledge & PACKET_MASK ].messageAcked = svs.time;

	// if this is the first usercmd we have received
	// this gamestate, put the client into the world
	if ( cl->state == clientState_t::CS_PRIMED )
	{
		SV_ClientEnterWorld( cl, &cmds[ 0 ] );
		// the moves can be processed normally
	}

	if ( cl->state != clientState_t::CS_ACTIVE )
	{
		cl->deltaMessage = -1;
		return;
	}

	// usually, the first couple commands will be duplicates
	// of ones we have previously received, but the servertimes
	// in the commands will cause them to be immediately discarded
	for ( i = 0; i < cmdCount; i++ )
	{
		// if this is a cmd from before a map_restart ignore it
		if ( cmds[ i ].serverTime > cmds[ cmdCount - 1 ].serverTime )
		{
			continue;
		}

		// extremely lagged or cmd from before a map_restart
		if ( cmds[ i ].serverTime <= cl->lastUsercmd.serverTime )
		{
			continue;
		}

		SV_ClientThink( cl, &cmds[ i ] );
	}
}

/*
===========================================================================

USER CMD EXECUTION

===========================================================================
*/

/*
===================
SV_ExecuteClientMessage

Parse a client packet
===================
*/
void SV_ExecuteClientMessage( client_t *cl, msg_t *msg )
{
	int c;
	int serverId;

	MSG_Bitstream( msg );

	serverId = MSG_ReadLong( msg );
	cl->messageAcknowledge = MSG_ReadLong( msg );

	if ( cl->messageAcknowledge < 0 )
	{
		// usually only hackers create messages like this
		// it is more annoying for them to let them hanging
#ifndef NDEBUG
		SV_DropClient( cl, "DEBUG: illegible client message" );
#endif
		return;
	}

	cl->reliableAcknowledge = MSG_ReadLong( msg );

	// NOTE: when the client message is fux0red the acknowledgement numbers
	// can be out of range, this could cause the server to send thousands of server
	// commands which the server thinks are not yet acknowledged in SV_UpdateServerCommandsToClient
	if ( cl->reliableAcknowledge < 0 || cl->reliableAcknowledge < cl->reliableSequence - MAX_RELIABLE_COMMANDS || cl->reliableAcknowledge > cl->reliableSequence )
	{
		// usually only hackers create messages like this
		// it is more annoying for them to let them hanging
#ifndef NDEBUG
		SV_DropClient( cl, "DEBUG: illegible client message" );
#endif
		cl->reliableAcknowledge = cl->reliableSequence;
		return;
	}

	// if this is a usercmd from a previous gamestate,
	// ignore it or retransmit the current gamestate
	//
	// if the client was downloading, let it stay at whatever serverId and
	// gamestate it was at.  This allows it to keep downloading even when
	// the gamestate changes.  After the download is finished, we'll
	// notice and send it a new game state
	//
	// show_bug.cgi?id=536
	// don't drop as long as previous command was a nextdl, after a dl is done, downloadName is set back to ""
	// but we still need to read the next message to move to next download or send gamestate
	// I don't like this hack though, it must have been working fine at some point, suspecting the fix is somewhere else
	if ( serverId != sv.serverId && !*cl->downloadName && !strstr( cl->lastClientCommandString, "nextdl" ) )
	{
		if ( serverId >= sv.restartedServerId && serverId < sv.serverId )
		{
			// TTimo - use a comparison here to catch multiple map_restart
			// they just haven't caught the map_restart yet
			Log::Debug( "%s^*: ignoring pre map_restart / outdated client message", cl->name );
			return;
		}

		// if the client has not been sent the gamestate yet, or if we can tell that the client has dropped the last
		// gamestate we sent them, resend it
		if ( cl->messageAcknowledge > cl->gamestateMessageNum )
		{
			Log::Debug( "%s^*: sending gamestate", cl->name );
			SV_SendClientGameState( cl );
		}

		// read optional clientCommand strings
		do
		{
			c = MSG_ReadByte( msg );

			if ( c == clc_EOF )
			{
				break;
			}

			if ( c != clc_clientCommand )
			{
				break;
			}

			if ( !SV_ClientCommand( cl, msg, true ) )
			{
				return; // we couldn't execute it because of the flood protection
			}

			if ( cl->state == clientState_t::CS_ZOMBIE )
			{
				return; // disconnect command
			}
		}
		while ( true );

		return;
	}

	// read optional clientCommand strings
	for (;;)
	{
		c = MSG_ReadByte( msg );

		if ( c != clc_clientCommand )
		{
			break;
		}

		if ( !SV_ClientCommand( cl, msg, false ) )
		{
			return; // we couldn't execute it because of the flood protection
		}

		if ( cl->state == clientState_t::CS_ZOMBIE )
		{
			return; // disconnect command
		}
	}

	// read the usercmd_t
	if (c == clc_move) {
		SV_UserMove(cl, msg, true);
	} else if (c == clc_moveNoDelta) {
		SV_UserMove(cl, msg, false);
	} else if (c != clc_EOF) {
		Log::Warn("bad command byte for client %i", (int) (cl - svs.clients));
	}
	if (c != clc_EOF && MSG_ReadByte(msg) != clc_EOF) {
		Log::Warn("missing clc_EOF byte for client %i", (int) (cl - svs.clients));
	}

//  TODO: track bytes read
//	if (msg->readcount != msg->cursize) {
//		Log::Warn("Junk at end of packet for client %i (%i bytes), read %i of %i bytes", cl - svs.clients, msg->cursize - msg->readcount, msg->readcount, msg->cursize);
//	}
}
