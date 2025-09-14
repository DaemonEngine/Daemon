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

// cl_serverlist.cpp: master server querying and serverinfo queries
// Everything involved in the two-step process for getting an in-game server list:
// (1) Query the master server for a list of servers' IP addresses
// (2) Send serverinfo queries to the addresses on the list. These queries are referred
//     to as  "pings". In addition to the response latency, the status information
//     about the server is used.

#include "common/Common.h"
#include "client.h"
#include "engine/framework/Crypto.h"
#include "engine/framework/Network.h"

#define MAX_PINGREQUESTS         16

static Log::Logger serverInfoLog("client.serverinfo", "");

static Cvar::Cvar<std::string> cl_gamename(
	"cl_gamename", "game name for master server queries", Cvar::TEMPORARY, GAMENAME_FOR_MASTER);

static Cvar::Range<Cvar::Cvar<int>> cl_maxPing(
	"cl_maxPing", "ping timeout for server list", Cvar::NONE, 800, 100, 9999);

constexpr int PING_MAX_ATTEMPTS = 3;
static Cvar::Range<Cvar::Cvar<int>> pingSpacing[ PING_MAX_ATTEMPTS ] {
	{"cl_pingSpacing", "milliseconds between ping packets (1st attempt)", Cvar::NONE, 5, 0, 5000},
	{"cl_pingSpacingRetry1", "milliseconds between ping packets for 1st retry or -1 to disable retry", Cvar::NONE, 50, -1, 5000},
	{"cl_pingSpacingRetry2", "milliseconds between ping packets for 2nd retry or -1 to disable retry", Cvar::NONE, 125, -1, 5000},
};

struct ping_t
{
	netadr_t adr;
	int      start;
	int      time;
	char     challenge[ 9 ]; // 8-character challenge string
	serverResponseProtocol_t responseProto;
	std::string info;
};

ping_t             cl_pinglist[ MAX_PINGREQUESTS ];
static int lastPingSendTime = -99999;

/*
===================
CL_InitServerInfo
===================
*/
static void CL_InitServerInfo( serverInfo_t *server, netadr_t *address )
{
	server->adr = *address;
	server->label[ 0 ] = '\0';
	server->pingStatus = pingStatus_t::WAITING;
	server->pingAttempts = 0;
	server->ping = -1;
	server->responseProto = serverResponseProtocol_t::UNKNOWN;
}

/*
===================
CL_GSRSequenceInformation

Parses this packet's index and the number of packets from a master server's
response. Updates the packet count and returns the index. Advances the data
pointer as appropriate (but only when parsing was successful)

The sequencing information isn't terribly useful at present (we can skip
duplicate packets, but we don't bother to make sure we've got all of them).
===================
*/
static int CL_GSRSequenceInformation( byte **data )
{
	char *p = ( char * ) *data, *e;
	int  ind, num;

	// '\0'-delimited fields: this packet's index, total number of packets
	if ( *p++ != '\0' )
	{
		return -1;
	}

	ind = strtol( p, &e, 10 );

	if ( *e++ != '\0' )
	{
		return -1;
	}

	num = strtol( e, &p, 10 );

	if ( *p++ != '\0' )
	{
		return -1;
	}

	if ( num <= 0 || ind <= 0 || ind > num )
	{
		return -1; // nonsensical response
	}

	if ( cls.numMasterPackets > 0 && num != cls.numMasterPackets )
	{
		// Assume we sent two getservers and somehow they changed in
		// between - only use the results that arrive later
		Log::Debug( "Master changed its mind about packet count!" );
		cls.numglobalservers = 0;
	}

	cls.numMasterPackets = num;

	// successfully parsed
	*data = ( byte * ) p;
	return ind;
}

