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

// cl_main.c  -- client main loop

#include "client.h"
#include "qcommon/q_unicode.h"
#include "qcommon/sys.h"

#include "common/Defs.h"

#include "framework/CommandSystem.h"
#include "framework/CvarSystem.h"

#if defined(USE_MUMBLE)
#include "mumblelink/libmumblelink.h"
#endif

#include "qcommon/crypto.h"
#include "framework/Rcon.h"
#include "framework/Crypto.h"
#include "framework/Network.h"

#ifndef _WIN32
#include <sys/stat.h>
#endif
#ifdef BUILD_GRAPHICAL_CLIENT
#include <SDL.h>
#endif

#if defined(USE_MUMBLE)
cvar_t *cl_useMumble;
cvar_t *cl_mumbleScale;
#endif

cvar_t *cl_nodelta;

cvar_t *cl_noprint;

cvar_t *cl_timeout;
cvar_t *cl_maxpackets;
cvar_t *cl_packetdup;
cvar_t *cl_timeNudge;
cvar_t *cl_showTimeDelta;

cvar_t *cl_shownet = nullptr; // NERVE - SMF - This is referenced in msg.c and we need to make sure it is nullptr
cvar_t *cl_showSend;

Cvar::Cvar<bool> cvar_demo_status_isrecording(
    "demo.status.isrecording",
    "(Read-only) Whether there is a demo currently being recorded",
    Cvar::ROM,
    false
);

Cvar::Cvar<std::string> cvar_demo_status_filename(
    "demo.status.filename",
    "(Read-only) Name of the demo currently being recorded",
    Cvar::ROM,
    ""
);

cvar_t *cl_aviFrameRate;

cvar_t *cl_freelook;
cvar_t *cl_sensitivity;
cvar_t *cl_gameControllerAvailable;

cvar_t *cl_mouseAccelOffset;
cvar_t *cl_mouseAccel;
cvar_t *cl_mouseAccelStyle;
cvar_t *cl_showMouseRate;

cvar_t *m_pitch;
cvar_t *m_yaw;
cvar_t *m_forward;
cvar_t *m_side;
cvar_t *m_filter;

cvar_t *j_pitch;
cvar_t *j_yaw;
cvar_t *j_forward;
cvar_t *j_side;
cvar_t *j_up;
cvar_t *j_pitch_axis;
cvar_t *j_yaw_axis;
cvar_t *j_forward_axis;
cvar_t *j_side_axis;
cvar_t *j_up_axis;

cvar_t *cl_activeAction;

cvar_t *cl_autorecord;

cvar_t *cl_allowDownload;

cvar_t                 *cl_consoleFont;
cvar_t                 *cl_consoleFontSize;
cvar_t                 *cl_consoleFontScaling;
cvar_t                 *cl_consoleFontKerning;
cvar_t                 *cl_consoleCommand; //see also com_consoleCommand for terminal consoles

struct rsa_public_key  public_key;
struct rsa_private_key private_key;

cvar_t             *cl_altTab;

// XreaL BEGIN
cvar_t             *cl_aviMotionJpeg;
// XreaL END

cvar_t             *cl_rate;

clientActive_t     cl;
clientConnection_t clc;
clientStatic_t     cls;
CGameVM            cgvm;

// Structure containing functions exported from refresh DLL
refexport_t        re;

void        CL_CheckForResend();

#if defined(USE_MUMBLE)
static void CL_UpdateMumble()
{
	vec3_t pos, forward, up;
	float  scale = cl_mumbleScale->value;
	float  tmp;

	if ( !cl_useMumble->integer )
	{
		return;
	}

	// !!! FIXME: not sure if this is even close to correct.
	AngleVectors( cl.snap.ps.viewangles, forward, nullptr, up );

	pos[ 0 ] = cl.snap.ps.origin[ 0 ] * scale;
	pos[ 1 ] = cl.snap.ps.origin[ 2 ] * scale;
	pos[ 2 ] = cl.snap.ps.origin[ 1 ] * scale;

	tmp = forward[ 1 ];
	forward[ 1 ] = forward[ 2 ];
	forward[ 2 ] = tmp;

	tmp = up[ 1 ];
	up[ 1 ] = up[ 2 ];
	up[ 2 ] = tmp;

	if ( cl_useMumble->integer > 1 )
	{
		fprintf( stderr, "%f %f %f, %f %f %f, %f %f %f\n",
		         pos[ 0 ], pos[ 1 ], pos[ 2 ],
		         forward[ 0 ], forward[ 1 ], forward[ 2 ],
		         up[ 0 ], up[ 1 ], up[ 2 ] );
	}

	mumble_update_coordinates( pos, forward, up );
}
#endif

/*
=======================================================================

CLIENT RELIABLE COMMAND COMMUNICATION

=======================================================================
*/

/*
======================
CL_AddReliableCommand

The given command will be transmitted to the server, and is guaranteed to
not have future usercmd_t executed before it is executed
======================
*/
void CL_AddReliableCommand( const char *cmd )
{
	int index;

	// catch empty commands
	while ( *cmd && *cmd <= ' ' )
	{
		++cmd;
	}

	if ( !*cmd )
	{
		return;
	}

	// if we would be losing an old command that hasn't been acknowledged,
	// we must drop the connection
	if ( clc.reliableSequence - clc.reliableAcknowledge > MAX_RELIABLE_COMMANDS )
	{
		Sys::Drop( "Client command overflow" );
	}

	clc.reliableSequence++;
	index = clc.reliableSequence & ( MAX_RELIABLE_COMMANDS - 1 );
	Q_strncpyz( clc.reliableCommands[ index ], cmd, sizeof( clc.reliableCommands[ index ] ) );
}

/*
=======================================================================

CLIENT SIDE DEMO RECORDING

=======================================================================
*/

/*
====================
CL_WriteDemoMessage

Dumps the current net message, prefixed by the length
====================
*/
void CL_WriteDemoMessage( msg_t *msg, int headerBytes )
{
	int len, swlen;

	// write the packet sequence
	len = clc.serverMessageSequence;
	swlen = LittleLong( len );
	FS_Write( &swlen, 4, clc.demofile );

	// skip the packet sequencing information
	len = msg->cursize - headerBytes;
	swlen = LittleLong( len );
	FS_Write( &swlen, 4, clc.demofile );
	FS_Write( msg->data + headerBytes, len, clc.demofile );
}


/**
 * If a demo is being recorded, this stops it
 */
static void CL_StopRecord()
{
    if ( !clc.demorecording )
        return;

    // finish up
    int len = -1;
    FS_Write( &len, 4, clc.demofile );
    FS_Write( &len, 4, clc.demofile );
    FS_FCloseFile( clc.demofile );
    clc.demofile = 0;

    clc.demorecording = false;
    Cvar::SetValueForce(cvar_demo_status_isrecording.Name(), "0");
    Cvar::SetValueForce(cvar_demo_status_filename.Name(), "");
    Log::Notice("Stopped demo." );
}

class DemoRecordStopCmd: public Cmd::StaticCmd
{
public:
    DemoRecordStopCmd()
        : Cmd::StaticCmd("demo_record_stop", Cmd::CLIENT, "Stops recording a demo")
    {}

    void Run(const Cmd::Args&) const override
    {
        if ( !clc.demorecording )
        {
            Log::Notice("Not recording a demo." );
            return;
        }
        CL_StopRecord();
    }
};
static DemoRecordStopCmd DemoRecordStopCmdRegistration;


class DemoRecordCmd : public Cmd::StaticCmd
{
public:
    DemoRecordCmd()
        : Cmd::StaticCmd("demo_record", Cmd::CLIENT, "Begins recording a demo from the current position")
    {}

    void Run(const Cmd::Args& args) const override
    {
        if ( args.size() > 2 )
        {
            PrintUsage(args, "[demoname]", "Begins recording a demo from the current position");
            return;
        }

        if ( clc.demorecording )
        {
            Log::Warn("Already recording.");
            return;
        }

        if ( cls.state != connstate_t::CA_ACTIVE )
        {
            Log::Warn("You must be in a level to record.");
            return;
        }

        // ATVI Wolfenstein Misc #479 - changing this to a warning
        // sync 0 doesn't prevent recording, so not forcing it off .. everyone does g_sync 1 ; record ; g_sync 0 ..
        if ( NET_IsLocalAddress(clc.serverAddress) && !Cvar_VariableValue("g_synchronousClients") )
        {
            Log::Warn("You should set '%s' for smoother demo recording" , "g_synchronousClients 1");
        }

        std::string demo_name;

        if ( args.size() == 2 )
        {
            demo_name = args[1];
        }

        CL_Record(demo_name);
    }

};
static DemoRecordCmd DemoRecordCmdRegistration;

