/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2013-2016, Daemon Developers
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Daemon developers nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL DAEMON DEVELOPERS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
===========================================================================
*/

#include <engine/client/cg_msgdef.h>
#include <shared/VMMain.h>
#include <shared/CommandBufferClient.h>
#include "cg_api.h"

IPC::CommandBufferClient cmdBuffer("cgame");

// Definition of the VM->Engine calls

// All Miscs

void trap_SendClientCommand( const char *s )
{
	VM::SendMsg<SendClientCommandMsg>(s);
}

void trap_UpdateScreen()
{
	VM::SendMsg<UpdateScreenMsg>();
}

int trap_CM_MarkFragments( int numPoints, const vec3_t *points, const vec3_t projection, int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t *fragmentBuffer )
{
	if (!numPoints) return 0;

	std::vector<std::array<float, 3>> mypoints(numPoints);
	std::array<float, 3> myproj;
	memcpy(mypoints.data(), points, sizeof(float) * 3 * numPoints);
	VectorCopy(projection, myproj);

	std::vector<std::array<float, 3>> mypointBuffer;
	std::vector<markFragment_t> myfragmentBuffer;
	VM::SendMsg<CMMarkFragmentsMsg>(mypoints, myproj, maxPoints, maxFragments, mypointBuffer, myfragmentBuffer);

	memcpy(pointBuffer, mypointBuffer.data(), sizeof(float) * 3 * maxPoints);
	std::copy(myfragmentBuffer.begin(), myfragmentBuffer.end(), fragmentBuffer);
	return myfragmentBuffer.size();
}

void trap_CM_BatchMarkFragments(
	unsigned maxPoints, //per mark
	unsigned maxFragments, //per mark
	const std::vector<markMsgInput_t> &markMsgInput,
	std::vector<markMsgOutput_t> &markMsgOutput )
{
	if (!markMsgInput.empty())
	{
		VM::SendMsg<CMBatchMarkFragments>(maxPoints, maxFragments, markMsgInput, markMsgOutput);
	}
}

void trap_GetCurrentSnapshotNumber( int *snapshotNumber, int *serverTime )
{
	VM::SendMsg<GetCurrentSnapshotNumberMsg>(*snapshotNumber, *serverTime);
}

bool trap_GetSnapshot( int snapshotNumber, ipcSnapshot_t *snapshot )
{
	bool res;
	VM::SendMsg<GetSnapshotMsg>(snapshotNumber, res, *snapshot);
	return res;
}

int trap_GetCurrentCmdNumber()
{
	int res;
	VM::SendMsg<GetCurrentCmdNumberMsg>(res);
	return res;
}

// Use a local cache to reduce IPC traffic. We assume that the user command number never decreases as
// the command number only resets on an svc_gamestate command, and this command is only sent once
// per cgame lifetime when the user enters the game (it's not even used for map_restart).
bool trap_GetUserCmd( int cmdNumber, usercmd_t *ucmd )
{
	static usercmd_t cache[ CMD_BACKUP ];
	static int latestInCache = -1;

	if ( cmdNumber <= latestInCache - CMD_BACKUP )
	{
		// Either the cgame is buggy and trying to request really old stuff, or there is some case
		// of additional calls to CL_ClearState that I didn't know about. Reset the cache in case
		// it's legit.
		Log::Warn( "Unexpectedly old command number requested in trap_GetUserCmd" );
		latestInCache = -1;
	}
	else if ( cmdNumber <= latestInCache )
	{
		*ucmd = cache[ cmdNumber & CMD_MASK ];
		return true;
	}

	bool res;
	VM::SendMsg<GetUserCmdMsg>(cmdNumber, res, *ucmd);
	if ( res )
	{
		latestInCache = cmdNumber;
		cache[ cmdNumber & CMD_MASK ] = *ucmd;
	}
	return res;
}

void trap_SetUserCmdValue( int stateValue, int flags, float sensitivityScale )
{
	VM::SendMsg<SetUserCmdValueMsg>(stateValue, flags, sensitivityScale);
}