/*
===================
CL_GSRFeaturedLabel

Parses from the data an arbitrary text string labelling the servers in the
following getserversresponse packet.
The result is copied to *buf, and *data is advanced as appropriate
===================
*/
static void CL_GSRFeaturedLabel( byte **data, char *buf, int size )
{
	char *l = buf;
	buf[ 0 ] = '\0';

	// copy until '\0' which indicates field break
	// or slash which indicates beginning of server list
	while ( **data && **data != '\\' && **data != '/' )
	{
		if ( l < &buf[ size - 1 ] )
		{
			*l = **data;
		}
		else if ( l == &buf[ size - 1 ] )
		{
			Log::Warn( "CL_GSRFeaturedLabel: overflow" );
		}

		l++, ( *data ) ++;
	}
}

static const int MAX_SERVERSPERPACKET = 256;

/*
===================
CL_ServerLinksResponsePacket
===================
*/
void CL_ServerLinksResponsePacket( msg_t *msg )
{
	uint16_t      port;
	byte      *buffptr;
	byte      *buffend;

	serverInfoLog.Debug( "CL_ServerLinksResponsePacket" );

	if ( msg->data[ 30 ] != 0 )
	{
		return;
	}

	// parse through server response string
	cls.numserverLinks = 0;
	buffptr = msg->data + 31; // skip header
	buffend = msg->data + msg->cursize;

	// Each link contains:
	// * an IPv4 address & port
	// * an IPv6 address & port
	while ( buffptr < buffend - 20 && cls.numserverLinks < ARRAY_LEN( cls.serverLinks ) )
	{
		cls.serverLinks[ cls.numserverLinks ].type = netadrtype_t::NA_IP_DUAL;

		// IPv4 address
		memcpy( cls.serverLinks[ cls.numserverLinks ].ip, buffptr, 4 );
		port = buffptr[ 4 ] << 8 | buffptr[ 5 ];
		cls.serverLinks[ cls.numserverLinks ].port4 = UBigShort( port );
		buffptr += 6;

		// IPv4 address
		memcpy( cls.serverLinks[ cls.numserverLinks ].ip6, buffptr, 16 );
		port = buffptr[ 16 ] << 8 | buffptr[ 17 ];
		cls.serverLinks[ cls.numserverLinks ].port6 = UBigShort( port );
		buffptr += 18;

		++cls.numserverLinks;
	}

	serverInfoLog.Debug( "%d server address pairs parsed", cls.numserverLinks );
}