/**
 * Returns a demo name from the current map and time
 */
std::string GenerateDemoName()
{
    qtime_t time;
    Com_RealTime(&time);

    std::string map_name = cl.mapname;
    map_name.erase(map_name.rfind('.'));
    auto last_slash = map_name.rfind('/');
    if ( last_slash != std::string::npos )
        map_name.erase(0, last_slash + 1);

    return Str::Format(
        "%04i-%02i-%02i_%02i%02i%02i_%s_%s",
        1900 + time.tm_year, time.tm_mon + 1, time.tm_mday,
        time.tm_hour, time.tm_min, time.tm_sec,
        NET_AdrToString(clc.serverAddress),
        map_name
    );
}

void CL_Record(std::string demo_name)
{
    if ( demo_name.empty() )
        demo_name = GenerateDemoName();

    std::string file_name = Str::Format("demos/%s.dm_%d", demo_name, PROTOCOL_VERSION);
    clc.demofile = FS_FOpenFileWrite(file_name.c_str());
    if ( !clc.demofile )
    {
        Log::Warn("couldn't open %s.", file_name);
        return;
    }
    Log::Notice( "recording to %s.", file_name );

    clc.demorecording = true;
    Q_strncpyz(clc.demoName, demo_name.c_str(), std::min<std::size_t>(demo_name.size(), MAX_QPATH));
    Cvar::SetValueForce(cvar_demo_status_isrecording.Name(), "1");
    Cvar::SetValueForce(cvar_demo_status_filename.Name(), demo_name);

    // don't start saving messages until a non-delta compressed message is received
    clc.demowaiting = true;

    msg_t buf;
    byte bufData[ MAX_MSGLEN ];
    // write out the gamestate message
    MSG_Init( &buf, bufData, sizeof( bufData ) );
    MSG_Bitstream( &buf );

    // NOTE, MRE: all server->client messages now acknowledge
    MSG_WriteLong( &buf, clc.reliableSequence );

    MSG_WriteByte( &buf, svc_gamestate );
    MSG_WriteLong( &buf, clc.serverCommandSequence );


    // configstrings
    for ( int i = 0; i < MAX_CONFIGSTRINGS; i++ )
    {
        if ( cl.gameState[i].empty() )
        {
            continue;
        }

        MSG_WriteByte( &buf, svc_configstring );
        MSG_WriteShort( &buf, i );
        MSG_WriteBigString( &buf, cl.gameState[i].c_str() );
    }

    // baselines
    entityState_t nullstate{};

    for ( int i = 0; i < MAX_GENTITIES; i++ )
    {
        entityState_t *ent = &cl.entityBaselines[ i ];

        if ( !ent->number )
        {
            continue;
        }

        MSG_WriteByte( &buf, svc_baseline );
        MSG_WriteDeltaEntity( &buf, &nullstate, ent, true );
    }

    MSG_WriteByte( &buf, svc_EOF );

    // finished writing the gamestate stuff

    // write the client num
    MSG_WriteLong( &buf, clc.clientNum );

    // finished writing the client packet
    MSG_WriteByte( &buf, svc_EOF );

    // write it to the demo file
    int len = LittleLong( clc.serverMessageSequence - 1 );
    FS_Write( &len, 4, clc.demofile );

    len = LittleLong( buf.cursize );
    FS_Write( &len, 4, clc.demofile );
    FS_Write( buf.data, buf.cursize, clc.demofile );

    // the rest of the demo file will be copied from net messages
}

/*
=======================================================================

CLIENT SIDE DEMO PLAYBACK

=======================================================================
*/

/*
=================
CL_DemoCompleted
=================
*/

NORETURN static void CL_DemoCompleted()
{
	if ( cvar_demo_timedemo.Get() )
	{
		int time;

		time = Sys::Milliseconds() - clc.timeDemoStart;

		if ( time > 0 )
		{
			Log::Notice( "%i frames, %3.1fs: %3.1f fps", clc.timeDemoFrames,
			            time / 1000.0, clc.timeDemoFrames * 1000.0 / time );
		}
	}

	throw Sys::DropErr(false, "Demo completed");
}

/*
=================
CL_ReadDemoMessage
=================
*/

void CL_ReadDemoMessage()
{
	int   r;
	msg_t buf;
	byte  bufData[ MAX_MSGLEN ];
	int   s;

	if ( !clc.demofile )
	{
		CL_DemoCompleted();
	}

	// get the sequence number
	r = FS_Read( &s, 4, clc.demofile );

	if ( r != 4 )
	{
		CL_DemoCompleted();
	}

	clc.serverMessageSequence = LittleLong( s );

	// init the message
	MSG_Init( &buf, bufData, sizeof( bufData ) );

	// get the length
	r = FS_Read( &buf.cursize, 4, clc.demofile );

	if ( r != 4 )
	{
		CL_DemoCompleted();
	}

	buf.cursize = LittleLong( buf.cursize );

	if ( buf.cursize == -1 )
	{
		CL_DemoCompleted();
	}

	if ( buf.cursize > buf.maxsize )
	{
		Sys::Drop( "CL_ReadDemoMessage: demoMsglen > MAX_MSGLEN" );
	}

	r = FS_Read( buf.data, buf.cursize, clc.demofile );

	if ( r != buf.cursize )
	{
		Log::Notice("Demo file was truncated.");
		CL_DemoCompleted();
	}

	clc.lastPacketTime = cls.realtime;
	buf.readcount = 0;
	CL_ParseServerMessage( &buf );
}


class DemoPlayCmd: public Cmd::StaticCmd {
    public:
        DemoPlayCmd(): Cmd::StaticCmd("demo_play", Cmd::CLIENT, "Starts playing a demo file") {
        }

        void Run(const Cmd::Args& args) const override {
            if (args.Argc() != 2) {
                PrintUsage(args, "<demoname>", "starts playing a demo file");
                return;
            }

            // make sure a local server is killed
            Cvar_Set( "sv_killserver", "1" );
            CL_Disconnect( true );

            // open the demo file
            const std::string& fileName = args.Argv(1);

            const char* arg = fileName.c_str();
            int prot_ver = PROTOCOL_VERSION - 1;

            char extension[32];
            char name[ MAX_OSPATH ];
            while (prot_ver <= PROTOCOL_VERSION && !clc.demofile) {
                Com_sprintf(extension, sizeof(extension), ".dm_%d", prot_ver );

                if (!Q_stricmp(arg + strlen(arg) - strlen(extension), extension)) {
                    Com_sprintf(name, sizeof(name), "demos/%s", arg);

                } else {
                    Com_sprintf(name, sizeof(name), "demos/%s.dm_%d", arg, prot_ver);
                }

                FS_FOpenFileRead(name, &clc.demofile);
                prot_ver++;
            }

            if (!clc.demofile) {
                Sys::Drop("couldn't open %s", name);
            }

            Q_strncpyz(clc.demoName, arg, sizeof(clc.demoName));

            Con_Close();

            cls.state = connstate_t::CA_CONNECTED;
            clc.demoplaying = true;

            // read demo messages until connected
            while (cls.state >= connstate_t::CA_CONNECTED && cls.state < connstate_t::CA_PRIMED) {
                CL_ReadDemoMessage();
            }

            // don't get the first snapshot this frame, to prevent the long
            // time from the gamestate load from messing causing a time skip
            clc.firstDemoFrameSkipped = false;
        }

        Cmd::CompletionResult Complete(int argNum, const Cmd::Args&, Str::StringRef prefix) const override {
            if (argNum == 1) {
                return FS::HomePath::CompleteFilename(prefix, "demos", ".dm_" XSTRING(PROTOCOL_VERSION), false, true);
            }

            return {};
        }
};
static DemoPlayCmd DemoPlayCmdRegistration;

// stop demo recording and playback
static void StopDemos()
{
	// stop demo recording
	CL_StopRecord();

	// stop demo playback
	if ( clc.demofile )
	{
		FS_FCloseFile( clc.demofile );
		clc.demofile = 0;
	}
}

//======================================================================

// stop video recording
static void StopVideo()
{
	// stop recording any video
	if ( CL_VideoRecording() )
	{
		// finish rendering current frame
		//SCR_UpdateScreen();
		CL_CloseAVI();
	}
}

