/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2006-2011 Robert Beckebans <trebor_7@users.sourceforge.net>

This file is part of Daemon source code.

Daemon source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Daemon source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// tr_cmds.c
#include "tr_local.h"

volatile bool            renderThreadActive;

/*
=====================
R_PerformanceCounters
=====================
*/
void R_PerformanceCounters()
{
	if ( !r_speeds->integer )
	{
		// clear the counters even if we aren't printing
		tr.pc = {};
		backEnd.pc = {};
		return;
	}

	if ( r_speeds->integer == Util::ordinal(renderSpeeds_t::RSPEEDS_GENERAL))
	{
		Log::Notice("%i views %i portals %i batches %i surfs %i leafs %i verts %i tris",
		           backEnd.pc.c_views, backEnd.pc.c_portals, backEnd.pc.c_batches, backEnd.pc.c_surfaces, tr.pc.c_leafs,
		           backEnd.pc.c_vertexes, backEnd.pc.c_indexes / 3 );

		Log::Notice("%i draws %i vbos %i ibos %i verts %i tris",
		           backEnd.pc.c_drawElements,
		           backEnd.pc.c_vboVertexBuffers, backEnd.pc.c_vboIndexBuffers,
		           backEnd.pc.c_vboVertexes, backEnd.pc.c_vboIndexes / 3 );

		Log::Notice("%i multidraws %i primitives %i tris",
		           backEnd.pc.c_multiDrawElements,
		           backEnd.pc.c_multiDrawPrimitives,
		           backEnd.pc.c_multiVboIndexes / 3 );
	}
	else if ( r_speeds->integer == Util::ordinal(renderSpeeds_t::RSPEEDS_CULLING ))
	{
		Log::Notice("(gen) %i pin %i pout %i bin %i bclip %i bout",
		           tr.pc.c_plane_cull_in, tr.pc.c_plane_cull_out, tr.pc.c_box_cull_in,
		           tr.pc.c_box_cull_clip, tr.pc.c_box_cull_out );

		Log::Notice("(mdv) %i sin %i sclip %i sout %i bin %i bclip %i bout",
		           tr.pc.c_sphere_cull_mdv_in, tr.pc.c_sphere_cull_mdv_clip,
		           tr.pc.c_sphere_cull_mdv_out, tr.pc.c_box_cull_mdv_in, tr.pc.c_box_cull_mdv_clip, tr.pc.c_box_cull_mdv_out );

		Log::Notice("(md5) %i bin %i bclip %i bout",
		           tr.pc.c_box_cull_md5_in, tr.pc.c_box_cull_md5_clip, tr.pc.c_box_cull_md5_out );
	}
	else if ( r_speeds->integer == Util::ordinal(renderSpeeds_t::RSPEEDS_VIEWCLUSTER ))
	{
		Log::Notice("viewcluster: %i", tr.visClusters[ tr.visIndex ] );
	}
	else if ( r_speeds->integer == Util::ordinal(renderSpeeds_t::RSPEEDS_FOG ))
	{
		Log::Notice("fog srf:%i batches:%i", backEnd.pc.c_fogSurfaces, backEnd.pc.c_fogBatches );
	}
	else if ( r_speeds->integer == Util::ordinal(renderSpeeds_t::RSPEEDS_NEAR_FAR ))
	{
		Log::Notice("zNear: %.0f zFar: %.0f", tr.viewParms.zNear, tr.viewParms.zFar );
	}

	tr.pc = {};
	backEnd.pc = {};
}

/*
====================
R_IssueRenderCommands
====================
*/
int c_blockedOnRender;
int c_blockedOnMain;