/*
===================
CL_ServersResponsePacket
===================
*/
void CL_ServersResponsePacket( const netadr_t *from, msg_t *msg, bool extended )
{
	int      count, duplicate_count, parsed_count;
	netadr_t addresses[ MAX_SERVERSPERPACKET ];
	int      numservers;
	byte     *buffptr;
	byte     *buffend;
	char     label[ MAX_FEATLABEL_CHARS ] = "";

	serverInfoLog.Debug( "CL_ServersResponsePacket" );

	duplicate_count = 0;

	if ( cls.numglobalservers == -1 )
	{
		// state to detect lack of servers or lack of response
		cls.numglobalservers = 0;
		cls.numMasterPackets = 0;
	}

	// parse through server response string
	numservers = 0;
	buffptr = msg->data;
	buffend = buffptr + msg->cursize;

	// advance to initial token
	// skip header
	buffptr += 4;

	// advance to initial token
	// I considered using strchr for this but I don't feel like relying
	// on its behaviour with '\0'
	while ( *buffptr && *buffptr != '\\' && *buffptr != '/' )
	{
		buffptr++;

		if ( buffptr + 1 >= buffend )
		{
			break;
		}
	}

	if ( *buffptr == '\0' )
	{
		int ind = CL_GSRSequenceInformation( &buffptr );

		if ( ind >= 0 )
		{

			// TODO: detect dropped packets and make another
			// request
			serverInfoLog.Debug(
				"CL_ServersResponsePacket: packet %d of %d", ind, cls.numMasterPackets );

			CL_GSRFeaturedLabel( &buffptr, label, sizeof( label ) );
		}

		// now skip to the server list
		for ( ; buffptr < buffend && *buffptr != '\\' && *buffptr != '/';
		      buffptr++ ) {; }
	}

	while ( buffptr + 1 < buffend )
	{
		bool duplicate = false;
		byte ip6[16];
		byte ip[4];
		uint16_t port;

		// IPv4 address
		if ( *buffptr == '\\' )
		{
			buffptr++;

			if ( buffend - buffptr < (ptrdiff_t) (sizeof( addresses[ numservers ].ip ) + sizeof( addresses[ numservers ].port ) + 1) )
			{
				break;
			}

			// parse out ip
			memcpy( ip, buffptr, sizeof( ip ) );
			buffptr += sizeof( ip );

			// parse out port
			port = ( *buffptr++ ) << 8;
			port += *buffptr++;
			port = UBigShort( port );;

			// deduplicate server list, do not add known server
			for ( int i = 0; i < cls.numglobalservers; i++ )
			{
				if ( cls.globalServers[ i ].adr.port == port && !memcmp( cls.globalServers[ i ].adr.ip, ip, sizeof( ip ) ) )
				{
					duplicate = true;
					duplicate_count++;
					break;
				}
			}

			memcpy( addresses[ numservers ].ip, ip, sizeof( ip ) );

			addresses[ numservers ].port = port;
			addresses[ numservers ].type = netadrtype_t::NA_IP;

			// look up this address in the links list
			for (unsigned j = 0; j < cls.numserverLinks && !duplicate; ++j )
			{
				if ( addresses[ numservers ].port == cls.serverLinks[ j ].port4 && !memcmp( addresses[ numservers ].ip, cls.serverLinks[ j ].ip, sizeof( addresses[ numservers ].ip ) ) )
				{
					// found it, so look up the corresponding address

					// hax to get the IP address & port as a string (memcmp etc. SHOULD work, but...)
					cls.serverLinks[ j ].type = netadrtype_t::NA_IP6;
					cls.serverLinks[ j ].port = cls.serverLinks[ j ].port6;
					std::string s = Net::AddressToString( cls.serverLinks[ j ], true );
					cls.serverLinks[j].type = netadrtype_t::NA_IP_DUAL;

					for ( int i = 0; i < numservers; ++i )
					{
						if ( s == Net::AddressToString( addresses[ i ], true ) )
						{
							// found: replace with the preferred address, exit both loops
							addresses[ i ] = cls.serverLinks[ j ];
							addresses[ i ].type = NET_TYPE( cls.serverLinks[ j ].type );
							addresses[ i ].port = ( addresses[ i ].type == netadrtype_t::NA_IP ) ? cls.serverLinks[ j ].port4 : cls.serverLinks[ j ].port6;
							duplicate = true;
							duplicate_count++;
							break;
						}
					}
				}
			}
		}
		// IPv6 address, if it's an extended response
		else if ( extended && *buffptr == '/' )
		{
			buffptr++;

			if ( buffend - buffptr < (ptrdiff_t) (sizeof( addresses[ numservers ].ip6 ) + sizeof( addresses[ numservers ].port ) + 1) )
			{
				break;
			}

			for ( unsigned i = 0; i < sizeof( addresses[ numservers ].ip6 ); i++ )
			{
				addresses[ numservers ].ip6[ i ] = *buffptr++;
			}

			// parse out ip
			memcpy( ip6, buffptr, sizeof( ip6 ) );
			buffptr += sizeof( ip6 );

			// parse out port
			port = ( *buffptr++ ) << 8;
			port += *buffptr++;
			port = UBigShort( port );;

			// deduplicate server list, do not add known server
			for ( int i = 0; i < cls.numglobalservers; i++ )
			{
				if ( cls.globalServers[ i ].adr.port == port && !memcmp( cls.globalServers[ i ].adr.ip6, ip6, sizeof( ip6 ) ) )
				{
					duplicate = true;
					duplicate_count++;
					break;
				}
			}

			memcpy( addresses[ numservers ].ip6, ip6, sizeof( ip6 ) );

			addresses[ numservers ].port = port;
			addresses[ numservers ].type = netadrtype_t::NA_IP6;
			addresses[ numservers ].scope_id = from->scope_id;

			// look up this address in the links list
			for ( unsigned j = 0; j < cls.numserverLinks && !duplicate; ++j )
			{
				if ( addresses[ numservers ].port == cls.serverLinks[ j ].port6 && !memcmp( addresses[ numservers ].ip6, cls.serverLinks[ j ].ip6, sizeof( addresses[ numservers ].ip6 ) ) )
				{
					// found it, so look up the corresponding address

					// hax to get the IP address & port as a string (memcmp etc. SHOULD work, but...)
					cls.serverLinks[ j ].type = netadrtype_t::NA_IP;
					cls.serverLinks[ j ].port = cls.serverLinks[ j ].port4;
					std::string s = Net::AddressToString( cls.serverLinks[ j ], true );
					cls.serverLinks[j].type = netadrtype_t::NA_IP_DUAL;

					for ( int i = 0; i < numservers; ++i )
					{
						if ( s == Net::AddressToString( addresses[ i ], true ) )
						{
							// found: replace with the preferred address, exit both loops
							addresses[ i ] = cls.serverLinks[ j ];
							addresses[ i ].type = NET_TYPE( cls.serverLinks[ j ].type );
							addresses[ i ].port = ( addresses[ i ].type == netadrtype_t::NA_IP ) ? cls.serverLinks[ j ].port4 : cls.serverLinks[ j ].port6;
							duplicate = true;
							duplicate_count++;
							break;
						}
					}
				}
			}
		}
		else
		{
			// syntax error!
			break;
		}

		// syntax check
		if ( *buffptr != '\\' && *buffptr != '/' )
		{
			break;
		}

		if ( !duplicate )
		{
			++numservers;

			if ( numservers >= MAX_SERVERSPERPACKET )
			{
				break;
			}
		}
	}

	count = cls.numglobalservers;

	int i;
	for ( i = 0; i < numservers && count < MAX_GLOBAL_SERVERS; i++ )
	{
		// build net address
		serverInfo_t *server = &cls.globalServers[ count ];

		CL_InitServerInfo( server, &addresses[ i ] );
		Q_strncpyz( server->label, label, sizeof( server->label ) );
		// advance to next slot
		count++;
	}

	cls.numglobalservers = count;
	parsed_count = numservers + duplicate_count;

	serverInfoLog.Debug( "%d servers parsed, %s new, %d duplicate (total %d)",
		parsed_count, numservers, duplicate_count, count );
}