/*
=====================
CL_ShutdownAll
=====================
*/
void CL_ShutdownAll()
{
	// clear sounds
	Audio::StopAllSounds();
	// download subsystem
	DL_Shutdown();
	// shutdown CGame
	CL_ShutdownCGame();

	// Clear Faces
	if ( cls.consoleFont )
	{
		re.UnregisterFont( cls.consoleFont );
		cls.consoleFont = nullptr;
	}

	// shutdown the renderer
	if ( re.Shutdown )
	{
		re.Shutdown( false );  // don't destroy window or context
	}

	cls.rendererStarted = false;
	cls.soundRegistered = false;

	StopVideo();
	// Gordon: stop recording on map change etc, demos aren't valid over map changes anyway
	CL_StopRecord();

	if ( !com_sv_running->integer )
	{
		void SV_ShutdownGameProgs();
		SV_ShutdownGameProgs();
	}

	Hunk_Clear();
}

/*
=====================
CL_MapLoading

A local server is starting to load a map, so update the
screen to let the user know about it, then dump all client
memory on the hunk from cgame, ui, and renderer
=====================
*/
void CL_MapLoading()
{
	if ( !com_cl_running->integer )
	{
		return;
	}

	Con_Close();
	cls.keyCatchers = 0;

	// if we are already connected to the local host, stay connected
	if ( cls.state >= connstate_t::CA_CONNECTED && !Q_stricmp( cls.servername, "loopback" ) )
	{
		cls.state = connstate_t::CA_CONNECTED; // so the connect screen is drawn
		memset( cls.updateInfoString, 0, sizeof( cls.updateInfoString ) );
		memset( clc.serverMessage, 0, sizeof( clc.serverMessage ) );
		cl.gameState.fill("");
		clc.lastPacketSentTime = -9999;
		SCR_UpdateScreen();
	}
	else
	{
		try {
			CL_Disconnect( false );
		} catch (Sys::DropErr& err) {
			Sys::Error( "CL_Disconnect error during map load: %s", err.what() );
		}
		Q_strncpyz( cls.servername, "loopback", sizeof( cls.servername ) );
		*cls.reconnectCmd = 0; // can't reconnect to this!
		cls.state = connstate_t::CA_CHALLENGING; // so the connect screen is drawn
		cls.keyCatchers = 0;
		SCR_UpdateScreen();
		clc.connectTime = -RETRANSMIT_TIMEOUT;
		NET_StringToAdr( cls.servername, &clc.serverAddress, netadrtype_t::NA_UNSPEC );
		// we don't need a challenge on the localhost

		CL_CheckForResend();
	}
}

/*
=====================
CL_ClearState

Called before parsing a gamestate
=====================
*/
void CL_ClearState()
{
	ResetStruct( cl );
}

/*
=====================
CL_Disconnect

Called when a connection or demo is being terminated.
Goes from a connected state to either a menu state or a console state
Sends a disconnect message to the server
This is also called on Com_Error and Com_Quit, so it shouldn't cause any errors
=====================
*/
static void CL_SendDisconnect()
{
	// send a disconnect message to the server
	// send it a few times in case one is dropped
	if ( com_cl_running && com_cl_running->integer && cls.state >= connstate_t::CA_CONNECTED )
	{
		CL_AddReliableCommand( "disconnect" );
		CL_WritePacket();
		CL_WritePacket();
		CL_WritePacket();
	}
}

void CL_Disconnect( bool showMainMenu )
{
	if ( !com_cl_running || !com_cl_running->integer )
	{
		return;
	}

	CL_SendDisconnect();

#if defined(USE_MUMBLE)
	if ( cl_useMumble->integer && mumble_islinked() )
	{
		Log::Notice("Mumble: Unlinking from Mumble application" );
		mumble_unlink();
	}
#endif

	if ( clc.download )
	{
		FS_FCloseFile( clc.download );
		clc.download = 0;
	}

	*cls.downloadTempName = *cls.downloadName = 0;
	Cvar_Set( "cl_downloadName", "" );

	StopVideo();
	StopDemos();

	// allow cheats locally again
	if (showMainMenu) {
		Cvar::SetValueForce("sv_cheats", "1");
	}

	CL_ClearState();

	// wipe the client connection
	ResetStruct( clc );

	CL_ClearStaticDownload();

	FS::PakPath::ClearPaks();
	FS_LoadBasePak();

	// show_bug.cgi?id=589
	// don't try a restart if rocket is nullptr, as we might be in the middle of a restart already
	if ( cgvm.IsActive() && cls.state > connstate_t::CA_DISCONNECTED )
	{
		// restart the UI
		cls.state = connstate_t::CA_DISCONNECTED;

		// shutdown the UI
		CL_ShutdownCGame();

		// init the UI
		cgvm.Start();
		cgvm.CGameRocketInit();
	}
	else
	{
		cls.state = connstate_t::CA_DISCONNECTED;
	}

	CL_OnTeamChanged( 0 );
}

/*
===================
CL_ForwardCommandToServer

adds the current command line as a clientCommand
things like godmode, noclip, etc, are commands directed to the server,
so when they are typed in at the console, they will need to be forwarded.
===================
*/
void CL_ForwardCommandToServer( const char *string )
{
	const char *cmd = Cmd_Argv( 0 );

	// ignore key up commands
	if ( cmd[ 0 ] == '-' )
	{
		return;
	}

	if ( clc.demoplaying || cls.state < connstate_t::CA_CONNECTED || cmd[ 0 ] == '+' || cmd[ 0 ] == '\0' )
	{
		Log::Notice( "Unknown command \"%s\"", cmd );
		return;
	}

	if ( Cmd_Argc() > 1 )
	{
		CL_AddReliableCommand( string );
	}
	else
	{
		CL_AddReliableCommand( cmd );
	}
}

/*
======================================================================

CONSOLE COMMANDS

======================================================================
*/

/*
==================
CL_ForwardToServer_f
==================
*/
void CL_ForwardToServer_f()
{
	if ( cls.state != connstate_t::CA_ACTIVE || clc.demoplaying )
	{
		Log::Notice("Not connected to a server." );
		return;
	}

	// don't forward the first argument
	if ( Cmd_Argc() > 1 )
	{
		CL_AddReliableCommand( Cmd_Args() );
	}
}

/*
==================
CL_Disconnect_f
==================
*/
void CL_Disconnect_f()
{
	throw Sys::DropErr(false, "Disconnecting.");
}

/*
================
CL_Reconnect_f

================
*/
void CL_Reconnect_f()
{
	if ( !*cls.servername )
	{
		Log::Notice("Can't reconnect to nothing." );
	}
	else if ( !*cls.reconnectCmd )
	{
		Log::Notice("Can't reconnect to localhost." );
	}
	else
	{
		Cmd::BufferCommandTextAfter(cls.reconnectCmd, true);
	}
}

/*
================
CL_Connect_f

================
*/
void CL_Connect_f()
{
	char         *server;
	char password[ 64 ];
	char         *offset;
	int          argc = Cmd_Argc();
	netadrtype_t family = netadrtype_t::NA_UNSPEC;

	if ( argc != 2 && argc != 3 )
	{
		Cmd_PrintUsage("[-4|-6] <server>", nullptr);
		return;
	}

	if ( argc == 2 )
	{
		server = (char *) Cmd_Argv( 1 );
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

		server = (char *) Cmd_Argv( 2 );
	}

	// Skip the URI scheme.
	if ( !Q_strnicmp( server, URI_SCHEME, URI_SCHEME_LENGTH ) )
	{
		server += URI_SCHEME_LENGTH;
	}

	// Set and skip the password.
	if ( ( offset = strchr( server, '@' ) ) != nullptr )
	{
		Q_strncpyz( password, server, std::min( sizeof( password ), (size_t)( offset - server + 1 ) ) );
		Cvar_Set( "password", password );
		server = offset + 1;
	}

	if ( ( offset = strchr( server, '/' ) ) != nullptr )
	{
		// trailing slash, or path supplied - chop it off since we don't use it
		*offset = 0;
	}

	//Copy the arguments before they can be overwritten, after that server is invalid
	Q_strncpyz( cls.servername, server, sizeof( cls.servername ) );
	Q_strncpyz( cls.reconnectCmd, Cmd::GetCurrentArgs().EscapedArgs(0).c_str(), sizeof( cls.reconnectCmd ) );

	Audio::StopAllSounds(); // NERVE - SMF

	// clear any previous "server full" type messages
	clc.serverMessage[ 0 ] = 0;

	if ( com_sv_running->integer && !strcmp( server, "loopback" ) )
	{
		// if running a local server, kill it
		SV_Shutdown( "Server quit" );
	}

	// make sure a local server is killed
	Cvar_Set( "sv_killserver", "1" );
	SV_Frame( 0 );

	try {
		CL_Disconnect( true );
	} catch (Sys::DropErr& err) {
		Sys::Error( "CL_Disconnect error during /connect: %s", err.what() );
	}
	Con_Close();

	if ( !NET_StringToAdr( cls.servername, &clc.serverAddress, family ) )
	{
		Log::Notice("Bad server address" );
		cls.state = connstate_t::CA_DISCONNECTED;
		return;
	}

	if ( clc.serverAddress.port == 0 )
	{
		clc.serverAddress.port = UBigShort( PORT_SERVER );
	}

	std::string serverString = Net::AddressToString( clc.serverAddress, true );

	Log::Debug( "%s resolved to %s", cls.servername, serverString );

	// if we aren't playing on a LAN, we need to authenticate
	// with the cd key
	if ( NET_IsLocalAddress( clc.serverAddress ) )
	{
		cls.state = connstate_t::CA_CHALLENGING;
	}
	else
	{
		cls.state = connstate_t::CA_CONNECTING;
	}

	// we need to setup a correct default for this, otherwise the first val we set might reappear
	Cvar_Set( "com_errorMessage", "" );

	cls.keyCatchers = 0;
	clc.connectTime = -99999; // CL_CheckForResend() will fire immediately
	clc.connectPacketCount = 0;

	// server connection string
	Cvar_Set( "cl_currentServerAddress", server );
	Cvar_Set( "cl_currentServerIP", serverString.c_str() );
}