void R_IssueRenderCommands( bool runPerformanceCounters )
{
	renderCommandList_t *cmdList;

	cmdList = &backEndData[ tr.smpFrame ]->commands;
	ASSERT(cmdList != nullptr);
	// add an end-of-list command
	new (&cmdList->cmds[cmdList->used]) EndOfListCommand();

	// clear it out, in case this is a sync and not a buffer flip
	cmdList->used = 0;

	if ( glConfig.smpActive )
	{
		// if the render thread is not idle, wait for it
		if ( renderThreadActive )
		{
			c_blockedOnRender++;

			if ( r_showSmp->integer )
			{
				Log::Notice("R");
			}
		}
		else
		{
			c_blockedOnMain++;

			if ( r_showSmp->integer )
			{
				Log::Notice(".");
			}
		}

		// sleep until the renderer has completed
		GLimp_FrontEndSleep();
	}

	// at this point, the back end thread is idle, so it is ok
	// to look at its performance counters
	if ( runPerformanceCounters )
	{
		R_PerformanceCounters();
	}

	// actually start the commands going
	if ( !r_skipBackEnd->integer )
	{
		// let it start on the new batch
		if ( !glConfig.smpActive )
		{
			RB_ExecuteRenderCommands( cmdList->cmds );
		}
		else
		{
			GLimp_WakeRenderer( cmdList->cmds );
		}
	}
}

/*
====================
R_SyncRenderThread

Issue any pending commands and wait for them to complete.
After exiting, the render thread will have completed its work
and will remain idle and the main thread is free to issue
OpenGL calls until R_IssueRenderCommands is called.
====================
*/
void R_SyncRenderThread()
{
	ASSERT( Sys::OnMainThread() ); // only call this from the frontend

	if ( !tr.registered )
	{
		return;
	}

	R_IssueRenderCommands( false );

	if ( !glConfig.smpActive )
	{
		return;
	}

	GLimp_SyncRenderThread();
}

/*
============
R_GetCommandBuffer

make sure there is enough command space, waiting on the
render thread if needed.
============
*/
void           *R_GetCommandBuffer( size_t bytes )
{
	renderCommandList_t *cmdList;
	const size_t reserved = sizeof( SwapBuffersCommand ) + sizeof( EndOfListCommand );

	cmdList = &backEndData[ tr.smpFrame ]->commands;

	// always leave room for the swap buffers and end of list commands
	// RB: added swapBuffers_t from ET
	if ( cmdList->used + bytes + reserved > MAX_RENDER_COMMANDS )
	{
		if ( bytes > MAX_RENDER_COMMANDS - reserved )
		{
			Sys::Error( "R_GetCommandBuffer: bad size %u", bytes );
		}

		// if we run out of room, just start dropping commands
		return nullptr;
	}

	cmdList->used += bytes;

	return cmdList->cmds + cmdList->used - bytes;
}

/*
=============
R_AddSetupLightsCmd
=============
*/
void R_AddSetupLightsCmd()
{
	if ( !glConfig2.realtimeLighting )
	{
		return;
	}

	SetupLightsCommand *cmd;

	cmd = R_GetRenderCommand<SetupLightsCommand>();

	if ( !cmd )
	{
		return;
	}

	cmd->refdef = tr.refdef;
}

/*
=============
R_AddDrawViewCmd
=============
*/
void R_AddDrawViewCmd( bool depthPass )
{
	DrawViewCommand *cmd;

	cmd = R_GetRenderCommand<DrawViewCommand>();

	if ( !cmd )
	{
		return;
	}

	cmd->refdef = tr.refdef;
	cmd->viewParms = tr.viewParms;
	cmd->depthPass = depthPass;
}

/*
=============
R_AddClearBufferCmd
=============
*/
void R_AddClearBufferCmd( )
{
	ClearBufferCommand *cmd;

	cmd = R_GetRenderCommand<ClearBufferCommand>();

	if ( !cmd )
	{
		return;
	}

	cmd->refdef = tr.refdef;
	cmd->viewParms = tr.viewParms;
}

/*
=============
R_AddPreparePortalCmd
=============
*/
void R_AddPreparePortalCmd( drawSurf_t *surf )
{
	PreparePortalCommand *cmd;

	cmd = R_GetRenderCommand<PreparePortalCommand>();

	if ( !cmd )
	{
		return;
	}

	cmd->refdef = tr.refdef;
	cmd->viewParms = tr.viewParms;
	cmd->surface = surf;
}

/*
=============
R_AddPreparePortalCmd
=============
*/
void R_AddFinalisePortalCmd( drawSurf_t *surf )
{
	FinalisePortalCommand *cmd;

	cmd = R_GetRenderCommand<FinalisePortalCommand>();

	if ( !cmd )
	{
		return;
	}

	cmd->refdef = tr.refdef;
	cmd->viewParms = tr.viewParms;
	cmd->surface = surf;
}