bool trap_GetEntityToken( char *buffer, int bufferSize )
{
	bool res;
	std::string token;
	VM::SendMsg<CgGetEntityTokenMsg>(bufferSize, res, token);
	Q_strncpyz(buffer, token.c_str(), bufferSize);
	return res;
}

void trap_RegisterButtonCommands( const char *cmds )
{
	VM::SendMsg<RegisterButtonCommandsMsg>(cmds);
}

void trap_QuoteString( const char *str, char *buffer, int size )
{
	std::string quoted;
	VM::SendMsg<QuoteStringMsg>(size, str, quoted);
	Q_strncpyz(buffer, quoted.c_str(), size);
}

void trap_notify_onTeamChange( int newTeam )
{
	VM::SendMsg<NotifyTeamChangeMsg>(newTeam);
}

void trap_PrepareKeyUp()
{
	VM::SendMsg<PrepareKeyUpMsg>();
}

// All Sounds

void trap_S_StartSound( vec3_t origin, int entityNum, soundChannel_t, sfxHandle_t sfx )
{
    Vec3 myorigin = Vec3(0.0f, 0.0f, 0.0f);
	if (origin) {
        myorigin = Vec3::Load(origin);
	}
	cmdBuffer.SendMsg<Audio::StartSoundMsg>(!!origin, myorigin, entityNum, sfx);
}

void trap_S_StartLocalSound( sfxHandle_t sfx, soundChannel_t )
{
	cmdBuffer.SendMsg<Audio::StartLocalSoundMsg>(sfx);
}

void trap_S_ClearLoopingSounds( bool )
{
	cmdBuffer.SendMsg<Audio::ClearLoopingSoundsMsg>();
}

void trap_S_AddLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx )
{
	if (origin) {
		trap_S_UpdateEntityPosition(entityNum, origin);
	}
	if (velocity) {
		trap_S_UpdateEntityVelocity(entityNum, velocity);
	}
	cmdBuffer.SendMsg<Audio::AddLoopingSoundMsg>(entityNum, sfx);
}

void trap_S_AddRealLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx )
{
	trap_S_AddLoopingSound(entityNum, origin, velocity, sfx);
}

void trap_S_StopLoopingSound( int entityNum )
{
	cmdBuffer.SendMsg<Audio::StopLoopingSoundMsg>(entityNum);
}

void trap_S_UpdateEntityPosition( int entityNum, const vec3_t origin )
{
	cmdBuffer.SendMsg<Audio::UpdateEntityPositionMsg>(entityNum, Vec3::Load(origin));
}

void trap_S_Respatialize( int entityNum, const vec3_t origin, vec3_t axis[ 3 ], int )
{
	if (origin) {
		trap_S_UpdateEntityPosition(entityNum, origin);
	}
	std::array<Vec3, 3> myaxis;
    myaxis[0] = Vec3::Load(axis[0]);
    myaxis[1] = Vec3::Load(axis[1]);
    myaxis[2] = Vec3::Load(axis[2]);
	cmdBuffer.SendMsg<Audio::RespatializeMsg>(entityNum, myaxis);
}

sfxHandle_t trap_S_RegisterSound( const char *sample, bool)
{
	int sfx;
	VM::SendMsg<Audio::RegisterSoundMsg>(sample, sfx);
	return sfx;
}

void trap_S_StartBackgroundTrack( const char *intro, const char *loop )
{
	cmdBuffer.SendMsg<Audio::StartBackgroundTrackMsg>(intro, loop);
}

void trap_S_StopBackgroundTrack()
{
	cmdBuffer.SendMsg<Audio::StopBackgroundTrackMsg>();
}

void trap_S_UpdateEntityVelocity( int entityNum, const vec3_t velocity )
{
	cmdBuffer.SendMsg<Audio::UpdateEntityVelocityMsg>(entityNum, Vec3::Load(velocity));
}

void trap_S_UpdateEntityPositionVelocity( int entityNum, const vec3_t origin, const vec3_t velocity )
{
	cmdBuffer.SendMsg<Audio::UpdateEntityPositionVelocityMsg>(entityNum, Vec3::Load(origin), Vec3::Load(velocity));
}

