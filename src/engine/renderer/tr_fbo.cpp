/*
===========================================================================
Copyright (C) 2006 Kirk Barnes
Copyright (C) 2006-2008 Robert Beckebans <trebor_7@users.sourceforge.net>

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
// tr_fbo.c
#include "tr_local.h"
#include "GLUtils.h"

/*
=============
R_CheckFBO
=============
*/
bool R_CheckFBO( const FBO_t *fbo )
{
	int code;
	int id;

	glGetIntegerv( GL_FRAMEBUFFER_BINDING, &id );
	GL_fboShim.glBindFramebuffer( GL_DRAW_FRAMEBUFFER, fbo->frameBuffer );

	code = GL_fboShim.glCheckFramebufferStatus( GL_DRAW_FRAMEBUFFER );

	if ( code == GL_FRAMEBUFFER_COMPLETE )
	{
		GL_fboShim.glBindFramebuffer( GL_DRAW_FRAMEBUFFER, id );
		return true;
	}

	// an error occurred
	switch ( code )
	{
		case GL_FRAMEBUFFER_UNSUPPORTED:
			Log::Warn("R_CheckFBO: (%s) Unsupported framebuffer format", fbo->name );
			break;

		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			Log::Warn("R_CheckFBO: (%s) Framebuffer incomplete attachment", fbo->name );
			break;

		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			Log::Warn("R_CheckFBO: (%s) Framebuffer incomplete, missing attachment", fbo->name );
			break;

		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
			Log::Warn("R_CheckFBO: (%s) Framebuffer incomplete, missing draw buffer", fbo->name );
			break;

		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
			Log::Warn("R_CheckFBO: (%s) Framebuffer incomplete, missing read buffer", fbo->name );
			break;

		/* Errors specific to EXT_framebuffer_object. Some headers may also define
		the names without _EXT suffix but that's for GLES so those names may only
		be available when GLES is available. The GL/glext.h header for desktop
		OpenGL uses _EXT suffixed names. */
		case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
			Log::Warn("R_CheckFBO: (%s) Framebuffer incomplete, attached images must have same dimensions", fbo->name );
			break;

		case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
			Log::Warn("R_CheckFBO: (%s) Framebuffer incomplete, attached images must have same format", fbo->name );
			break;

		default:
			Log::Warn("R_CheckFBO: (%s) unknown error 0x%X", fbo->name, code );
			break;
	}

	GL_fboShim.glBindFramebuffer( GL_DRAW_FRAMEBUFFER, id );

	return false;
}

/*
============
R_CreateFBO
============
*/
FBO_t          *R_CreateFBO( const char *name, int width, int height )
{
	FBO_t *fbo;

	if ( strlen( name ) >= MAX_QPATH )
	{
		Sys::Drop( "R_CreateFBO: \"%s\" is too long", name );
	}

	if ( width <= 0 )
	{
		Sys::Drop( "R_CreateFBO: bad width %i", width );
	}

	if ( height <= 0 )
	{
		Sys::Drop( "R_CreateFBO: bad height %i", height );
	}

	if ( tr.numFBOs == MAX_FBOS )
	{
		Sys::Drop( "R_CreateFBO: MAX_FBOS hit" );
	}

	fbo = tr.fbos[ tr.numFBOs ] = (FBO_t*) ri.Hunk_Alloc( sizeof( *fbo ), ha_pref::h_low );
	Q_strncpyz( fbo->name, name, sizeof( fbo->name ) );
	fbo->width = width;
	fbo->height = height;

	GL_fboShim.glGenFramebuffers( 1, &fbo->frameBuffer );

	return fbo;
}

/*
=================
R_AttachFBOTexture1D
=================
*/
void R_AttachFBOTexture1D( int texId, int index )
{
	if ( index < 0 || index >= glConfig.maxColorAttachments )
	{
		Log::Warn("R_AttachFBOTexture1D: invalid attachment index %i", index );
		return;
	}

	glFramebufferTexture1D( GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, GL_TEXTURE_1D, texId, 0 );
}