static void CL_SetServerInfo(
	serverInfo_t *server, const std::string& info,
	serverResponseProtocol_t proto, pingStatus_t pingStatus, int ping )
{
	if ( info.size() )
	{
		server->infoString = info;
	}

	server->responseProto = proto;
	server->pingStatus = pingStatus;
	server->ping = ping;
}

static void CL_SetServerInfoByAddress(
	const netadr_t& from, const std::string& info,
	serverResponseProtocol_t proto, pingStatus_t pingStatus, int ping )
{
	for ( int i = 0; i < MAX_OTHER_SERVERS; i++ )
	{
		if ( NET_CompareAdr( from, cls.localServers[ i ].adr ) )
		{
			CL_SetServerInfo( &cls.localServers[ i ], info, proto, pingStatus, ping );
		}
	}

	for ( int i = 0; i < MAX_GLOBAL_SERVERS; i++ )
	{
		if ( NET_CompareAdr( from, cls.globalServers[ i ].adr ) )
		{
			CL_SetServerInfo( &cls.globalServers[ i ], info, proto, pingStatus, ping );
		}
	}
}

/*
===================
CL_ServerInfoPacket
===================
*/
void CL_ServerInfoPacket( const netadr_t& from, msg_t *msg )
{
	std::string infoStr = MSG_ReadString( msg );
	InfoMap infoString = InfoStringToMap( infoStr );

	// if this isn't the correct protocol version, ignore it
	int prot = atoi( infoString["protocol"].c_str() );

	if ( prot != PROTOCOL_VERSION )
	{
		serverInfoLog.Verbose( "Different protocol info packet: %i", prot );
		return;
	}

	// Arnout: if this isn't the correct game, ignore it
	const std::string& gameName = infoString["gamename"];

	if ( !gameName[ 0 ] || gameName != GAMENAME_STRING )
	{
		serverInfoLog.Verbose( "Different game info packet: %s", gameName );
		return;
	}

	// iterate servers waiting for ping response
	for ( int i = 0; i < MAX_PINGREQUESTS; i++ )
	{
		if ( cl_pinglist[ i ].adr.port && cl_pinglist[ i ].time == -1 && NET_CompareAdr( from, cl_pinglist[i].adr ) )
		{
			if ( cl_pinglist[ i ].challenge != infoString["challenge"] )
			{
				serverInfoLog.Verbose( "wrong challenge for ping response from %s", NET_AdrToString( from ) );
				return;
			}

			// calc ping time
			cl_pinglist[ i ].time = Sys::Milliseconds() - cl_pinglist[ i ].start;

			serverInfoLog.Debug( "ping time %dms from %s", cl_pinglist[ i ].time, NET_AdrToString( from ) );

			// save of info
			cl_pinglist[i].info = infoStr;

			// tack on the net type
			switch ( from.type )
			{
				case netadrtype_t::NA_BROADCAST:
				case netadrtype_t::NA_IP:
					cl_pinglist[ i ].responseProto = serverResponseProtocol_t::IP4;
					break;

				case netadrtype_t::NA_IP6:
					cl_pinglist[ i ].responseProto = serverResponseProtocol_t::IP6;
					break;

				default:
					cl_pinglist[ i ].responseProto = serverResponseProtocol_t::UNKNOWN;
					break;
			}

			CL_SetServerInfoByAddress( from, infoStr, cl_pinglist[ i ].responseProto,
			                           pingStatus_t::COMPLETE, cl_pinglist[ i ].time );

			return;
		}
	}

	// if not just sent a local broadcast or pinging local servers
	if ( cls.pingUpdateSource != AS_LOCAL )
	{
		return;
	}

	int i = 0;
	for ( ; i < MAX_OTHER_SERVERS; i++ )
	{
		// empty slot
		if ( cls.localServers[ i ].adr.port == 0 )
		{
			break;
		}

		// avoid duplicate
		if ( NET_CompareAdr( from, cls.localServers[ i ].adr ) )
		{
			return;
		}
	}

	if ( i == MAX_OTHER_SERVERS )
	{
		serverInfoLog.Notice("MAX_OTHER_SERVERS hit, dropping infoResponse" );
		return;
	}

	// add this to the list
	cls.numlocalservers = i + 1;
	cls.localServers[ i ].adr = from;
	cls.localServers[ i ].ping = -1;
	cls.localServers[ i ].pingStatus = pingStatus_t::WAITING;
	cls.localServers[ i ].pingAttempts = 0;
	cls.localServers[ i ].responseProto = serverResponseProtocol_t::UNKNOWN;
	cls.localServers[ i ].infoString.clear();

	std::string info = MSG_ReadString( msg );

	// TODO when does this happen?
	if ( info[ 0 ] )
	{
		if ( info.back() == '\n' )
		{
			info = info.substr( 0, info.size() - 1 );
		}

		Log::Notice( "%s: %s", Net::AddressToString( from, true ), info );
	}
}