void trap_S_SetReverb( int slotNum, const char* name, float ratio )
{
	cmdBuffer.SendMsg<Audio::SetReverbMsg>(slotNum, name, ratio);
}

void trap_S_BeginRegistration()
{
	cmdBuffer.SendMsg<Audio::BeginRegistrationMsg>();
}

void trap_S_EndRegistration()
{
	cmdBuffer.SendMsg<Audio::EndRegistrationMsg>();
}

// All renderer

void trap_R_SetAltShaderTokens( const char *str )
{
	VM::SendMsg<Render::SetAltShaderTokenMsg>(str);
}

void trap_R_GetShaderNameFromHandle( const qhandle_t shader, char *out, int len )
{
	std::string result;
	VM::SendMsg<Render::GetShaderNameFromHandleMsg>(shader, result);
	Q_strncpyz(out, result.c_str(), len);
}

void trap_R_ScissorEnable( bool enable )
{
    cmdBuffer.SendMsg<Render::ScissorEnableMsg>(enable);
}

void trap_R_ScissorSet( int x, int y, int w, int h )
{
	cmdBuffer.SendMsg<Render::ScissorSetMsg>(x, y, w, h);
}

bool trap_R_inPVVS( const vec3_t p1, const vec3_t p2 )
{
	bool res;
	std::array<float, 3> myp1, myp2;
	VectorCopy(p1, myp1);
	VectorCopy(p2, myp2);
	VM::SendMsg<Render::InPVVSMsg>(myp1, myp2, res);
	return res;
}

void trap_R_LoadWorldMap( const char *mapname )
{
	VM::SendMsg<Render::LoadWorldMapMsg>(mapname);
}

qhandle_t trap_R_RegisterModel( const char *name )
{
	int handle;
	VM::SendMsg<Render::RegisterModelMsg>(name, handle);
	return handle;
}

qhandle_t trap_R_RegisterSkin( const char *name )
{
	int handle;
	VM::SendMsg<Render::RegisterSkinMsg>(name, handle);
	return handle;
}

qhandle_t trap_R_RegisterShader( const char *name, int flags )
{
	int handle;
	VM::SendMsg<Render::RegisterShaderMsg>(name, flags, handle);
	return handle;
}

void trap_R_ClearScene()
{
	cmdBuffer.SendMsg<Render::ClearSceneMsg>();
}

void trap_R_AddRefEntityToScene( const refEntity_t *re )
{
	cmdBuffer.SendMsg<Render::AddRefEntityToSceneMsg>(*re);
}

void trap_R_AddPolyToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts )
{
	if (!numVerts)
	{
		return;
	}

	std::vector<polyVert_t> myverts(verts, verts + numVerts);
	cmdBuffer.SendMsg<Render::AddPolyToSceneMsg>(hShader, myverts);
}

void trap_R_AddPolysToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts, int numPolys )
{
	size_t size = numVerts * numPolys;

	if (!size)
	{
		return;
	}

	std::vector<polyVert_t> myverts(verts, verts + size);

	cmdBuffer.SendMsg<Render::AddPolysToSceneMsg>(hShader, myverts, numVerts, numPolys);
}

void trap_R_Add2dPolysIndexedToScene( const polyVert_t* polys, int numPolys, const int* indexes, int numIndexes, int trans_x, int trans_y, qhandle_t shader )
{
	if (!numIndexes)
	{
		return;
	}

	std::vector<polyVert_t> mypolys(polys, polys + numPolys);
	std::vector<int> myindices(indexes, indexes + numIndexes);
	cmdBuffer.SendMsg<Render::Add2dPolysIndexedMsg>(mypolys, numPolys, myindices, numIndexes, trans_x, trans_y, shader);
}

// Used exclusively for RmlUI's transformations. Other usecases might
// not work as expected.
void trap_R_SetMatrixTransform( const matrix_t matrix )
{
	std::array<float, 16> mymatrix;
	MatrixCopy(matrix, mymatrix.data());
	cmdBuffer.SendMsg<Render::SetMatrixTransformMsg>(mymatrix);
}