/*
=============
R_AddPostProcessCmd
=============
*/
void R_AddPostProcessCmd()
{
	RenderPostProcessCommand *cmd;

	cmd = R_GetRenderCommand<RenderPostProcessCommand>();

	if (!cmd)
	{
		return;
	}

	cmd->refdef = tr.refdef;
	cmd->viewParms = tr.viewParms;
}

/*
=============
RE_SetColor

Passing nullptr will set the color to white
=============
*/
void RE_SetColor( const Color::Color& rgba )
{
	SetColorCommand *cmd;

	if ( !tr.registered )
	{
		return;
	}

	cmd = R_GetRenderCommand<SetColorCommand>();

	if ( !cmd )
	{
		return;
	}

	cmd->color = rgba;
}

/*
=============
RE_SetColorGrading
=============
*/
void RE_SetColorGrading( int slot, qhandle_t hShader )
{
	SetColorGradingCommand *cmd;
	shader_t *shader = R_GetShaderByHandle( hShader );
	image_t *image;

	if ( !glConfig2.colorGrading )
	{
		return;
	}

	if ( !tr.registered )
	{
		return;
	}

	if ( slot < 0 || slot > 3 )
	{
		return;
	}

	if ( shader->defaultShader || shader->stages == shader->lastStage )
	{
		return;
	}

	image = shader->stages[ 0 ].bundle[ 0 ].image[ 0 ];

	if ( !image )
	{
		return;
	}

	if ( image->width != REF_COLORGRADEMAP_SIZE && image->height != REF_COLORGRADEMAP_SIZE )
	{
		return;
	}

	if ( image->width * image->height != REF_COLORGRADEMAP_STORE_SIZE )
	{
		return;
	}

	cmd = R_GetRenderCommand<SetColorGradingCommand>();

	if ( !cmd )
	{
		return;
	}

	cmd->slot = slot;
	cmd->image = image;
}

/*
=============
R_ClipRegion
=============
*/
static bool R_ClipRegion ( float *x, float *y, float *w, float *h, float *s1, float *t1, float *s2, float *t2 )
{
	float left, top, right, bottom;
	float _s1, _t1, _s2, _t2;
	float clipLeft, clipTop, clipRight, clipBottom;

	if ( tr.clipRegion[2] <= tr.clipRegion[0] ||
		tr.clipRegion[3] <= tr.clipRegion[1] )
	{
		return false;
	}

	left = *x;
	top = *y;
	right = *x + *w;
	bottom = *y + *h;

	_s1 = *s1;
	_t1 = *t1;
	_s2 = *s2;
	_t2 = *t2;

	clipLeft = tr.clipRegion[0];
	clipTop = tr.clipRegion[1];
	clipRight = tr.clipRegion[2];
	clipBottom = tr.clipRegion[3];

	// Completely clipped away
	if ( right <= clipLeft || left >= clipRight ||
		bottom <= clipTop || top >= clipBottom )
	{
		return true;
	}

	// Clip left edge
	if ( left < clipLeft )
	{
		float f = ( clipLeft - left ) / ( right - left );
		*s1 = ( f * ( _s2 - _s1 ) ) + _s1;
		*x = clipLeft;
		*w -= ( clipLeft - left );
	}

	// Clip right edge
	if ( right > clipRight )
	{
		float f = ( clipRight - right ) / ( left - right );
		*s2 = ( f * ( _s1 - _s2 ) ) + _s2;
		*w = clipRight - *x;
	}

	// Clip top edge
	if ( top < clipTop )
	{
		float f = ( clipTop - top ) / ( bottom - top );
		*t1 = ( f * ( _t2 - _t1 ) ) + _t1;
		*y = clipTop;
		*h -= ( clipTop - top );
	}

	// Clip bottom edge
	if ( bottom > clipBottom )
	{
		float f = ( clipBottom - bottom ) / ( top - bottom );
		*t2 = ( f * ( _t1 - _t2 ) ) + _t2;
		*h = clipBottom - *y;
	}

	return false;
}


/*
=============
RE_SetClipRegion
=============
*/
void RE_SetClipRegion( const float *region )
{
	if ( region == nullptr )
	{
		memset( tr.clipRegion, 0, sizeof( vec4_t ) );
	}
	else
	{
		Vector4Copy( region, tr.clipRegion );
	}
}

