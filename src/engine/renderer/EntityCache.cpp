/*
===========================================================================

Daemon BSD Source Code
Copyright (c) 2025 Daemon Developers
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
// EntityCache.cpp

#include "tr_local.h"

#include "EntityCache.h"

trRefEntity_t entities[MAX_REF_ENTITIES] {};

static constexpr uint32_t blockCount = PAD( MAX_REF_ENTITIES / 64, 64 );
static uint16_t highestActiveID = 0;
static uint64_t blocks[blockCount];

static void PositionEntityOnTag( trRefEntity_t* entity, const refEntity_t* parent, orientation_t* orientation ) {
	// FIXME: allow origin offsets along tag?
	VectorCopy( parent->origin, entity->e.origin );

	for ( int i = 0; i < 3; i++ ) {
		VectorMA( entity->e.origin, orientation->origin[i], parent->axis[i], entity->e.origin );
	}

	// had to cast away the const to avoid compiler problems...
	AxisMultiply( orientation->axis, ( ( refEntity_t* ) parent )->axis, entity->e.axis );
	entity->e.backlerp = parent->backlerp;
}

static void PositionRotatedEntityOnTag( trRefEntity_t* entity, const refEntity_t* parent, orientation_t* orientation ) {
	// FIXME: allow origin offsets along tag?
	VectorCopy( parent->origin, entity->e.origin );

	for ( int i = 0; i < 3; i++ ) {
		VectorMA( entity->e.origin, orientation->origin[i], parent->axis[i], entity->e.origin );
	}

	axis_t tempAxis;
	AxisMultiply( axisDefault, orientation->axis, tempAxis );
	AxisMultiply( tempAxis, ( ( refEntity_t* ) parent )->axis, entity->e.axis );
}

static void BuildSkeleton( trRefEntity_t* ent ) {
	if ( ent->e.scale == 0 ) {
		ent->e.scale = 1;
	}

	ent->skeleton.scale = ent->e.scale;

	if ( ent->e.animationHandle ) {
		RE_BuildSkeleton( &ent->skeleton, ent->e.animationHandle, ent->e.startFrame, ent->e.endFrame,
			ent->e.lerp, ent->e.clearOrigin );

		for ( const BoneMod& boneMod : ent->e.boneMods ) {
			if ( boneMod.type == BONE_ROTATE ) {
				QuatMultiply2( ent->skeleton.bones[boneMod.index].t.rot, boneMod.rotation );
			}
		}
	}

	refSkeleton_t skel;
	if ( ent->e.animationHandle2 ) {
		refSkeleton_t* skeleton2 = ent->e.animationHandle ? &skel : &ent->skeleton;

		RE_BuildSkeleton( skeleton2, ent->e.animationHandle2, ent->e.startFrame2, ent->e.endFrame2,
			ent->e.lerp2, ent->e.clearOrigin2 );

		for ( const BoneMod& boneMod : ent->e.boneMods ) {
			QuatMultiply2( skeleton2->bones[boneMod.index].t.rot, boneMod.rotation );
		}

		if ( ent->e.animationHandle && ent->e.blendLerp > 0.0 ) {
			RE_BlendSkeleton( &ent->skeleton, skeleton2, ent->e.blendLerp );
		}
	}

	if ( ent->e.animationHandle || ent->e.animationHandle2 ) {
		for ( const BoneMod& boneMod : ent->e.boneMods ) {
			if ( boneMod.type == BUILD_EXTRA_SKELETON ) {
				if ( ent->e.animationHandle ) {
					RE_BuildSkeleton( &skel, boneMod.animationHandle, boneMod.startFrame, boneMod.endFrame,
						boneMod.lerp, ent->e.clearOrigin );
				}
			} else if ( boneMod.type == BONE_FROM_EXTRA_SKELETON ) {
				ent->skeleton.bones[boneMod.index] = skel.bones[boneMod.index];
			}
		}

		R_TransformSkeleton( &ent->skeleton, ent->e.scale );
	} else {
		ent->skeleton.type = refSkeletonType_t::SK_ABSOLUTE;
		ent->skeleton.numBones = MAX_BONES;
		for ( int i = 0; i < MAX_BONES; i++ ) {
			ent->skeleton.bones[i].parentIndex = -1;
			TransInit( &ent->skeleton.bones[i].t );
		}
	}

	if ( ent->e.boundsAdd ) {
		matrix_t mat;
		vec3_t bounds[2];

		MatrixFromAngles( mat, ent->e.boundsRotation[0], ent->e.boundsRotation[1], ent->e.boundsRotation[2] );
		MatrixTransformBounds( mat, ent->skeleton.bounds[0], ent->skeleton.bounds[1], bounds[0], bounds[1] );
		BoundsAdd( ent->skeleton.bounds[0], ent->skeleton.bounds[1], bounds[0], bounds[1] );
	}
}

void TransformEntity( trRefEntity_t* ent ) {
	// FIXME: for some reason using == here breaks jet animation
	if ( ent->transformFrame > tr.frameCount ) {
		return;
	}

	switch ( ent->e.positionOnTag ) {
		case EntityTag::ON_TAG:
		{
			TransformEntity( &entities[ent->e.attachmentEntity] );

			orientation_t orientation;
			RE_LerpTagET( &orientation, &entities[ent->e.attachmentEntity], ent->e.tag.c_str(), 0 );
			PositionEntityOnTag( ent, &entities[ent->e.attachmentEntity].e, &orientation );
			break;
		}

		case EntityTag::ON_TAG_ROTATED:
		{
			TransformEntity( &entities[ent->e.attachmentEntity] );

			orientation_t orientation;
			RE_LerpTagET( &orientation, &entities[ent->e.attachmentEntity], ent->e.tag.c_str(), 0 );
			PositionRotatedEntityOnTag( ent, &entities[ent->e.attachmentEntity].e, &orientation );
			break;
		}

		case EntityTag::NONE:
		default:
			break;
	}

	switch ( ent->e.reType ) {
		case refEntityType_t::RT_PORTALSURFACE:
			break;

		case refEntityType_t::RT_SPRITE:
			break;

		case refEntityType_t::RT_MODEL:
			tr.currentModel = R_GetModelByHandle( ent->e.hModel );

			switch ( tr.currentModel->type ) {
				case modtype_t::MOD_MESH:
					break;

				case modtype_t::MOD_MD5:
					BuildSkeleton( ent );
					break;

				case modtype_t::MOD_IQM:
					BuildSkeleton( ent );
					break;

				case modtype_t::MOD_BSP:
				case modtype_t::MOD_BAD:
				default:
					break;
			}

			break;

		default:
			Sys::Drop( "TransformEntity: Bad reType" );
	}

	ent->transformFrame = tr.frameCount;
}

void AddRefEntities() {
	uint16_t highestFound = 0;

	for ( uint16_t i = 0; i < highestActiveID / 64 + 1; i++ ) {
		uint64_t block = blocks[i];

		while ( block ) {
			uint32_t offset = CountTrailingZeroes( block );

			trRefEntity_t* ent = &entities[offset + i * 64];

			TransformEntity( ent );
			RE_AddEntityToScene( ent );

			block &= offset == 63 ? 0 : ( UINT64_MAX << ( offset + 1 ) );

			highestFound = offset + i * 64;
		}
	}

	highestActiveID = highestFound;
}

void ClearEntityCache() {
	highestActiveID = 0;

	memset( blocks, 0, blockCount * sizeof( uint64_t ) );

	for ( trRefEntity_t& ent : entities ) {
		ent = {};
	}
}

std::vector<LerpTagSync> SyncEntityCacheToCGame( const std::vector<LerpTagUpdate>& lerpTags ) {
	std::vector<LerpTagSync> entityOrientations;
	entityOrientations.reserve( lerpTags.size() );

	for ( const LerpTagUpdate& tag : lerpTags ) {
		TransformEntity( &entities[tag.id] );

		orientation_t orientation;
		RE_LerpTagET( &orientation, &entities[tag.id], tag.tag.c_str(), 0 );

		orientation_t entOrientation;
		VectorCopy( entities[tag.id].e.origin, entOrientation.origin );
		AxisCopy( entities[tag.id].e.axis, entOrientation.axis );

		entityOrientations.emplace_back( LerpTagSync { entOrientation, orientation } );
	}

	return entityOrientations;
}

void SyncEntityCacheFromCGame( const std::vector<EntityUpdate>& ents ) {
	for ( const EntityUpdate& ent : ents ) {
		bool flip = entities[ent.id].e.active != ent.ent.active;

		if ( ent.ent.positionOnTag == EntityTag::NONE || entities[ent.id].transformFrame != tr.frameCount ) {
			entities[ent.id].e = ent.ent;
		} else {
			vec3_t origin;
			VectorCopy( entities[ent.id].e.origin, origin );

			axis_t axis;
			AxisCopy( entities[ent.id].e.axis, axis );

			entities[ent.id].e = ent.ent;

			VectorCopy( origin, entities[ent.id].e.origin );
			AxisCopy( axis, entities[ent.id].e.axis );
		}

		AxisCopy( ent.ent.axis, entities[ent.id].axis );
		blocks[ent.id / 64] ^= ( 1ull << ( ent.id & 63 ) ) * flip;

		if ( ent.ent.active ) {
			highestActiveID = std::max( highestActiveID, ent.id );
		}
	}
}