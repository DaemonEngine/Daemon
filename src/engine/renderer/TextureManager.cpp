/*
===========================================================================

Daemon BSD Source Code
Copyright (c) 2024 Daemon Developers
All rights reserved.

This file is part of the Daemon BSD Source Code (Daemon Source Code).

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
// TextureManager.cpp

#include "tr_local.h"
#include "GLUtils.h"

Texture::Texture() {
}

Texture::~Texture() {
	if ( bindlessTextureResident ) {
		MakeNonResident();
	}
}

bool Texture::IsResident() const {
	return bindlessTextureResident;
}

void Texture::MakeResident() {
	glMakeTextureHandleResidentARB( bindlessTextureHandle );
	bindlessTextureResident = true;
}

void Texture::MakeNonResident() {
	glMakeTextureHandleNonResidentARB( bindlessTextureHandle );
	bindlessTextureResident = false;
}

void Texture::GenBindlessHandle() {
	bindlessTextureHandle = glGetTextureHandleARB( textureHandle );

	if ( bindlessTextureHandle == 0 ) {
		Sys::Drop( "Failed to generate bindless texture handle" );
	}

	hasBindlessHandle = true;
}

TextureManager::TextureManager() = default;
TextureManager::~TextureManager() = default;

GLuint64 TextureManager::BindTexture( const GLint location, Texture *texture ) {
	if( location == -1 ) {
		return 0;
	}

	if ( texture->IsResident() ) {
		return texture->bindlessTextureHandle;
	}

	if( std::find( textures.begin(), textures.end(), texture ) == textures.end() ) {
		textures.push_back( texture );
	}

	// Bindless textures make the texture state immutable, so generate the handle as late as possible
	if ( !texture->hasBindlessHandle ) {
		texture->GenBindlessHandle();
	}

	texture->MakeResident();

	// Make lowest priority textures non-resident first
	int i = textures.size() - 1;
	while ( !glIsTextureHandleResidentARB( texture->bindlessTextureHandle ) ) {
		if ( i < 0 ) {
			Sys::Drop( "No texture space available" );
		}

		if ( textures[i]->IsResident() ) {
			textures[i]->MakeNonResident();
			texture->MakeResident();
		}
		i--;
	}

	GL_CheckErrors();

	return texture->bindlessTextureHandle;
}

void TextureManager::BindReservedTexture( const GLenum target, const GLuint handle ) {
	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( target, handle );
}

void TextureManager::FreeTextures() {
	textures.clear();
}