/*
=============
RE_StretchPic
=============
*/
void RE_StretchPic ( float x, float y, float w, float h,
					  float s1, float t1, float s2, float t2, qhandle_t hShader )
{
	StretchPicCommand	*cmd;

	if (!tr.registered)
	{
		return;
	}
	if ( R_ClipRegion( &x, &y, &w, &h, &s1, &t1, &s2, &t2 ) )
	{
		return;
	}

	cmd = R_GetRenderCommand<StretchPicCommand>();
	if ( !cmd )
	{
		return;
	}

	cmd->shader = R_GetShaderByHandle( hShader );
	cmd->x = x;
	cmd->y = y;
	cmd->w = w;
	cmd->h = h;
	cmd->s1 = s1;
	cmd->t1 = t1;
	cmd->s2 = s2;
	cmd->t2 = t2;
}

/*
=============
RE_2DPolyies
=============
*/
extern int r_numPolyVerts;
extern int r_numPolyIndexes;

void RE_2DPolyies( polyVert_t *verts, int numverts, qhandle_t hShader )
{
	Poly2dCommand *cmd;

	if ( r_numPolyVerts + numverts > r_maxPolyVerts->integer )
	{
		return;
	}

	cmd = R_GetRenderCommand<Poly2dCommand>();

	if ( !cmd )
	{
		return;
	}

	cmd->verts = &backEndData[ tr.smpFrame ]->polyVerts[ r_numPolyVerts ];
	cmd->numverts = numverts;
	std::copy_n( verts, numverts, cmd->verts );
	cmd->shader = R_GetShaderByHandle( hShader );

	r_numPolyVerts += numverts;
}

void RE_2DPolyiesIndexed( polyVert_t *verts, int numverts, int *indexes, int numindexes, int trans_x, int trans_y, qhandle_t hShader )
{
	Poly2dIndexedCommand *cmd;

	if ( r_numPolyVerts + numverts > r_maxPolyVerts->integer )
	{
		return;
	}

	if ( r_numPolyIndexes + numindexes > r_maxPolyVerts->integer )
	{
		return;
	}

	cmd = R_GetRenderCommand<Poly2dIndexedCommand>();

	if ( !cmd )
	{
		return;
	}

	cmd->verts = &backEndData[ tr.smpFrame ]->polyVerts[ r_numPolyVerts ];
	cmd->numverts = numverts;
	std::copy_n( verts, numverts, cmd->verts );
	cmd->shader = R_GetShaderByHandle( hShader );
	cmd->indexes = &backEndData[ tr.smpFrame ]->polyIndexes[ r_numPolyIndexes ];
	std::copy_n( indexes, numindexes, cmd->indexes );
	cmd->numIndexes = numindexes;
	cmd->translation[ 0 ] = trans_x;
	cmd->translation[ 1 ] = trans_y;

	r_numPolyVerts += numverts;
	r_numPolyIndexes += numindexes;
}

/*
================
RE_ScissorEnable
================
*/
void RE_ScissorEnable( bool enable )
{
	// scissor disable sets scissor to full screen
	// scissor enable is a no-op
	if( !enable ) {
		RE_ScissorSet( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
	}
}

/*
=============
RE_ScissorSet
=============
*/
void RE_ScissorSet( int x, int y, int w, int h )
{
	ScissorSetCommand *cmd;

	cmd = R_GetRenderCommand<ScissorSetCommand>();

	if ( !cmd )
	{
		return;
	}

	cmd->x = x;
	cmd->y = y;
	cmd->w = w;
	cmd->h = h;
}

/*
=============
RE_SetMatrixTransform

Set the 2D matrix transform. Will override the previous matrix transform if called in
succession. Latest call of SetMatrixTransform's matrix will continue to take effect until
ResetMatrixTransform is called.
=============
*/
void RE_SetMatrixTransform(const matrix_t matrix)
{
	SetMatrixTransformCommand *cmd;
	cmd = R_GetRenderCommand<SetMatrixTransformCommand>();

	if ( !cmd )
	{
		return;
	}

	MatrixCopy(matrix, cmd->matrix);
}

/*
============
RE_PopMatrix

Resets the 2D matrix transform that was set with RE_SetMatrixTransform.
============
*/
void RE_ResetMatrixTransform()
{
	// This allocates the command and adds it to the queue to run.
	R_GetRenderCommand<ResetMatrixTransformCommand>();
}

/*
=============
RE_RotatedPic
=============
*/
void RE_RotatedPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader, float angle )
{
	RotatedPicCommand *cmd;

	cmd = R_GetRenderCommand<RotatedPicCommand>();

	if ( !cmd )
	{
		return;
	}

	cmd->shader = R_GetShaderByHandle( hShader );
	cmd->x = x;
	cmd->y = y;
	cmd->w = w;
	cmd->h = h;

	cmd->angle = angle;
	cmd->s1 = s1;
	cmd->t1 = t1;
	cmd->s2 = s2;
	cmd->t2 = t2;
}