/*
=================
R_AttachFBOTexture2D
=================
*/
void R_AttachFBOTexture2D( int target, int texId, int index )
{
	if ( target != GL_TEXTURE_2D && ( target < GL_TEXTURE_CUBE_MAP_POSITIVE_X || target > GL_TEXTURE_CUBE_MAP_NEGATIVE_Z ) )
	{
		Log::Warn("R_AttachFBOTexture2D: invalid target %i", target );
		return;
	}

	if ( index < 0 || index >= glConfig.maxColorAttachments )
	{
		Log::Warn("R_AttachFBOTexture2D: invalid attachment index %i", index );
		return;
	}

	GL_fboShim.glFramebufferTexture2D( GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, target, texId, 0 );
}

/*
=================
R_AttachFBOTexture3D
=================
*/
void R_AttachFBOTexture3D( int texId, int index, int zOffset )
{
	if ( index < 0 || index >= glConfig.maxColorAttachments )
	{
		Log::Warn("R_AttachFBOTexture3D: invalid attachment index %i", index );
		return;
	}

	glFramebufferTexture3D( GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, GL_TEXTURE_3D, texId, 0, zOffset );
}

/*
=================
R_AttachFBOTexturePackedDepthStencil
=================
*/
void R_AttachFBOTexturePackedDepthStencil( int texId )
{
	GL_fboShim.glFramebufferTexture2D( GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texId, 0 );
	GL_fboShim.glFramebufferTexture2D( GL_DRAW_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, texId, 0 );
}

/*
============
R_BindFBO
============
*/
void R_BindFBO( FBO_t *fbo )
{
	if ( !fbo )
	{
		R_BindNullFBO();
		return;
	}

	GLIMP_LOGCOMMENT( "--- R_BindFBO( %s ) ---", fbo->name );

	if ( glState.currentFBO != fbo )
	{
		GL_fboShim.glBindFramebuffer( GL_DRAW_FRAMEBUFFER, fbo->frameBuffer );

		glState.currentFBO = fbo;
	}
}

/*
============
R_BindNullFBO
============
*/
void R_BindNullFBO()
{
	GLIMP_LOGCOMMENT( "--- R_BindNullFBO ---" );

	if ( glState.currentFBO )
	{
		GL_fboShim.glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 );
		glState.currentFBO = nullptr;
	}
}

