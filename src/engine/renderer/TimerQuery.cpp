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
// TimerQuery.cpp

#include "tr_local.h"
#include "TimerQuery.h"

GLTimerQuery::GLTimerQuery() {
}

GLTimerQuery::~GLTimerQuery() {
}

void GLTimerQuery::GenID() {
	glGenQueries( 1, &id );
	glGenQueries( 1, &id2 );
}

void GLTimerQuery::Delete() {
	glDeleteQueries( 1, &id );
	glDeleteQueries( 1, &id2 );
}

// This will add the query into OpenGL command stream that will only become signalled
// after all the previous commands have been executed
void GLTimerQuery::Insert() {
	if ( inProgress ) {
		// Log::Warn( "Tried to issue GL timer query again after it already begun" );
		return;
	}

	glQueryCounter( id, GL_TIMESTAMP );
	inProgress = true;
	issued = true;
	useBeginEnd = false;
}

void GLTimerQuery::Begin() {
	if ( inProgress ) {
		// Log::Warn( "Tried to issue GL timer query again after it already begun" );
		return;
	}

	glQueryCounter( id, GL_TIMESTAMP );
	inProgress = true;
	issued = true;
	useBeginEnd = true;
}

void GLTimerQuery::End() {
	if ( !inProgress ) {
		// Log::Warn( "Tried to end GL timer query that was never issued" );
		return;
	}

	glQueryCounter( id2, GL_TIMESTAMP );
	inProgress = false;
	useBeginEnd = true;
}

// This will retrieve the time in nanoseconds if this query was issued before, otherwise 0
// This will block the CPU until the query becomes available
GLuint64 GLTimerQuery::TimeCompleted( int timeout ) {
	if ( !issued ) {
		return 0;
	}

	GLint available = 0;
	int t1 = ri.Milliseconds();
	while ( !available ) {
		glGetQueryObjectiv( id, GL_QUERY_RESULT_AVAILABLE, &available );
		if ( ri.Milliseconds() - t1 > timeout ) {
			return 0;
		}
	}

	GLuint64 time;
	glGetQueryObjectui64v( id, GL_QUERY_RESULT, &time );

	if ( useBeginEnd ) {
		available = 0;
		t1 = ri.Milliseconds();
		while ( !available ) {
			glGetQueryObjectiv( id2, GL_QUERY_RESULT_AVAILABLE, &available );
			if ( ri.Milliseconds() - t1 > timeout ) {
				return 0;
			}
		}

		GLuint64 time2;
		glGetQueryObjectui64v( id2, GL_QUERY_RESULT, &time2 );
		time = time2 - time;
	}

	inProgress = false;
	issued = false;

	return time;
}

// This will return the current timestamp on the GPU after all the previous commands have been issued by the driver,
// but not necessarily executed
GLuint64 GLTimerQuery::TimeIssued() {
	GLint64 time;
	glGetInteger64v( GL_TIMESTAMP, &time );
	return time;
}

void R_InitTimerQueries() {
	for ( GLTimerQuery& glQuery : tr.GLTimerQueries ) {
		glQuery.GenID();
	}
}

void R_ShutdownTimerQueries() {
	for ( GLTimerQuery& glQuery : tr.GLTimerQueries ) {
		glQuery.Delete();
	}
}

void R_UpdateTimerQueries() {
	for ( uint32_t i = 0; i < Util::ordinal( TimerQuery::QUERY_COUNT ); i++ ) {
		GLuint64 glT1 = tr.GLTimerQueries[i].TimeCompleted( 1000 );
		tr.timers.glTime[i] = glT1;
	}
}

EngineTimers* RE_GetEngineTimers() {
	return &( tr.timers );
}