static Cvar::Cvar<std::string> cvar_rcon_client_destination(
    "rcon.client.destination",
    "Destination address for rcon commands, if empty the current server.",
    Cvar::NONE,
    ""
);

static Cvar::Cvar<std::string> cvar_rcon_client_password(
    "rcon.client.password",
    "Password used to protect the remote console",
    Cvar::NONE,
    ""
);

static Cvar::Range<Cvar::Cvar<int>> cvar_rcon_client_secure(
    "rcon.client.secure",
    "How secure the Rcon protocol should be: "
        "0: Allow unencrypted rcon, "
        "1: Require encryption, "
        "2: Require encryption and challenge check",
    Cvar::NONE,
    0,
    0,
    2
);

/*
 * Sends the message to the remote server
 */
static void CL_RconDeliver(const Rcon::Message &message)
{
    if ( message.secure == Rcon::Secure::Unencrypted )
    {
        Net::OutOfBandPrint(netsrc_t::NS_CLIENT, message.remote, "rcon %s %s", message.password, message.command);
        return;
    }

    std::string method = "PLAIN";
    Crypto::Data key = Crypto::Hash::Sha256(Crypto::FromString(message.password));
    std::string plaintext = message.command;

    if ( message.secure == Rcon::Secure::EncryptedChallenge )
    {
        method = "CHALLENGE";
        plaintext = message.challenge + ' ' + plaintext;
    }

    Crypto::Data cypher;
    if ( Crypto::Aes256Encrypt(Crypto::FromString(plaintext), key, cypher) )
    {
        Net::OutOfBandPrint(netsrc_t::NS_CLIENT, message.remote, "srcon %s %s",
            method,
            Crypto::ToString(Crypto::Encoding::Base64Encode(cypher))
        );
    }
}

static void CL_RconSend(const Rcon::Message &message)
{
	std::string invalid_reason;
	if ( message.Valid(&invalid_reason) )
	{
		CL_RconDeliver(message);
	}
	else
	{
		Log::Notice("Invalid rcon message: %s", invalid_reason.c_str());
	}
}

/*
 * Queue for commands that need a challenge from the server
 */
class RconMessageQueue
{
public:

	void Push(const Rcon::Message& message)
	{
		auto lock = std::unique_lock<std::mutex>(mutex);
		requests.push_back(message);
	}

	bool Empty() const
	{
		auto lock = std::unique_lock<std::mutex>(mutex);
		return requests.empty();
	}

	/*
	 * Pops a queued command and executes it whenever there's a matching challenge.
	 * Returns whether the command has been successful
	 */
	bool Pop(const netadr_t& server, const Str::StringRef& challenge)
	{
		auto lock = std::unique_lock<std::mutex>(mutex);

		if ( requests.empty() )
		{
			return false;
		}

		auto it = std::find_if(requests.begin(), requests.end(),
			[server](const Rcon::Message& message) {
				return NET_CompareAdr(message.remote, server);
			});

		if ( it != requests.end() )
		{
			it->challenge = challenge;
			CL_RconSend(*it);
			requests.erase(it);
			return true;
		}

		return false;
	}


private:
	std::vector<Rcon::Message> requests;
	mutable std::mutex mutex;
};

static netadr_t CL_RconDestinationAddress()
{
	if ( cls.state >= connstate_t::CA_CONNECTED )
	{
		return clc.netchan.remoteAddress;
	}

	netadr_t to;

	NET_StringToAdr( cvar_rcon_client_destination.Get().c_str(), &to, netadrtype_t::NA_UNSPEC );

	if ( to.port == 0 )
	{
		to.port = UBigShort( PORT_SERVER );
	}

	return to;
}

/*
  Send the rest of the command line over as
  an unconnected command.
*/
class RconCmd: public Cmd::StaticCmd
{
public:
	RconCmd():
		StaticCmd("rcon", Cmd::CLIENT, "Sends a remote console command")
	{}

	void Run(const Cmd::Args& args) const override
	{
		if ( cvar_rcon_client_password.Get().empty() )
		{
			Log::Notice("You must set '%s' before issuing an rcon command.",
				cvar_rcon_client_password.Name());
			return;
		}

		if ( cls.state < connstate_t::CA_CONNECTED && cvar_rcon_client_destination.Get().empty() )
		{
			Log::Notice( "Connect to a server or set the '%s' cvar to issue rcon commands",
				cvar_rcon_client_destination.Name()
			);

			return;
		}

		Rcon::Message message(
			CL_RconDestinationAddress(),
			args.EscapedArgs(1),
			Rcon::Secure(cvar_rcon_client_secure.Get()),
			cvar_rcon_client_password.Get()
		);

		if ( message.secure == Rcon::Secure::EncryptedChallenge )
		{
			queue.Push(message);
			Net::OutOfBandPrint(netsrc_t::NS_CLIENT, message.remote, "getchallenge");
		}
		else
		{
			CL_RconSend(message);
		}
	}

	/*
	 * If there's a command planned for execution on the given server, it will
	 * use the challenge to do so.
	 * Returns true if a command has been executed
	 */
	bool HandleChallenge(const netadr_t& server, const Str::StringRef& challenge)
	{
		return queue.Pop(server, challenge);
	}

private:
	static RconMessageQueue queue;
};

RconMessageQueue RconCmd::queue;
static RconCmd RconCmdRegistration;


/*
  Populates rcon.client cvars from the server
*/
class RconDiscoverCmd: public Cmd::StaticCmd
{
public:
	RconDiscoverCmd():
		StaticCmd("rconDiscover", Cmd::CLIENT, "Sends a request to the server to populate rcon.client cvars")
	{}

	void Run(const Cmd::Args&) const override
	{
		if ( cls.state < connstate_t::CA_CONNECTED && cvar_rcon_client_destination.Get().empty() )
		{
			Log::Notice( "Connect to a server or set the '%s' cvar to discover rcon settings",
				cvar_rcon_client_destination.Name()
			);
			return;
		}

		Net::OutOfBandPrint(netsrc_t::NS_CLIENT, CL_RconDestinationAddress(), "rconinfo");
	}
};

static RconDiscoverCmd RconDiscoverCmdRegistration;

static void CL_ServerRconInfoPacket( netadr_t, msg_t *msg )
{
	InfoMap info = InfoStringToMap( MSG_ReadString( msg ) );
	int value;
	if ( Str::ParseInt( value, info["secure"] ) )
	{
		cvar_rcon_client_secure.Set( value );
	}
}

static void CL_GetRSAKeysFileName( char *buffer, size_t size )
{
	Q_snprintf( buffer, size, "%s", RSAKEY_FILE );
}