/*
==================
CL_LocalServers_f
==================
*/
void CL_LocalServers_f()
{
	const char *message;
	int      i, j;

	serverInfoLog.Verbose( "Scanning for servers on the local network…" );

	// reset the list, waiting for response
	cls.numlocalservers = 0;
	cls.pingUpdateSource = AS_LOCAL;

	for ( i = 0; i < MAX_OTHER_SERVERS; i++ )
	{
		bool b = cls.localServers[ i ].visible;
		cls.localServers[ i ] = {};
		cls.localServers[ i ].visible = b;
	}

	netadr_t to{};

	// The 'xxx' in the message is a challenge that will be echoed back
	// by the server.  We don't care about that here, but master servers
	// can use that to prevent spoofed server responses from invalid IP addresses
	message = "\377\377\377\377getinfo xxx";
	int messageLen = strlen(message);

	// send each message twice in case one is dropped
	for ( i = 0; i < 2; i++ )
	{
		// send a broadcast packet on each server port
		// we support multiple server ports so a single machine
		// can nicely run multiple servers
		for ( j = 0; j < NUM_SERVER_PORTS; j++ )
		{
			to.port = UBigShort( ( uint16_t )( PORT_SERVER + j ) );

			to.type = netadrtype_t::NA_BROADCAST;
			NET_SendPacket( netsrc_t::NS_CLIENT, messageLen, message, to );

			to.type = netadrtype_t::NA_MULTICAST6;
			NET_SendPacket( netsrc_t::NS_CLIENT, messageLen, message, to );
		}
	}
}