/*
============
R_InitFBOs
============
*/
void R_InitFBOs()
{
	int i;
	int width, height;

	Log::Debug("------- R_InitFBOs -------");

	tr.numFBOs = 0;

	GL_CheckErrors();

	// make sure the render thread is stopped
	R_SyncRenderThread();

	width = windowConfig.vidWidth;
	height = windowConfig.vidHeight;

	tr.mainFBO[0] = R_CreateFBO( "_main[0]", width, height );
	R_BindFBO( tr.mainFBO[0] );
	R_AttachFBOTexture2D( GL_TEXTURE_2D, tr.currentRenderImage[0]->texnum, 0 );
	R_AttachFBOTexturePackedDepthStencil( tr.currentDepthImage->texnum );
	R_CheckFBO( tr.mainFBO[0] );

	tr.mainFBO[1] = R_CreateFBO( "_main[1]", width, height );
	R_BindFBO( tr.mainFBO[1] );
	R_AttachFBOTexture2D( GL_TEXTURE_2D, tr.currentRenderImage[1]->texnum, 0 );
	R_AttachFBOTexturePackedDepthStencil( tr.currentDepthImage->texnum );
	R_CheckFBO( tr.mainFBO[1] );

	if ( glConfig.realtimeLighting )
	{
		/* It's only required to create frame buffers only used by the
		tiled dynamic lighting renderer when this feature is enabled. */

		tr.depthtile1FBO = R_CreateFBO( "_depthtile1", tr.depthtile1RenderImage->width, tr.depthtile1RenderImage->height );
		R_BindFBO( tr.depthtile1FBO );
		R_AttachFBOTexture2D( GL_TEXTURE_2D, tr.depthtile1RenderImage->texnum, 0 );
		R_CheckFBO( tr.depthtile1FBO );

		tr.depthtile2FBO = R_CreateFBO( "_depthtile2", tr.depthtile2RenderImage->width, tr.depthtile2RenderImage->height );
		R_BindFBO( tr.depthtile2FBO );
		R_AttachFBOTexture2D( GL_TEXTURE_2D, tr.depthtile2RenderImage->texnum, 0 );
		R_CheckFBO( tr.depthtile2FBO );

		tr.lighttileFBO = R_CreateFBO( "_lighttile", tr.lighttileRenderImage->width, tr.lighttileRenderImage->height );
		R_BindFBO( tr.lighttileFBO );
		R_AttachFBOTexture3D( tr.lighttileRenderImage->texnum, 0, 0 );
		R_CheckFBO( tr.lighttileFBO );
	}

	if ( r_liquidMapping->integer )
	{
		width = windowConfig.vidWidth;
		height = windowConfig.vidHeight;

		tr.portalRenderFBO = R_CreateFBO( "_portalRender", width, height );
		R_BindFBO( tr.portalRenderFBO );

		R_AttachFBOTexture2D( GL_TEXTURE_2D, tr.portalRenderImage->texnum, 0 );

		R_CheckFBO( tr.portalRenderFBO );
	}

	if ( glConfig.bloom )
	{
		width = windowConfig.vidWidth * 0.25f;
		height = windowConfig.vidHeight * 0.25f;

		tr.contrastRenderFBO = R_CreateFBO( "_contrastRender", width, height );
		R_BindFBO( tr.contrastRenderFBO );

		R_AttachFBOTexture2D( GL_TEXTURE_2D, tr.contrastRenderFBOImage->texnum, 0 );

		R_CheckFBO( tr.contrastRenderFBO );

		for ( i = 0; i < 2; i++ )
		{
			tr.bloomRenderFBO[ i ] = R_CreateFBO( va( "_bloomRender%d", i ), width, height );
			R_BindFBO( tr.bloomRenderFBO[ i ] );

			R_AttachFBOTexture2D( GL_TEXTURE_2D, tr.bloomRenderFBOImage[ i ]->texnum, 0 );

			R_CheckFBO( tr.bloomRenderFBO[ i ] );
		}
	}

	GL_CheckErrors();

	R_BindNullFBO();
}

/*
============
R_ShutdownFBOs
============
*/
void R_ShutdownFBOs()
{
	FBO_t *fbo;

	Log::Debug("------- R_ShutdownFBOs -------" );

	R_BindNullFBO();

	for ( int i = 0; i < tr.numFBOs; i++ )
	{
		fbo = tr.fbos[ i ];

		if ( fbo->frameBuffer )
		{
			GL_fboShim.glDeleteFramebuffers( 1, &fbo->frameBuffer );
		}
	}
}

class ListFBOsCmd : public Cmd::StaticCmd
{
public:
	ListFBOsCmd() : StaticCmd("listFBOs", Cmd::RENDERER, "list renderer's OpenGL framebuffer objects") {}

	void Run( const Cmd::Args & ) const override
	{
		int   i;
		FBO_t *fbo;

		Print("             size       name" );
		Print("----------------------------------------------------------" );

		for ( i = 0; i < tr.numFBOs; i++ )
		{
			fbo = tr.fbos[ i ];

			Print("  %4i: %4i %4i %s", i, fbo->width, fbo->height, fbo->name );
		}

		Print(" %i FBOs", tr.numFBOs );
	}
};
static ListFBOsCmd listFBOsCmdRegistration;