static void CL_GenerateRSAKeys( const char *fileName )
{
	struct nettle_buffer key_buffer;
	fileHandle_t         f;

	mpz_set_ui( public_key.e, RSA_PUBLIC_EXPONENT );

	if ( !rsa_generate_keypair( &public_key, &private_key, nullptr, qnettle_random, nullptr, nullptr, RSA_KEY_LENGTH, 0 ) )
	{
		Sys::Error( "Error generating RSA keypair" );
	}

	nettle_buffer_init( &key_buffer );

	if ( !rsa_keypair_to_sexp( &key_buffer, nullptr, &public_key, &private_key ) )
	{
		Sys::Error( "Error converting RSA keypair to sexp" );
	}

	Log::Notice( "^5Regenerating RSA keypair; writing to %s" , fileName );

#ifndef _WIN32
	int old_umask = umask(0077);
#endif
	f = FS_FOpenFileWrite( fileName );
#ifndef _WIN32
	umask(old_umask);
#endif

	if ( !f )
	{
		Sys::Error( "Daemon could not open %s for writing the RSA keypair", RSAKEY_FILE );
	}

	FS_Write( key_buffer.contents, key_buffer.size, f );
	FS_FCloseFile( f );

	nettle_buffer_clear( &key_buffer );
	Log::Notice( "Daemon RSA keys generated" );
}

/*
===============
CL_LoadRSAKeys

Attempt to load the RSA keys from a file
If this fails then generate a new keypair
===============
*/
static void CL_LoadRSAKeys()
{
	int                  len;
	fileHandle_t         f;
	char                 fileName[ MAX_QPATH ];
	uint8_t              *buf;

	rsa_public_key_init( &public_key );
	rsa_private_key_init( &private_key );

	CL_GetRSAKeysFileName( fileName, sizeof( fileName ) );

	Log::Notice( "Loading RSA keys from %s" , fileName );

	len = FS_FOpenFileRead( fileName, &f );

	if ( !f || len < 1 )
	{
		Log::Notice( "^2%s", "Daemon RSA public-key file not found, regenerating\n" );
		CL_GenerateRSAKeys( fileName );
		return;
	}

	buf = (uint8_t*) Z_Malloc( len );
	FS_Read( buf, len, f );
	FS_FCloseFile( f );

	if ( !rsa_keypair_from_sexp( &public_key, &private_key, 0, len, buf ) )
	{
		Log::Notice( "^1%s", "Invalid RSA keypair in file, regenerating"  );
		Z_Free( buf );
		CL_GenerateRSAKeys( fileName );
		return;
	}

	Z_Free( buf );
	Log::Notice( "Daemon RSA public-key found." );
}

static void CL_ClearRSAKeys()
{
	rsa_private_key_clear( &private_key );
	rsa_public_key_clear( &public_key );
}


/*
=================
CL_Vid_Restart_f

Restart the video subsystem

we also have to reload the UI and CGame because the renderer
doesn't know what graphics to reload
=================
*/

#ifdef _WIN32
extern void IN_Restart();  // fretn

#endif

void CL_Vid_Restart_f()
{
	// settings may have changed so stop recording now
	StopVideo();

	// don't let them loop during the restart
	Audio::StopAllSounds();
	// shutdown the CGame
	CL_ShutdownCGame();
	// clear the font cache
	re.UnregisterFont( nullptr );
	cls.consoleFont = nullptr;
	// shutdown the renderer and clear the renderer interface
	CL_ShutdownRef();

	cls.rendererStarted = false;
	cls.soundRegistered = false;

	Hunk_Clear();

	// startup all the client stuff
	CL_StartHunkUsers();

	// start the cgame if connected
	if ( cls.state > connstate_t::CA_CONNECTED )
	{
		cls.cgameStarted = true;
		CL_InitCGame();
	}
}

/*
=================
CL_Snd_Restart_f

Restart the sound subsystem
The cgame must also be forced to restart because
handles will be invalid
=================
*/
void CL_Snd_Restart_f()
{
	Audio::Shutdown();

	if (!Audio::Init()) {
		Log::Warn("Couldn't initialize the audio subsystem.");
	}
	CL_Vid_Restart_f();
}

/*
==================
CL_Configstrings_f
==================
*/
void CL_Configstrings_f()
{
	int i;

	if ( cls.state != connstate_t::CA_ACTIVE )
	{
		Log::Notice("Not connected to a server." );
		return;
	}

	for ( i = 0; i < MAX_CONFIGSTRINGS; i++ )
	{
		if (cl.gameState[i].empty())
		{
			continue;
		}

		Log::Notice( "%4i: %s", i, cl.gameState[i].c_str() );
	}
}

/*
==============
CL_Clientinfo_f
==============
*/
void CL_Clientinfo_f()
{
	Log::Notice( "--------- Client Information ---------" );
	Log::Notice( "state: %s", Util::enum_str(cls.state));
	Log::Notice( "Server: %s", cls.servername );
	Log::Notice("User info settings:" );
	Info_Print( Cvar_InfoString( CVAR_USERINFO, false ) );
	Log::Notice("--------------------------------------" );
}


class DemoVideoCmd: public Cmd::StaticCmd
{
public:
    DemoVideoCmd()
        : Cmd::StaticCmd("demo_video", Cmd::CLIENT,
                         "Begins recording a video from the current demo")
    {}

    void Run(const Cmd::Args& args) const override
    {
        if ( args.size() > 2 )
        {
            PrintUsage(args, "[filename]", "Begins recording a video from the current demo");
            return;
        }

        if ( !clc.demoplaying )
        {
            Log::Notice("The demo_video command can only be used when playing back demos");
            return;
        }

        std::string filename;
        if ( args.size() == 2 )
        {
            // explicit filename
            filename = Str::Format( "videos/%s.avi", args[1] );
        }
        else
        {
            // scan for a free filename
            int i;
            for ( i = 0; i <= 9999; i++ )
            {

                filename = Str::Format("videos/%s-%04d.avi", clc.demoName, i);

                if ( !FS::HomePath::FileExists(filename) )
                {
                    break; // file doesn't exist
                }
            }

            if ( i > 9999 )
            {
                Log::Warn("no free file names to create video" );
                return;
            }
        }

        Log::Notice("Writing demo video to %s", filename);

        CL_OpenAVIForWriting( filename.c_str() );
    }
};
static DemoVideoCmd DemoVideoCmdRegistration;


class DemoStopVideoCmd: public Cmd::StaticCmd
{
public:
    DemoStopVideoCmd()
        : Cmd::StaticCmd("demo_video_stop", Cmd::CLIENT, "Stops recording a video")
    {}

    void Run(const Cmd::Args&) const override
    {
        CL_CloseAVI();
    }
};
static DemoStopVideoCmd DemoStopVideoCmdRegistration;


/*
=================
CL_CheckForResend

Resend a connect message if the last one has timed out
=================
*/
void CL_CheckForResend()
{
	int  port;
	char info[ MAX_INFO_STRING ];
	char data[ MAX_INFO_STRING ];

	// don't send anything if playing back a demo
	if ( clc.demoplaying )
	{
		return;
	}

	// resend if we haven't gotten a reply yet
	if ( cls.state != connstate_t::CA_CONNECTING && cls.state != connstate_t::CA_CHALLENGING )
	{
		return;
	}

	if ( cls.realtime - clc.connectTime < RETRANSMIT_TIMEOUT )
	{
		return;
	}

	clc.connectTime = cls.realtime; // for retransmit requests
	clc.connectPacketCount++;

	switch ( cls.state )
	{
		case connstate_t::CA_CONNECTING:
			Net::OutOfBandPrint( netsrc_t::NS_CLIENT, clc.serverAddress, "getchallenge" );
			break;

		case connstate_t::CA_CHALLENGING:
		{
			char key[ RSA_STRING_LENGTH ];

			mpz_get_str( key, 16, public_key.n);
			// sending back the challenge
			port = Cvar_VariableValue( "net_qport" );

			Q_strncpyz( info, Cvar_InfoString( CVAR_USERINFO, false ), sizeof( info ) );
			Info_SetValueForKey( info, "protocol", va( "%i", PROTOCOL_VERSION ), false );
			Info_SetValueForKey( info, "qport", va( "%i", port ), false );
			Info_SetValueForKey( info, "challenge", clc.challenge.c_str(), false );
			Info_SetValueForKey( info, "pubkey", key, false );

			Com_sprintf( data, sizeof(data), "connect %s", Cmd_QuoteString( info ) );

			Net::OutOfBandData( netsrc_t::NS_CLIENT, clc.serverAddress,
				reinterpret_cast<byte*>( data ), strlen( data ) );
			// the most current userinfo has been sent, so watch for any
			// newer changes to userinfo variables
			cvar_modifiedFlags &= ~CVAR_USERINFO;
			break;
		}
		default:
			Sys::Error( "CL_CheckForResend: bad cls.state" );
	}
}