/*
==================
CL_GlobalServers_f
==================
*/
void CL_GlobalServers_f()
{
	netadr_t to;
	int      count, i, masterNum, protocol;
	char     command[ 1024 ], *masteraddress;
	bool     wildcard = false;
	bool     active_master_servers[MAX_MASTER_SERVERS] = { false, false, false, false, false };

	count = Cmd_Argc();
	protocol = atoi( Cmd_Argv( 2 ) );   // Do this right away, otherwise weird things happen when you use the ingame "Get New Servers" button.

	if ( ! strcmp( Cmd_Argv( 1 ), "*" ) )
	{
		wildcard = true;
		for ( masterNum = 0; masterNum < MAX_MASTER_SERVERS; masterNum++ )
		{
			active_master_servers[ masterNum ] = true;
		}
	}
	else {
		masterNum = atoi( Cmd_Argv( 1 ) );
		active_master_servers[ masterNum ] = true;
	}

	if ( count < 2 || ( ( masterNum < 0 || masterNum > MAX_MASTER_SERVERS - 1 ) && !wildcard ) )
	{
		Cmd_PrintUsage("(<master# 0-" XSTRING(MAX_MASTER_SERVERS - 1) "> | *) [<protocol>] [<keywords>]", nullptr);
		return;
	}

	for ( masterNum = 0; masterNum < MAX_MASTER_SERVERS; masterNum++ )
	{
		if ( !active_master_servers[ masterNum ] ) {
			continue;
		}

		Com_sprintf( command, sizeof( command ), "sv_master%d", masterNum + 1 );
		masteraddress = Cvar_VariableString( command );

		if ( !*masteraddress )
		{
			if ( !wildcard )
			{
				serverInfoLog.Warn( "CL_GlobalServers_f: No master server address given" );
			}
			continue;
		}

		serverInfoLog.Debug( "CL_GlobalServers_f: Resolving %s", masteraddress );

		// reset the list, waiting for response
		// -1 is used to distinguish a "no response"

		i = NET_StringToAdr( masteraddress, &to, netadrtype_t::NA_UNSPEC );

		if ( !i )
		{
			serverInfoLog.Warn( "CL_GlobalServers_f: Could not resolve address of master %s", masteraddress );
			continue;
		}
		else if ( i == 2 )
		{
			to.port = UBigShort( PORT_MASTER );
		}

		serverInfoLog.Verbose(
			"CL_GlobalServers_f: %s resolved to %s", masteraddress, Net::AddressToString( to, true ) );

		serverInfoLog.Debug( "CL_GlobalServers_f: Requesting servers from master %s…", masteraddress );

		cls.numglobalservers = -1;
		cls.numserverLinks = 0;
		cls.pingUpdateSource = AS_GLOBAL;

		Com_sprintf( command, sizeof( command ), "getserversExt %s %d dual",
		             cl_gamename.Get().c_str(), protocol);

		// TODO: test if we only have IPv4/IPv6, if so request only the relevant
		// servers with getserversExt %s %d ipvX
		// not that big a deal since the extra servers won't respond to getinfo
		// anyway.

		for ( i = 3; i < count; i++ )
		{
			Q_strcat( command, sizeof( command ), " " );
			Q_strcat( command, sizeof( command ), Cmd_Argv( i ) );
		}

		Net::OutOfBandPrint( netsrc_t::NS_SERVER, to, "%s", command );
	}
}