// Used exclusively for RmlUI's transformations. Other usecases might
// not work as expected.
void trap_R_ResetMatrixTransform()
{
	cmdBuffer.SendMsg<Render::ResetMatrixTransformMsg>();
}

void trap_R_AddLightToScene( const vec3_t org, float radius, float intensity, float r, float g, float b, qhandle_t hShader, int flags )
{
	std::array<float, 3> myorg;
	VectorCopy(org, myorg);
	cmdBuffer.SendMsg<Render::AddLightToSceneMsg>(myorg, radius, intensity, r, g, b, hShader, flags);
}

void trap_R_AddAdditiveLightToScene( const vec3_t org, float intensity, float r, float g, float b )
{
	std::array<float, 3> myorg;
	VectorCopy(org, myorg);
	cmdBuffer.SendMsg<Render::AddAdditiveLightToSceneMsg>(myorg, intensity, r, g, b);
}

void trap_R_RenderScene( const refdef_t *fd )
{
	cmdBuffer.SendMsg<Render::RenderSceneMsg>(*fd);
}

void trap_R_ClearColor()
{
	cmdBuffer.SendMsg<Render::SetColorMsg>(Color::White);
}
void trap_R_SetColor( const Color::Color &rgba )
{
	cmdBuffer.SendMsg<Render::SetColorMsg>(rgba);
}
void trap_R_SetClipRegion( const float *region )
{
	std::array<float, 4> myregion;
	Vector4Copy(region, myregion);
	cmdBuffer.SendMsg<Render::SetClipRegionMsg>(myregion);
}

void trap_R_ResetClipRegion()
{
	cmdBuffer.SendMsg<Render::ResetClipRegionMsg>();
}

void trap_R_DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader )
{
	cmdBuffer.SendMsg<Render::DrawStretchPicMsg>(x, y, w, h, s1, t1, s2, t2, hShader);
}

void trap_R_DrawRotatedPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader, float angle )
{
	cmdBuffer.SendMsg<Render::DrawRotatedPicMsg>(x, y, w, h, s1, t1, s2, t2, hShader, angle);
}

void trap_R_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs )
{
	std::array<float, 3> mymins, mymaxs;
	VM::SendMsg<Render::ModelBoundsMsg>(model, mymins, mymaxs);
	VectorCopy(mymins, mins);
	VectorCopy(mymaxs, maxs);
}

int trap_R_LerpTag( orientation_t *tag, const refEntity_t *refent, const char *tagName, int startIndex )
{
	int result;
	VM::SendMsg<Render::LerpTagMsg>(*refent, tagName, startIndex, *tag, result);
	return result;
}

void trap_R_RemapShader( const char *oldShader, const char *newShader, const char *timeOffset )
{
	VM::SendMsg<Render::RemapShaderMsg>(oldShader, newShader, timeOffset);
}

bool trap_R_inPVS( const vec3_t p1, const vec3_t p2 )
{
	bool res;
	std::array<float, 3> myp1, myp2;
	VectorCopy(p1, myp1);
	VectorCopy(p2, myp2);
	VM::SendMsg<Render::InPVSMsg>(myp1, myp2, res);
	return res;
}

std::vector<bool> trap_R_BatchInPVS(
	const vec3_t origin,
	const std::vector<std::array<float, 3>>& posEntities )
{
	std::array<float, 3> myOrigin;
	VectorCopy(origin, myOrigin);
	std::vector<bool> inPVS;
	VM::SendMsg<Render::BatchInPVSMsg>(myOrigin, posEntities, inPVS);
	return inPVS;
}

int trap_R_LightForPoint( vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir )
{
	int result;
	std::array<float, 3> mypoint, myambient, mydirected, mydir;
	VectorCopy(point, mypoint);
	VM::SendMsg<Render::LightForPointMsg>(mypoint, myambient, mydirected, mydir, result);
	VectorCopy(myambient, ambientLight);
	VectorCopy(mydirected, directedLight);
	VectorCopy(mydir, lightDir);
	return result;
}