/*
===================
CL_DisconnectPacket

Sometimes the server can drop the client and the netchan based
disconnect can be lost.  If the client continues to send packets
to the server, the server will send out of band disconnect packets
to the client so it doesn't have to wait for the full timeout period.
===================
*/
static void CL_DisconnectPacket( const netadr_t& from )
{
	if ( cls.state < connstate_t::CA_CONNECTING )
	{
		return;
	}

	// if not from our server, ignore it
	if ( !NET_CompareAdr( from, clc.netchan.remoteAddress ) )
	{
		return;
	}

	// if we have received packets within three seconds, ignore (it might be a malicious spoof)
	// NOTE TTimo:
	// there used to be a  clc.lastPacketTime = cls.realtime; line in CL_PacketEvent before calling CL_ConnectionlessPacket
	// therefore .. packets never got through this check, clients never disconnected
	// switched the clc.lastPacketTime = cls.realtime to happen after the connectionless packets have been processed
	// you still can't spoof disconnects, cause legal netchan packets will maintain realtime - lastPacketTime below the threshold
	if ( cls.realtime - clc.lastPacketTime < 3000 )
	{
		return;
	}

	// drop the connection
	const char* message = "Server disconnected for unknown reason";
	Cvar_Set( "com_errorMessage", message );
	Sys::Drop( message );
}

/*
===================
CL_PrintPacket
an OOB message from server, with potential markups
print OOB are the only messages we handle markups in
[err_dialog]: used to indicate that the connection should be aborted
  no further information, just do an error diagnostic screen afterwards
[err_prot]: HACK. This is a protocol error. The client uses a custom
  protocol error message (client sided) in the diagnostic window.
  The space for the error message on the connection screen is limited
  to 256 chars.
===================
*/
void CL_PrintPacket( msg_t *msg )
{
	char *s;

	s = MSG_ReadBigString( msg );

	if ( !Q_strnicmp( s, "[err_dialog]", 12 ) )
	{
		Q_strncpyz( clc.serverMessage, s + 12, sizeof( clc.serverMessage ) );
		Sys::Drop( "^3Server disconnected:\n^7%s", clc.serverMessage );
	}
	else
	{
		Q_strncpyz( clc.serverMessage, s, sizeof( clc.serverMessage ) );
	}

	Log::Notice("%s", clc.serverMessage );
}

/*
=================
CL_ConnectionlessPacket

Responses to broadcasts, etc
=================
*/
static void CL_ConnectionlessPacket( const netadr_t& from, msg_t *msg )
{
	MSG_BeginReadingOOB( msg );
	MSG_ReadLong( msg );  // skip the -1

	Cmd::Args args(MSG_ReadStringLine( msg ));

	if ( args.Argc() < 1 )
	{
		return;
	}

	Log::Debug( "CL packet %s: %s", Net::AddressToString( from, true ), args.Argv(0).c_str() );

	// challenge from the server we are connecting to

	if ( args.Argv(0) == "challengeResponse" )
	{
		if ( cls.state == connstate_t::CA_CONNECTING )
		{
			if ( args.Argc() < 2 )
			{
				return;
			}
			// start sending challenge response instead of challenge request packets
			clc.challenge = args.Argv(1);
			cls.state = connstate_t::CA_CHALLENGING;
			clc.connectPacketCount = 0;
			clc.connectTime = -99999;

			// take this address as the new server address.  This allows
			// a server proxy to hand off connections to multiple servers
			clc.serverAddress = from;
			Log::Debug( "challenge: %s", clc.challenge.c_str() );
		}
		else if ( !RconCmdRegistration.HandleChallenge(from, args.Argv(1)) )
		{
			Log::Notice( "Unwanted challenge response received.  Ignored." );
		}

		return;
	}

	// server connection
	if ( args.Argv(0) == "connectResponse" )
	{
		if ( cls.state >= connstate_t::CA_CONNECTED )
		{
			Log::Notice( "Dup connect received. Ignored." );
			return;
		}

		if ( cls.state != connstate_t::CA_CHALLENGING )
		{
			Log::Notice( "connectResponse packet while not connecting. Ignored." );
			return;
		}

		if ( !NET_CompareAdr( from, clc.serverAddress ) )
		{
			Log::Notice( "connectResponse from a different address. Ignored." );
			Log::Notice( "%s should have been %s", NET_AdrToString( from ),
			             Net::AddressToString( clc.serverAddress, true ) );
			return;
		}

		Netchan_Setup( netsrc_t::NS_CLIENT, &clc.netchan, from, Cvar_VariableValue( "net_qport" ) );
		cls.state = connstate_t::CA_CONNECTED;
		clc.lastPacketSentTime = -9999; // send first packet immediately
		return;
	}

	// server responding to an info broadcast
	if ( args.Argv(0) == "infoResponse" )
	{
		CL_ServerInfoPacket( from, msg );
		return;
	}

	// server responding to a get playerlist
	if ( args.Argv(0) == "statusResponse" )
	{
		CL_ServerStatusResponse( from, msg );
		return;
	}

	// a disconnect message from the server, which will happen if the server
	// dropped the connection but it is still getting packets from us
	if ( args.Argv(0) == "disconnect" )
	{
		CL_DisconnectPacket( from );
		return;
	}

	// echo request from server
	if ( args.Argv(0) == "echo" && args.Argc() >= 2)
	{
		Net::OutOfBandPrint( netsrc_t::NS_CLIENT, from, "%s", args.Argv(1) );
		return;
	}

	// echo request from server
	if ( args.Argv(0) == "print" )
	{
		CL_PrintPacket( msg );
		return;
	}

	// echo request from server
	if ( args.Argv(0) == "getserversResponse" )
	{
		CL_ServersResponsePacket( &from, msg, false );
		return;
	}

	// list of servers with both IPv4 and IPv6 addresses; sent back by a master server (extended)
	if ( args.Argv(0) == "getserversExtResponseLinks" )
	{
		CL_ServerLinksResponsePacket( msg );
		return;
	}

	// list of servers sent back by a master server (extended)
	if ( args.Argv(0) == "getserversExtResponse" )
	{
		CL_ServersResponsePacket( &from, msg, true );
		return;
	}

	// prints a n error message returned by the server
	if ( args.Argv(0) == "error" )
	{
		Log::Warn( MSG_ReadStringLine(msg) );
		return;
	}

	// prints a n error message returned by the server
	if ( args.Argv(0) == "rconInfoResponse" )
	{
		CL_ServerRconInfoPacket( from, msg );
		return;
	}

	Log::Debug( "Unknown connectionless packet command." );
}

/*
=================
CL_PacketEvent

A packet has arrived from the main event loop
=================
*/
void CL_PacketEvent( const netadr_t& from, msg_t *msg )
{
	int headerBytes;

	if ( msg->cursize >= 4 && * ( int * ) msg->data == -1 )
	{
		CL_ConnectionlessPacket( from, msg );
		return;
	}

	clc.lastPacketTime = cls.realtime;

	if ( cls.state < connstate_t::CA_CONNECTED )
	{
		return; // can't be a valid sequenced packet
	}

	if ( msg->cursize < 4 )
	{
		Log::Notice( "%s: Runt packet", Net::AddressToString( from, true ) );
		return;
	}

	//
	// packet from server
	//
	if ( !NET_CompareAdr( from, clc.netchan.remoteAddress ) )
	{
		Log::Debug( "%s: sequenced packet without connection", Net::AddressToString( from, true ) );
		// FIXME: send a client disconnect?
		return;
	}

	if ( !Netchan_Process( &clc.netchan, msg ) )
	{
		return; // out of order, duplicated, etc
	}

	// the header is different lengths for reliable and unreliable messages
	headerBytes = msg->readcount;

	// track the last message received so it can be returned in
	// client messages, allowing the server to detect a dropped
	// gamestate
	clc.serverMessageSequence = LittleLong( * ( int * ) msg->data );

	clc.lastPacketTime = cls.realtime;
	CL_ParseServerMessage( msg );

	//
	// we don't know if it is ok to save a demo message until
	// after we have parsed the frame
	//

	if ( clc.demorecording && !clc.demowaiting )
	{
		CL_WriteDemoMessage( msg, headerBytes );
	}
}

/*
==================
CL_CheckTimeout

==================
*/
void CL_CheckTimeout()
{
	//
	// check timeout
	//
	if ( cls.state >= connstate_t::CA_CONNECTED && cls.realtime - clc.lastPacketTime > cl_timeout->value * 1000 )
	{
		if ( ++cl.timeoutcount > 5 )
		{
			// timeoutcount saves debugger
			const char* message = "Server connection timed out.";
			Cvar_Set( "com_errorMessage", message );
			Sys::Drop( message );
		}
	}
	else
	{
		cl.timeoutcount = 0;
	}
}

//============================================================================