//----(SA)  added

/*
==============
RE_StretchPicGradient
==============
*/
void RE_StretchPicGradient( float x, float y, float w, float h,
                            float s1, float t1, float s2, float t2,
                            qhandle_t hShader, const Color::Color& gradientColor,
                            int gradientType )
{
	GradientPicCommand *cmd;

	cmd = R_GetRenderCommand<GradientPicCommand>();

	if ( !cmd )
	{
		return;
	}

	cmd->shader = R_GetShaderByHandle( hShader );
	cmd->x = x;
	cmd->y = y;
	cmd->w = w;
	cmd->h = h;
	cmd->s1 = s1;
	cmd->t1 = t1;
	cmd->s2 = s2;
	cmd->t2 = t2;

	cmd->gradientColor = gradientColor;
	cmd->gradientType = gradientType;
}

//----(SA)  end

/*
====================
RE_BeginFrame
====================
*/
void RE_BeginFrame()
{
	DrawBufferCommand *cmd;

	if ( !tr.registered )
	{
		return;
	}

	GLIMP_LOGCOMMENT( "--- RE_BeginFrame ---" );

	tr.frameCount++;
	tr.frameSceneNum = 0;
	tr.viewCount = 0;

	// texturemode stuff
	if ( Util::optional<std::string> textureMode = r_textureMode.GetModifiedValue() )
	{
		R_SyncRenderThread();
		GL_TextureMode( textureMode->c_str() );
	}

	// check for errors
	if ( checkGLErrors() )
	{
		R_SyncRenderThread();
		GL_CheckErrors_( __FILE__, __LINE__ );
	}

	// draw buffer stuff
	cmd = R_GetRenderCommand<DrawBufferCommand>();

	if ( !cmd )
	{
		return;
	}

	if ( !Q_stricmp( r_drawBuffer->string, "GL_FRONT" ) )
	{
		cmd->buffer = ( int ) GL_FRONT;
	}
	else
	{
		cmd->buffer = ( int ) GL_BACK;
	}
}

/*
=============
RE_EndFrame

Returns the number of msec spent in the back end
=============
*/
void RE_EndFrame( int *frontEndMsec, int *backEndMsec )
{
	SwapBuffersCommand *cmd;

	if ( !tr.registered )
	{
		return;
	}

	GLimp_HandleCvars();

	cmd = R_GetRenderCommand<SwapBuffersCommand>();

	if ( !cmd )
	{
		return;
	}

	R_IssueRenderCommands( true );

	// use the other buffers next frame, because another CPU
	// may still be rendering into the current ones
	R_ToggleSmpFrame();

	// update the results of the vis tests
	R_UpdateVisTests();

	if ( frontEndMsec )
	{
		*frontEndMsec = tr.frontEndMsec;
	}

	tr.frontEndMsec = 0;

	if ( backEndMsec )
	{
		*backEndMsec = backEnd.pc.msec;
	}

	backEnd.pc.msec = 0;
}

/*
=============
RE_TakeVideoFrame
=============
*/
void RE_TakeVideoFrame( int width, int height, byte *captureBuffer, byte *encodeBuffer, bool motionJpeg )
{
	VideoFrameCommand *cmd;

	if ( !tr.registered )
	{
		return;
	}

	cmd = R_GetRenderCommand<VideoFrameCommand>();

	if ( !cmd )
	{
		return;
	}

	cmd->width = width;
	cmd->height = height;
	cmd->captureBuffer = captureBuffer;
	cmd->encodeBuffer = encodeBuffer;
	cmd->motionJpeg = motionJpeg;
}