qhandle_t trap_R_RegisterAnimation( const char *name )
{
	int handle;
	VM::SendMsg<Render::RegisterAnimationMsg>(name, handle);
	return handle;
}

int trap_R_BuildSkeleton( refSkeleton_t *skel, qhandle_t anim, int startFrame, int endFrame, float frac, bool clearOrigin )
{
	int result;
	VM::SendMsg<Render::BuildSkeletonMsg>(anim, startFrame, endFrame, frac, clearOrigin, *skel, result);
	return result;
}

// Shamelessly stolen from tr_animation.cpp
int trap_R_BlendSkeleton( refSkeleton_t *skel, const refSkeleton_t *blend, float frac )
{
    int    i;
    vec3_t bounds[ 2 ];

    if ( skel->numBones != blend->numBones )
    {
        Log::Warn("trap_R_BlendSkeleton: different number of bones %d != %d", skel->numBones, blend->numBones);
        return false;
    }

    // lerp between the 2 bone poses
    for ( i = 0; i < skel->numBones; i++ )
    {
        transform_t trans;

        TransStartLerp( &trans );
        TransAddWeight( 1.0f - frac, &skel->bones[ i ].t, &trans );
        TransAddWeight( frac, &blend->bones[ i ].t, &trans );
        TransEndLerp( &trans );

        skel->bones[ i ].t = trans;
    }

    // calculate a bounding box in the current coordinate system
    for ( i = 0; i < 3; i++ )
    {
        bounds[ 0 ][ i ] = skel->bounds[ 0 ][ i ] < blend->bounds[ 0 ][ i ] ? skel->bounds[ 0 ][ i ] : blend->bounds[ 0 ][ i ];
        bounds[ 1 ][ i ] = skel->bounds[ 1 ][ i ] > blend->bounds[ 1 ][ i ] ? skel->bounds[ 1 ][ i ] : blend->bounds[ 1 ][ i ];
    }

    VectorCopy( bounds[ 0 ], skel->bounds[ 0 ] );
    VectorCopy( bounds[ 1 ], skel->bounds[ 1 ] );

    return true;
}

int trap_R_BoneIndex( qhandle_t hModel, const char *boneName )
{
	int index;
	VM::SendMsg<Render::BoneIndexMsg>(hModel, boneName, index);
	return index;
}

int trap_R_AnimNumFrames( qhandle_t hAnim )
{
	int n;
	VM::SendMsg<Render::AnimNumFramesMsg>(hAnim, n);
	return n;
}

int trap_R_AnimFrameRate( qhandle_t hAnim )
{
	int n;
	VM::SendMsg<Render::AnimFrameRateMsg>(hAnim, n);
	return n;
}

qhandle_t trap_RegisterVisTest()
{
	int handle;
	VM::SendMsg<Render::RegisterVisTestMsg>(handle);
	return handle;
}

void trap_AddVisTestToScene( qhandle_t hTest, const vec3_t pos, float depthAdjust, float area )
{
	std::array<float, 3> mypos;
	VectorCopy(pos, mypos);
	cmdBuffer.SendMsg<Render::AddVisTestToSceneMsg>(hTest, mypos, depthAdjust, area);
}

float trap_CheckVisibility( qhandle_t hTest )
{
	float result;
	VM::SendMsg<Render::CheckVisibilityMsg>(hTest, result);
	return result;
}

void trap_R_GetTextureSize( qhandle_t handle, int *x, int *y )
{
	VM::SendMsg<Render::GetTextureSizeMsg>(handle, *x, *y);
}

qhandle_t trap_R_GenerateTexture( const byte *data, int x, int y )
{
	ASSERT( x && y );
	qhandle_t handle;
	std::vector<byte> mydata(data, data + 4 * x * y);
	VM::SendMsg<Render::GenerateTextureMsg>(mydata, x, y, handle);
	return handle;
}


void trap_UnregisterVisTest( qhandle_t hTest )
{
	cmdBuffer.SendMsg<Render::UnregisterVisTestMsg>(hTest);
}

