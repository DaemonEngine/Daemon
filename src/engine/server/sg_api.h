/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2011 Dusan Jocic <dusanjocic@msn.com>

Daemon is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Daemon is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#ifndef SGAPI_H
#define SGAPI_H

#include "engine/qcommon/q_shared.h"

// flags for masking which clients can see the entity
#define SVF_CLIENTMASK            BIT( 0 )
#define SVF_SINGLECLIENT          BIT( 1 )
#define SVF_NOCLIENT              BIT( 2 )
#define SVF_NOTSINGLECLIENT       BIT( 3 )

// flags for modifying visibility
#define SVF_BROADCAST             BIT( 4 ) // visible from anywhere
#define SVF_BROADCAST_ONCE        BIT( 5 ) // broadcasted to newly connecting clients, and once to connected clients when spawned
#define SVF_CLIENTS_IN_RANGE      BIT( 6 )
#define SVF_IGNOREBMODELEXTENTS   BIT( 7 )

#define SVF_PORTAL                BIT( 8 ) // if you see the portal, you also see what can be seen through its camera
#define SVF_NOSERVERINFO          BIT( 9 ) // only meaningful for entities numbered in [0..MAX_CLIENTS)

// ???
#define SVF_VISDUMMY              BIT( 10 )
#define SVF_VISDUMMY_MULTIPLE     BIT( 11 )
#define SVF_SELF_PORTAL           BIT( 12 )
#define SVF_SELF_PORTAL_EXCLUSIVE BIT( 13 )


#define MAX_ENT_CLUSTERS  16

struct entityShared_t
{
	bool linked; // false if not in any good cluster
	int      linkcount;

	int      svFlags; // SVF_*, flags for controlling which entities are sent to which clients
	int      singleClient; // only send to this client when SVF_SINGLECLIENT is set
	int      hiMask, loMask; // if SVF_CLIENTMASK is set, then only send to the
	//  clients specified by the following 64-bit bitmask:
	//  hiMask: high-order bits (32..63)
	//  loMask: low-order bits (0..31)
	float    clientRadius;    // if SVF_CLIENTS_IN_RANGE, send to all clients within this range

	bool bmodel; // if false, assume an explicit mins/maxs bounding box
	// only set by trap_SetBrushModel
	vec3_t   mins, maxs;

	// CONTENTS_TRIGGER, CONTENTS_SOLID, CONTENTS_BODY, etc.
	// used when tracing other entities against this one
	// a non-solid entity should have this set to 0
	int contents;

	vec3_t absmin, absmax; // derived from mins/maxs and origin + rotation

	// currentOrigin will be used for all collision detection and world linking.
	// it will not necessarily be the same as the trajectory evaluation for the current
	// time, because each entity must be moved one at a time after time is advanced
	// to avoid simultaneous collision issues
	vec3_t currentOrigin;
	vec3_t currentAngles;

	// when a trace call is made and the specified pass entity isn't none,
	//  then a given entity will be excluded from testing if:
	// - the given entity is the pass entity (use case: don't interact with self),
	// - the owner of the given entity is the pass entity (use case: don't interact with your own missiles), or
	// - the given entity and the pass entity have the same owner entity (that is not none)
	//    (use case: don't interact with other missiles from owner).
	// that is, ent will be excluded if
	// ( passEntityNum != ENTITYNUM_NONE &&
	//   ( ent->s.number == passEntityNum || ent->r.ownerNum == passEntityNum ||
	//     ( ent->r.ownerNum != ENTITYNUM_NONE && ent->r.ownerNum == entities[passEntityNum].r.ownerNum ) ) )
	int      ownerNum;

	int numClusters; // if -1, use headnode instead
	int clusternums[ MAX_ENT_CLUSTERS ];
	int lastCluster; // if all the clusters don't fit in clusternums
	int originCluster; // Gordon: calced upon linking, for origin only bmodel vis checks
	int areanum, areanum2;
};

// the server looks at a sharedEntity_t structure, which must be at the start of a gentity_t structure
struct sharedEntity_t
{
	entityState_t  s; // communicated by the server to clients
	entityShared_t r; // shared by both the server and game module
};

#endif