/*
==================
CL_GetPing
==================
*/
static pingStatus_t CL_GetPing( int n )
{
	ASSERT( n >= 0 && n < MAX_PINGREQUESTS );
	ASSERT( cl_pinglist[ n ].adr.port );

	pingStatus_t status;
	// FIXME: don't use 0 for timed out or waiting in cgame ABI
	int time;

	if ( cl_pinglist[ n ].time >= 0 )
	{
		time = cl_pinglist[ n ].time;
		status = pingStatus_t::COMPLETE;
	}
	else
	{
		// check for timeout
		int elapsed = Sys::Milliseconds() - cl_pinglist[ n ].start;

		if ( elapsed >= cl_maxPing.Get() )
		{
			time = 0;
			status = pingStatus_t::TIMEOUT;
		}
		else
		{
			time = 0;
			status = pingStatus_t::WAITING;
		}
	}

	// FIXME: do we really need to call this again? CL_ServerInfoPacket already calls it
	// at the moment the ping response arrives
	CL_SetServerInfoByAddress( cl_pinglist[ n ].adr, cl_pinglist[ n ].info,
	                           cl_pinglist[ n ].responseProto, status, time );

	return status;
}

/*
==================
CL_ClearPing
==================
*/
static void CL_ClearPing( int n )
{
	if ( n < 0 || n >= MAX_PINGREQUESTS )
	{
		return;
	}

	cl_pinglist[ n ].adr.port = 0;
	cl_pinglist[ n ].info[ 0 ] = '\0';
}

/*
==================
CL_GetPingQueueCount
==================
*/
static int CL_GetPingQueueCount()
{
	int    i;
	int    count;
	ping_t *pingptr;

	count = 0;
	pingptr = cl_pinglist;

	for ( i = 0; i < MAX_PINGREQUESTS; i++, pingptr++ )
	{
		if ( pingptr->adr.port )
		{
			count++;
		}
	}

	return ( count );
}

/*
==================
CL_GetFreePing
==================
*/
static ping_t &CL_GetFreePing()
{
	// Look for a free slot
	for ( ping_t &ping : cl_pinglist )
	{
		if ( !ping.adr.port )
		{
			return ping;
		}
	}

	// Look for an existing ping to cancel
	ping_t *best = nullptr;
	int oldestStart = std::numeric_limits<int>::max();

	for ( ping_t &ping : cl_pinglist )
	{
		if ( ping.start <= oldestStart )
		{
			best = &ping;
			oldestStart = ping.start;
		}
	}

	if ( best->time >= 0 )
	{
		serverInfoLog.Verbose( "CL_GetFreePing: evicting completed ping record" );
	}
	else
	{
		serverInfoLog.Verbose( "CL_GetFreePing: evicting outstanding ping request" );
	}

	return *best;
}

static void GeneratePingChallenge( ping_t &ping )
{
	Crypto::Data bytes( 6 );
	Sys::GenRandomBytes( bytes.data(), bytes.size() );
	Crypto::Data base64 = Crypto::Encoding::Base64Encode( bytes );
	Q_strncpyz( ping.challenge, Crypto::ToString( base64 ).c_str(), sizeof(ping.challenge) );
}

/*
==================
CL_Ping_f
==================
*/
void CL_Ping_f()
{
	ping_t        *pingptr;
	const char   *server;
	int          argc;
	netadrtype_t family = netadrtype_t::NA_UNSPEC;

	argc = Cmd_Argc();

	if ( argc != 2 && argc != 3 )
	{
		Cmd_PrintUsage("[-4|-6] <server>", nullptr);
		return;
	}

	if ( argc == 2 )
	{
		server = Cmd_Argv( 1 );
	}
	else
	{
		if ( !strcmp( Cmd_Argv( 1 ), "-4" ) )
		{
			family = netadrtype_t::NA_IP;
		}
		else if ( !strcmp( Cmd_Argv( 1 ), "-6" ) )
		{
			family = netadrtype_t::NA_IP6;
		}
		else
		{
			Log::Warn("only -4 or -6 as address type understood." );
		}

		server = Cmd_Argv( 2 );
	}

	netadr_t to{};

	if ( !NET_StringToAdr( server, &to, family ) )
	{
		return;
	}

	pingptr = &CL_GetFreePing();

	pingptr->adr = to;
	pingptr->start = Sys::Milliseconds();
	pingptr->time = -1;
	GeneratePingChallenge( *pingptr );

	CL_SetServerInfoByAddress( pingptr->adr, "", serverResponseProtocol_t::UNKNOWN,
	                           pingStatus_t::WAITING, 0 );

	Net::OutOfBandPrint( netsrc_t::NS_CLIENT, to, "getinfo %s", pingptr->challenge );
}