void trap_SetColorGrading( int slot, qhandle_t hShader )
{
	cmdBuffer.SendMsg<Render::SetColorGradingMsg>(slot, hShader);
}

// All keys

int trap_Key_GetCatcher()
{
	int result;
	VM::SendMsg<Keyboard::GetCatcherMsg>(result);
	return result;
}

void trap_Key_SetCatcher( int catcher )
{
	VM::SendMsg<Keyboard::SetCatcherMsg>(catcher);
}

std::vector<std::vector<Keyboard::Key>> trap_Key_GetKeysForBinds(int team, const std::vector<std::string>& binds) {
	std::vector<std::vector<Keyboard::Key>> result;
	VM::SendMsg<Keyboard::GetKeysForBindsMsg>(team, binds, result);
	return result;
}

int trap_Key_GetCharForScancode( int scancode )
{
	int result;
	VM::SendMsg<Keyboard::GetCharForScancodeMsg>(scancode, result);
	return result;
}

void trap_Key_SetBinding( Keyboard::Key key, int team, const char *cmd )
{
	VM::SendMsg<Keyboard::SetBindingMsg>(key, team, cmd);
}

std::vector<Keyboard::Key> trap_Key_GetConsoleKeys()
{
	std::vector<Keyboard::Key> result;
	VM::SendMsg<Keyboard::GetConsoleKeysMsg>(result);
	return result;
}

void trap_Key_SetConsoleKeys(const std::vector<Keyboard::Key>& keys)
{
	VM::SendMsg<Keyboard::SetConsoleKeysMsg>(keys);
}

void trap_Key_ClearCmdButtons()
{
	VM::SendMsg<Keyboard::ClearCmdButtonsMsg>();
}

void trap_Key_ClearStates()
{
	VM::SendMsg<Keyboard::ClearStatesMsg>();
}

std::vector<bool> trap_Key_KeysDown( const std::vector<Keyboard::Key>& keys )
{
	std::vector<bool> list;
	VM::SendMsg<Keyboard::KeysDownMsg>( keys, list );
	return list;
}

// Mouse

void trap_SetMouseMode( MouseMode mode )
{
	VM::SendMsg<Mouse::SetMouseMode>( mode );
}

// All LAN

int trap_LAN_GetServerCount( int source )
{
	int count;
	VM::SendMsg<LAN::GetServerCountMsg>(source, count);
	return count;
}

void trap_LAN_GetServerInfo( int source, int n, char *buf, int buflen )
{
	std::string info;
	VM::SendMsg<LAN::GetServerInfoMsg>(source, n, buflen, info);
	Q_strncpyz(buf, info.c_str(), buflen);
}

int trap_LAN_GetServerPing( int source, int n )
{
	int ping;
	VM::SendMsg<LAN::GetServerPingMsg>(source, n, ping);
	return ping;
}

void trap_LAN_MarkServerVisible( int source, int n, bool visible )
{
	VM::SendMsg<LAN::MarkServerVisibleMsg>(source, n, visible);
}

int trap_LAN_ServerIsVisible( int source, int n )
{
	bool visible;
	VM::SendMsg<LAN::ServerIsVisibleMsg>(source, n, visible);
	return visible;
}

bool trap_LAN_UpdateVisiblePings( int source )
{
	bool res;
	VM::SendMsg<LAN::UpdateVisiblePingsMsg>(source, res);
	return res;
}

void trap_LAN_ResetPings( int n )
{
	VM::SendMsg<LAN::ResetPingsMsg>(n);
}

int trap_LAN_ServerStatus( const char *serverAddress, char *serverStatus, int maxLen )
{
	std::string status;
	int res;
	VM::SendMsg<LAN::ServerStatusMsg>(serverAddress, maxLen, status, res);
	Q_strncpyz(serverStatus, status.c_str(), maxLen);
	return res;
}

void trap_LAN_ResetServerStatus()
{
	VM::SendMsg<LAN::ResetServerStatusMsg>();
}