/*
==================
CL_CheckUserinfo

==================
*/
void CL_CheckUserinfo()
{
	// don't add reliable commands when not yet connected
	if ( cls.state < connstate_t::CA_CHALLENGING )
	{
		return;
	}

	// send a reliable userinfo update if needed
	if ( cvar_modifiedFlags & CVAR_USERINFO )
	{
		cvar_modifiedFlags &= ~CVAR_USERINFO;
		CL_AddReliableCommand( va( "userinfo %s", Cmd_QuoteString( Cvar_InfoString( CVAR_USERINFO, false ) ) ) );
	}
}

/*
==================
CL_Frame

==================
*/
void CL_Frame( int msec )
{
	if ( !com_cl_running->integer )
	{
		return;
	}

	// if recording an avi, lock to a fixed fps
	if ( CL_VideoRecording() && cl_aviFrameRate->integer && msec )
	{
		// save the current screen
		if ( cls.state == connstate_t::CA_ACTIVE )
		{
			CL_TakeVideoFrame();

			// fixed time for next frame
			msec = ( int ) ceil( ( 1000.0f / cl_aviFrameRate->value ) * com_timescale->value );

			if ( msec == 0 )
			{
				msec = 1;
			}
		}

		// XreaL END
	}

	// save the msec before checking pause
	cls.realFrametime = msec;

	// decide the simulation time
	cls.frametime = msec;

	cls.realtime += cls.frametime;

	// see if we need to update any userinfo
	CL_CheckUserinfo();

	// if we haven't gotten a packet in a long time,
	// drop the connection
	CL_CheckTimeout();

	// wwwdl download may survive a server disconnect
	if ( cls.state == connstate_t::CA_DOWNLOADING && clc.bWWWDl )
	{
		CL_WWWDownload();
	}

	// send intentions now
	CL_SendCmd();

	// resend a connection request if necessary
	CL_CheckForResend();

	// decide on the serverTime to render
	CL_SetCGameTime();

	// update the screen
	SCR_UpdateScreen();

	// update the sound
	Audio::Update();

#if defined(USE_MUMBLE)
	CL_UpdateMumble();
#endif

	Con_RunConsole();

	cls.framecount++;
}

static bool CL_InitRef();
/*
============
CL_InitRenderer
============
*/
bool CL_InitRenderer()
{
	if ( !CL_InitRef() )
	{
		return false;
	}

	fileHandle_t f;

	// this sets up the renderer and calls R_Init
	if ( !re.BeginRegistration( &cls.glconfig, &cls.glconfig2 ) )
	{
		return false;
	}

	cl_consoleFont = Cvar_Get( "cl_consoleFont", "fonts/unifont.ttf",  CVAR_LATCH );
	cl_consoleFontSize = Cvar_Get( "cl_consoleFontSize", "16",  CVAR_LATCH );
	cl_consoleFontScaling = Cvar_Get( "cl_consoleFontScaling", "1", CVAR_LATCH );

	// load character sets
	cls.charSetShader = re.RegisterShader( "gfx/2d/bigchars", RSF_2D );
	cls.useLegacyConsoleFont = cls.useLegacyConsoleFace = true;

	// Register console font specified by cl_consoleFont, if any
	// filehandle is unused but forces FS_FOpenFileRead() to heed purecheck because it does not when filehandle is nullptr
	if ( cl_consoleFont->string[0] )
	{
		if ( FS_FOpenFileRead( cl_consoleFont->string, &f ) >= 0 )
		{
			if ( cl_consoleFontScaling->value == 0 )
			{
				cls.consoleFont = re.RegisterFont( cl_consoleFont->string, cl_consoleFontSize->integer );
			}
			else
			{
				// This gets 12px on 1920×1080 screen, which is libRocket default for 1em
				int fontScale = std::min(cls.glconfig.vidWidth, cls.glconfig.vidHeight) / 90;

				// fontScale / 12px gets 1px on 1920×1080 screen
				cls.consoleFont = re.RegisterFont( cl_consoleFont->string, cl_consoleFontSize->integer * fontScale / 12 );
			}

			if ( cls.consoleFont != nullptr )
				cls.useLegacyConsoleFont = false;
		}
		else
		{
			Log::Warn("Font file '%s' not found", cl_consoleFont->string);
		}

		FS_FCloseFile( f );
	}

	cls.whiteShader = re.RegisterShader( "white", RSF_NOMIP );

	g_console_field_width = cls.glconfig.vidWidth / SMALLCHAR_WIDTH - 2;
	g_consoleField.SetWidth(g_console_field_width);

	return true;
}

/*
============================
CL_StartHunkUsers

Starts any of {renderer, audio, cgame vm} that is not already started.
Only the renderer is really a hunk user.
============================
*/
void CL_StartHunkUsers()
{
	if ( !com_cl_running )
	{
		return;
	}

	if ( !com_cl_running->integer )
	{
		return;
	}

	if ( !cls.rendererStarted && CL_InitRenderer() )
	{
		cls.rendererStarted = true;
	}

	if ( !cls.rendererStarted )
	{
		CL_ShutdownRef();
		Sys::Error( "Couldn't load a renderer." );
	}

	if ( !Audio::Init() ) {
		Log::Warn("Couldn't initialize the audio subsystem.");
	}

	if ( !cls.soundRegistered )
	{
		cls.soundRegistered = true;
		//TODO
		//S_BeginRegistration();
	}

	if ( !cgvm.IsActive() )
	{
		cgvm.Start();
		cgvm.CGameRocketInit();
	}
}

int CL_ScaledMilliseconds()
{
	return Sys::Milliseconds() * com_timescale->value;
}

extern refexport_t *GetRefAPI( int apiVersion, refimport_t *rimp );

/*
============
CL_InitRef
============
*/
static bool CL_InitRef()
{
	refimport_t ri;
	refexport_t *ret;

	ri.Milliseconds = Sys::Milliseconds;
	ri.RealTime = Com_RealTime;

	ri.Hunk_Alloc = Hunk_Alloc;
	ri.Hunk_AllocateTempMemory = Hunk_AllocateTempMemory;
	ri.Hunk_FreeTempMemory = Hunk_FreeTempMemory;

	ri.FS_ReadFile = FS_ReadFile;
	ri.FS_FreeFile = FS_FreeFile;
	ri.FS_WriteFile = FS_WriteFile;
	ri.FS_FreeFileList = FS_FreeFileList;
	ri.FS_ListFiles = FS_ListFiles;
	ri.FS_Seek = FS_Seek;
	ri.FS_FTell = FS_FTell;
	ri.FS_Read = FS_Read;
	ri.FS_FCloseFile = FS_FCloseFile;
	ri.FS_FOpenFileRead = FS_FOpenFileRead;

	// XreaL BEGIN
	ri.CL_VideoRecording = CL_VideoRecording;
	ri.CL_WriteAVIVideoFrame = CL_WriteAVIVideoFrame;
	// XreaL END

	ri.IN_Init = IN_Init;
	ri.IN_Shutdown = IN_Shutdown;
	ri.IN_Restart = IN_Restart;

	Log::Notice("Calling GetRefAPI…" );
	ret = GetRefAPI( REF_API_VERSION, &ri );

	if ( !ret )
	{
		Log::Notice( "Couldn't initialize refresh module" );
		return false;
	}

	re = *ret;

	return true;
}

/*
============
CL_ShutdownRef
============
*/
void CL_ShutdownRef()
{
	if ( !re.Shutdown )
	{
		return;
	}

	re.Shutdown( true );
	re = {};
}

//===========================================================================================

/*
====================
CL_Init
====================
*/
static Cvar::Cvar<int> cvar_snaps(
	"snaps", "snapshots per second that the client wants from the server", Cvar::USERINFO, 40);
static Cvar::Cvar<std::string> cvar_password(
	"password", "client's password to get into the server", Cvar::USERINFO, "");
static Cvar::Cvar<std::string> cvar_name(
	"name", "player display name", Cvar::USERINFO | Cvar::ARCHIVE, UNNAMED_PLAYER);