// complete all of the 1st tries before starting 2nd tries, etc.
static int PingAttemptNum( const serverInfo_t *servers, int count )
{
	int result = PING_MAX_ATTEMPTS;
	for ( int i = 0; i < count; i++ )
	{
		if ( servers[ i ].visible && servers[ i ].pingStatus != pingStatus_t::COMPLETE )
		{
			result = std::min( result, servers[ i ].pingAttempts );
		}
	}

	while ( result < PING_MAX_ATTEMPTS && pingSpacing[ result ].Get() < 0 )
	{
		++result;
	}

	return result;
}

static void HarvestCompletedPings()
{
	for ( int i = 0; i < MAX_PINGREQUESTS; i++ )
	{
		if ( !cl_pinglist[ i ].adr.port )
		{
			continue;
		}

		if ( CL_GetPing( i ) != pingStatus_t::WAITING )
		{
			CL_ClearPing( i );
		}
	}
}

/*
==================
CL_UpdateVisiblePings_f

Returns true if there are any newly completed pings or any outstanding pings
==================
*/
bool CL_UpdateVisiblePings_f( int source )
{
	if ( source < 0 || source >= AS_NUM_TYPES )
	{
		return false;
	}

	cls.pingUpdateSource = source;
	int usedSlots = CL_GetPingQueueCount();
	bool status = usedSlots > 0;
	HarvestCompletedPings();

	if ( usedSlots < MAX_PINGREQUESTS )
	{
		serverInfo_t *server;
		int max;

		switch ( source )
		{
			case AS_LOCAL:
				server = &cls.localServers[ 0 ];
				max = cls.numlocalservers;
				break;

			case AS_GLOBAL:
				server = &cls.globalServers[ 0 ];
				max = cls.numglobalservers;
				break;

			default:
				ASSERT_UNREACHABLE();
		}

		int attempt = PingAttemptNum( server, max );

		if ( attempt >= PING_MAX_ATTEMPTS )
		{
			return status; // all pings are complete
		}

		if ( Sys::Milliseconds() < lastPingSendTime + pingSpacing[ attempt ].Get() )
		{
			return true; // rate limited
		}

		for ( int i = 0; i < max; i++ )
		{
			if ( !server[ i ].visible )
			{
				continue;
			}

			if ( server[ i ].pingStatus == pingStatus_t::COMPLETE )
			{
				continue;
			}

			if ( server[ i ].pingAttempts > attempt )
			{
				continue;
			}

			int j;

			for ( j = 0; j < MAX_PINGREQUESTS; j++ )
			{
				if ( !cl_pinglist[ j ].adr.port )
				{
					continue;
				}

				if ( NET_CompareAdr( cl_pinglist[ j ].adr, server[ i ].adr ) )
				{
					// already on the list
					break;
				}
			}

			// Not in the list, so find and use a free slot.
			if ( j >= MAX_PINGREQUESTS )
			{
				status = true;

				ping_t &ping = CL_GetFreePing();
				ping.adr = server[ i ].adr;
				ping.start = Sys::Milliseconds();
				ping.time = -1;
				GeneratePingChallenge( ping );
				Net::OutOfBandPrint( netsrc_t::NS_CLIENT, ping.adr, "getinfo %s", ping.challenge );
				lastPingSendTime = ping.start;
				server[ i ].pingAttempts = attempt + 1;

				if ( ++usedSlots >= MAX_PINGREQUESTS )
				{
					break;
				}

				if ( pingSpacing[ attempt ].Get() > 0 )
				{
					break;
				}
			}
		}
	}

	if ( usedSlots > 0 )
	{
		status = true;
	}

	return status;
}
