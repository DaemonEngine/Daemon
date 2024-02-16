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
// TextureManager.h

#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#include <vector>
#include <unordered_set>
#include "GL/glew.h"

enum {
	TEXTURE_PRIORITY_LOW = 0,
	TEXTURE_PRIORITY_MEDIUM = 1,
	TEXTURE_PRIORITY_HIGH = 2,
	TEXTURE_PRIORITY_PERSISTENT = 5,
};

class Texture {
	public:
	GLuint textureHandle = 0;
	GLuint64 bindlessTextureHandle = 0;
	bool hasBindlessHandle = false;

	int frameBindCounter = 0;
	int totalBindCounter = 0;

	GLenum target = GL_TEXTURE_2D;

	int basePriority = 0;
	float adjustedPriority = 0.0f;

	Texture();
	~Texture();

	void UpdateAdjustedPriority( const int totalFrameTextureBinds, const int totalTextureBinds );

	bool IsResident() const;
	void MakeResident();
	void MakeNonResident();

	void GenBindlessHandle();

	struct Compare {
		bool operator() ( const Texture* lhs, const Texture* rhs ) {
			if ( lhs->adjustedPriority != rhs->adjustedPriority ) {
				return lhs->adjustedPriority > rhs->adjustedPriority;
			}
			return lhs->adjustedPriority < rhs->adjustedPriority;
		}
	};

	private:
		bool bindlessTextureResident = false;
};

class TextureManager {
	public:
	TextureManager();
	~TextureManager();

	void UpdateAdjustedPriorities();

	void BindTexture( const GLint location, Texture* texture );
	void AllNonResident();
	void BindReservedTexture( const GLenum target, const GLuint handle );
	void StartTextureSequence();
	void EndTextureSequence();
	void FreeTextures();

	private:
		std::vector<const Texture*> textureUnits;
		std::vector<Texture*> textures;
		std::unordered_set<const Texture*> textureSequence;

		bool textureSequenceStarted;

		int totalFrameTextureBinds;
		int totalTextureBinds;
};

// extern TextureManager textureManager;

#endif