void CL_Init()
{
	PrintBanner( "Client Initialization" )

	Con_Init();

	CL_ClearState();

	cls.state = connstate_t::CA_DISCONNECTED; // no longer CA_UNINITIALIZED

	cls.realtime = 0;

	CL_InitInput();

	CL_LoadRSAKeys();

	//
	// register our variables
	//
	cl_noprint = Cvar_Get( "cl_noprint", "0", 0 );

	cl_timeout = Cvar_Get( "cl_timeout", "200", 0 );

	cl_timeNudge = Cvar_Get( "cl_timeNudge", "0", CVAR_TEMP );
	cl_shownet = Cvar_Get( "cl_shownet", "0", CVAR_TEMP );
	cl_showSend = Cvar_Get( "cl_showSend", "0", CVAR_TEMP );
	cl_showTimeDelta = Cvar_Get( "cl_showTimeDelta", "0", CVAR_TEMP );
	cl_activeAction = Cvar_Get( "activeAction", "", CVAR_TEMP );
	cl_autorecord = Cvar_Get( "cl_autorecord", "0", CVAR_TEMP );

	cl_aviFrameRate = Cvar_Get( "cl_aviFrameRate", "25", 0 );

	// XreaL BEGIN
	cl_aviMotionJpeg = Cvar_Get( "cl_aviMotionJpeg", "1", 0 );
	// XreaL END

	cl_yawspeed = Cvar_Get( "cl_yawspeed", "140", 0 );
	cl_pitchspeed = Cvar_Get( "cl_pitchspeed", "140", 0 );
	cl_anglespeedkey = Cvar_Get( "cl_anglespeedkey", "1.5", 0 );

	cl_maxpackets = Cvar_Get( "cl_maxpackets", "125", 0 );
	cl_packetdup = Cvar_Get( "cl_packetdup", "1", 0 );

	cl_run = Cvar_Get( "cl_run", "1", 0 );
	cl_sensitivity = Cvar_Get( "sensitivity", "5", CVAR_ARCHIVE );
	cl_mouseAccel = Cvar_Get( "cl_mouseAccel", "0", 0 );
	cl_freelook = Cvar_Get( "cl_freelook", "1", CVAR_ARCHIVE );

	cl_gameControllerAvailable = Cvar_Get( "in_gameControllerAvailable", "0", CVAR_ROM );

	// 0: legacy mouse acceleration
	// 1: new implementation

	cl_mouseAccelStyle = Cvar_Get( "cl_mouseAccelStyle", "0", 0 );
	// offset for the power function (for style 1, ignored otherwise)
	// this should be set to the max rate value
	cl_mouseAccelOffset = Cvar_Get( "cl_mouseAccelOffset", "5", 0 );

	cl_showMouseRate = Cvar_Get( "cl_showmouserate", "0", 0 );

	cl_allowDownload = Cvar_Get( "cl_allowDownload", "1", 0 );

	cl_doubletapdelay = Cvar_Get( "cl_doubletapdelay", "250", 0 );  // Arnout: double tap
	m_pitch = Cvar_Get( "m_pitch", "0.022", CVAR_ARCHIVE );
	m_yaw = Cvar_Get( "m_yaw", "0.022", 0 );
	m_forward = Cvar_Get( "m_forward", "0.25", 0 );
	m_side = Cvar_Get( "m_side", "0.25", 0 );
	m_filter = Cvar_Get( "m_filter", "0", CVAR_ARCHIVE );

	j_pitch = Cvar_Get( "j_pitch", "0.022", 0 );
	j_yaw = Cvar_Get( "j_yaw", "-0.022", 0 );
	j_forward = Cvar_Get( "j_forward", "-0.25", 0 );
	j_side = Cvar_Get( "j_side", "0.25", 0 );
	j_up = Cvar_Get ("j_up", "1", 0);

	j_pitch_axis = Cvar_Get( "j_pitch_axis", "3", 0 );
	j_yaw_axis = Cvar_Get( "j_yaw_axis", "4", 0 );
	j_forward_axis = Cvar_Get( "j_forward_axis", "1", 0 );
	j_side_axis = Cvar_Get( "j_side_axis", "0", 0 );
	j_up_axis = Cvar_Get( "j_up_axis", "2", 0 );

	cl_consoleFontKerning = Cvar_Get( "cl_consoleFontKerning", "0", 0 );

	cl_consoleCommand = Cvar_Get( "cl_consoleCommand", "say", 0 );

	cl_altTab = Cvar_Get( "cl_altTab", "1", 0 );

	// userinfo
	cl_rate = Cvar_Get( "rate", "25000", CVAR_USERINFO | CVAR_ARCHIVE );

#if defined(USE_MUMBLE)
	cl_useMumble = Cvar_Get( "cl_useMumble", "0",  CVAR_LATCH );
	cl_mumbleScale = Cvar_Get( "cl_mumbleScale", "0.0254", 0 );
#endif

	//
	// register our commands
	//
	Cmd_AddCommand( "cmd", CL_ForwardToServer_f );
	Cmd_AddCommand( "configstrings", CL_Configstrings_f );
	Cmd_AddCommand( "clientinfo", CL_Clientinfo_f );
	Cmd_AddCommand( "snd_restart", CL_Snd_Restart_f );
	Cmd_AddCommand( "vid_restart", CL_Vid_Restart_f );
	Cmd_AddCommand( "disconnect", CL_Disconnect_f );
	Cmd_AddCommand( "connect", CL_Connect_f );
	Cmd_AddCommand( "reconnect", CL_Reconnect_f );
	Cmd_AddCommand( "localservers", CL_LocalServers_f );
	Cmd_AddCommand( "globalservers", CL_GlobalServers_f );

	Cmd_AddCommand( "ping", CL_Ping_f );
	Cmd_AddCommand( "serverstatus", CL_ServerStatus_f );

	Cmd_AddCommand( "updatescreen", SCR_UpdateScreen );
	// done.

	SCR_Init();

	Cmd::ExecuteCommandBuffer();

	Cvar_Set( "cl_running", "1" );

	PrintBanner( "Client Initialization Complete" )
}

/*
===============
CL_Shutdown

Called only when Daemon is exiting
===============
*/
void CL_Shutdown()
{
	static bool recursive = false;

	// check whether the client is running at all.
	if ( !( com_cl_running && com_cl_running->integer ) )
	{
		return;
	}

	Log::Debug( "----- CL_Shutdown -----" );
	if ( recursive )
	{
		printf( "recursive shutdown\n" );
		return;
	}

	recursive = true;

	// quick version
	if ( !Sys::PedanticShutdown() )
	{
		CL_ShutdownCGame();
		CL_SendDisconnect();
		CL_StopRecord();
		StopVideo();
		// TODO: call DL_StopDownload when deleting the temp file is implemented
		return;
	}

	CL_Disconnect( true );

	CL_ShutdownCGame();

	Audio::Shutdown();
	DL_Shutdown();

	if ( re.UnregisterFont )
	{
		re.UnregisterFont( nullptr );
		cls.consoleFont = nullptr;
	}

	CL_ShutdownRef();

	Cmd_RemoveCommand( "cmd" );
	Cmd_RemoveCommand( "configstrings" );
	Cmd_RemoveCommand( "snd_restart" );
	Cmd_RemoveCommand( "vid_restart" );
	Cmd_RemoveCommand( "disconnect" );
	Cmd_RemoveCommand( "connect" );
	Cmd_RemoveCommand( "localservers" );
	Cmd_RemoveCommand( "globalservers" );
	Cmd_RemoveCommand( "ping" );
	Cmd_RemoveCommand( "serverstatus" );

	CL_ClearKeyBinding();
	CL_ClearInput();

	CL_ClearRSAKeys();

	// done.

	Cvar_Set( "cl_running", "0" );

	recursive = false;

	// do not leak.
	ResetStruct( cls );

	Log::Debug( "-----------------------" );

}

/*
====================
CL_GetClipboardData
====================
*/
#ifdef BUILD_GRAPHICAL_CLIENT
void CL_GetClipboardData( char *buf, int buflen )
{
	int         i, j;
	char       *cbd = SDL_GetClipboardText();
	const char *clean;

	if ( !cbd )
	{
		*buf = 0;
		return;
	}

	clean = Com_ClearForeignCharacters( cbd ); // yes, I know
	SDL_free( cbd );

	i = j = 0;
	while ( clean[ i ] )
	{
		if ( (unsigned char)clean[ i ] < ' ' || clean[ i ] == 0x7F )
		{
			if ( j + 1 >= buflen )
			{
				break;
			}

			i++;
			buf[ j++ ] = ' ';
		}
		else
		{
			int w = Q_UTF8_Width( clean + i );

			if ( j + w >= buflen )
			{
				break;
			}

			switch ( w )
			{
			case 4: buf[ j++ ] = clean[ i++ ]; DAEMON_FALLTHROUGH;
			case 3: buf[ j++ ] = clean[ i++ ]; DAEMON_FALLTHROUGH;
			case 2: buf[ j++ ] = clean[ i++ ]; DAEMON_FALLTHROUGH;
			case 1: buf[ j++ ] = clean[ i++ ];
			}
		}
	}

	buf[ j ] = '\0';
}
#endif
