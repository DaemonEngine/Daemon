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

#include "TextureManager.h"
#include "tr_local.h"

Texture::Texture() {
}

Texture::~Texture() {
	if ( bindlessTextureResident ) {
		MakeNonResident();
	}
}

void Texture::UpdateAdjustedPriority( const int totalFrameTextureBinds, const int totalTextureBinds ) {
	if ( totalFrameTextureBinds == 0 || totalTextureBinds == 0 ) {
		return;
	}

	adjustedPriority = basePriority + frameBindCounter / totalFrameTextureBinds * 0.5 + totalBindCounter / totalTextureBinds * 1.5;
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

// TextureManager textureManager;

TextureManager::TextureManager() {
	textureUnits.reserve( glConfig2.maxTextureUnits );
}

TextureManager::~TextureManager() = default;

void TextureManager::UpdateAdjustedPriorities() {
	for ( Texture* texture : textures ) {
		texture->UpdateAdjustedPriority( totalFrameTextureBinds, totalTextureBinds );
	}
	// std::sort( textures.begin(), textures.end(), Texture::Compare() );

	totalFrameTextureBinds = 0;
}

void TextureManager::BindTexture( const GLint location, Texture *texture ) {
	texture->frameBindCounter++;
	texture->totalBindCounter++;
	
	totalFrameTextureBinds++;
	totalTextureBinds++;

	if( location == -1 ) {
		return;
	}

	if ( texture->IsResident() ) {
		glUniformHandleui64ARB( location, texture->bindlessTextureHandle );
		return;
	}

	if( std::find( textures.begin(), textures.end(), texture ) == textures.end() ) {
		textures.push_back( texture );
	}

	// Use bindless textures if possible
	if ( glConfig2.bindlessTexturesAvailable ) {
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

		glUniformHandleui64ARB( location, texture->bindlessTextureHandle );

		GL_CheckErrors();

		return;
	}

	int lowestPriorityTexture = 1;
	float lowestPriority = 100000.0f;
	GLint handle;

	// Do a loop once so we don't have to search through it many times for each case
	for ( size_t i = 0; i < textureUnits.size(); i++ ) {
		// Already bound
		if ( textureUnits[i] == texture ) {
			glUniform1i( location, i + 1 );
			if ( textureSequenceStarted ) {
				textureSequence.insert( texture );
			}
			return;
		}

		if ( textureSequenceStarted && textureSequence.find( textureUnits[i] ) != textureSequence.end() ) {
			continue;
		}

		// Take note of the texture unit with the lowest priority texture
		if ( textureUnits[i] && textureUnits[i]->adjustedPriority < lowestPriority ) {
			lowestPriorityTexture = i;
			lowestPriority = textureUnits[i]->adjustedPriority;
		}
	}

	// Slot 0 is reserved for non-rendering OpenGL calls that require textures to be bound
	if ( textureUnits.size() + 1 < (size_t) glConfig2.maxTextureUnits ) {
		textureUnits.push_back( texture );
		glActiveTexture( GL_TEXTURE1 + textureUnits.size() - 1 );
		handle = textureUnits.size() - 1;
	}
	else {
		// No available texture units
		// Bind instead of the lowest priority texture
		textureUnits[lowestPriorityTexture] = texture;
		glActiveTexture( GL_TEXTURE1 + lowestPriorityTexture );
		handle = lowestPriorityTexture;
	}

	glBindTexture( texture->target, texture->textureHandle );
	glUniform1i( location, handle + 1 );
	if ( textureSequenceStarted ) {
		textureSequence.insert( texture );
	}

	GL_CheckErrors();
}

void TextureManager::AllNonResident() {
	for ( Texture* texture : textures ) {
		if ( texture->IsResident() ) {
			texture->MakeNonResident();
		}
	}
}

void TextureManager::BindReservedTexture( const GLenum target, const GLuint handle ) {
	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( target, handle );
}

// Texture units used within a texture sequence will not be bound to anything else until the texture sequence ends
void TextureManager::StartTextureSequence() {
	textureSequenceStarted = true;
}

void TextureManager::EndTextureSequence() {
	textureSequenceStarted = false;
	/* for ( const Texture* texture : textureSequence ) {
		const_cast< Texture* >( texture )->MakeNonResident();
	} */
	textureSequence.clear();
}

void TextureManager::FreeTextures() {
	textures.clear();
	textureUnits.clear();
}
