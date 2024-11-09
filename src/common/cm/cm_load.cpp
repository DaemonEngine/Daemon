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

#include "cm_local.h"

#include <common/FileSystem.h>

// to allow boxes to be treated as brush models, we allocate
// some extra indexes along with those needed by the map
static const int BOX_LEAF_BRUSHES = 1; // ydnar
static const int BOX_BRUSHES      = 1;
static const int BOX_SIDES        = 6;
static const int BOX_LEAFS        = 2;
static const int BOX_PLANES       = 12;

#define LL( x ) x = LittleLong( x )

clipMap_t cm;
int       c_pointcontents;
int       c_traces, c_brush_traces, c_patch_traces, c_trisoup_traces;

static cmodel_t  box_model;
static cplane_t  *box_planes;
static cbrush_t  *box_brush;

void      CM_InitBoxHull();
void      CM_FloodAreaConnections();

Cvar::Cvar<bool> cm_forceTriangles(VM_STRING_PREFIX "cm_forceTriangles", "Convert all patches into triangles?", Cvar::CHEAT | Cvar::ROM, false);
Log::Logger cmLog(VM_STRING_PREFIX "common.cm");

static std::vector<void*> allocations;

void* CM_Alloc( size_t size )
{
    void* alloc = calloc(size, 1);
    if (!alloc && size) Sys::Error("CM_Alloc: Out of memory");
    allocations.push_back(alloc);
    return alloc;
}

void CM_FreeAll()
{
    for (auto alloc : allocations)
    {
        free(alloc);
    }
    allocations.clear();
}

/*
===============================================================================

                                        MAP LOADING

===============================================================================
*/

/*
=================
CMod_LoadShaders
=================
*/
static void CMod_LoadShaders(const byte *const cmod_base, const lump_t *l)
{
	dshader_t *in, *out;
	int       i, count;

	in = ( dshader_t * )( cmod_base + l->fileofs );

	if ( l->filelen % sizeof( *in ) )
	{
		Sys::Drop( "CMod_LoadShaders: funny lump size" );
	}

	count = l->filelen / sizeof( *in );

	if ( count < 1 )
	{
		Sys::Drop( "Map with no shaders" );
	}

	cm.shaders = ( dshader_t * ) CM_Alloc( count * sizeof( *cm.shaders ) );
	cm.numShaders = count;

	memcpy( cm.shaders, in, count * sizeof( *cm.shaders ) );

	//FIXME: use a saner way to act on endianness, please
	if ( LittleLong( 1 ) != 1 )
	{
		out = cm.shaders;

		for ( i = 0; i < count; i++, in++, out++ )
		{
			out->contentFlags = LittleLong( out->contentFlags );
			out->surfaceFlags = LittleLong( out->surfaceFlags );
		}
	}
}

/*
=================
CMod_LoadSubmodels
=================
*/
static void CMod_LoadSubmodels(const byte *const cmod_base, const lump_t *l)
{
	dmodel_t *in;
	cmodel_t *out;
	int      i, j, count;
	int      *indexes;

	in = ( dmodel_t * )( cmod_base + l->fileofs );

	if ( l->filelen % sizeof( *in ) )
	{
		Sys::Drop( "CMod_LoadSubmodels: funny lump size" );
	}

	count = l->filelen / sizeof( *in );

	if ( count < 1 )
	{
		Sys::Drop( "Map with no models" );
	}

	cm.cmodels = ( cmodel_t * ) CM_Alloc( count * sizeof( *cm.cmodels ) );
	cm.numSubModels = count;

	for ( i = 0; i < count; i++, in++, out++ )
	{
		out = &cm.cmodels[ i ];

		for ( j = 0; j < 3; j++ )
		{
			// spread the mins / maxs by a pixel
			out->mins[ j ] = LittleFloat( in->mins[ j ] ) - 1;
			out->maxs[ j ] = LittleFloat( in->maxs[ j ] ) + 1;
		}

		if ( i == 0 )
		{
			continue; // world model doesn't need other info
		}

		// make a "leaf" just to hold the model's brushes and surfaces
		out->leaf.numLeafBrushes = LittleLong( in->numBrushes );
		indexes = ( int * ) CM_Alloc( out->leaf.numLeafBrushes * 4 );
		out->leaf.firstLeafBrush = indexes;

		for ( j = 0; j < out->leaf.numLeafBrushes; j++ )
		{
			indexes[ j ] = LittleLong( in->firstBrush ) + j;
		}

		out->leaf.numLeafSurfaces = LittleLong( in->numSurfaces );
		indexes = ( int * ) CM_Alloc( out->leaf.numLeafSurfaces * 4 );
		out->leaf.firstLeafSurface = indexes;

		for ( j = 0; j < out->leaf.numLeafSurfaces; j++ )
		{
			indexes[ j ] = LittleLong( in->firstSurface ) + j;
		}
	}
}

/*
=================
CMod_LoadNodes

=================
*/
static void CMod_LoadNodes(const byte *const cmod_base, const lump_t *l)
{
	dnode_t *in;
	int     child;
	cNode_t *out;
	int     i, j, count;

	in = ( dnode_t * )( cmod_base + l->fileofs );

	if ( l->filelen % sizeof( *in ) )
	{
		Sys::Drop( "MOD_LoadBmodel: funny lump size" );
	}

	count = l->filelen / sizeof( *in );

	if ( count < 1 )
	{
		Sys::Drop( "Map has no nodes" );
	}

	cm.nodes = ( cNode_t * ) CM_Alloc( count * sizeof( *cm.nodes ) );
	cm.numNodes = count;

	out = cm.nodes;

	for ( i = 0; i < count; i++, out++, in++ )
	{
		out->plane = cm.planes + LittleLong( in->planeNum );

		for ( j = 0; j < 2; j++ )
		{
			child = LittleLong( in->children[ j ] );
			out->children[ j ] = child;
		}
	}
}

/*
=================
CM_BoundBrush

=================
*/
void CM_BoundBrush( cbrush_t *b )
{
	b->bounds[ 0 ][ 0 ] = -b->sides[ 0 ].plane->dist;
	b->bounds[ 1 ][ 0 ] = b->sides[ 1 ].plane->dist;

	b->bounds[ 0 ][ 1 ] = -b->sides[ 2 ].plane->dist;
	b->bounds[ 1 ][ 1 ] = b->sides[ 3 ].plane->dist;

	b->bounds[ 0 ][ 2 ] = -b->sides[ 4 ].plane->dist;
	b->bounds[ 1 ][ 2 ] = b->sides[ 5 ].plane->dist;
}

/*
=================
CMod_LoadBrushes

=================
*/
static void CMod_LoadBrushes(const byte *const cmod_base, const lump_t *l)
{
	dbrush_t *in;
	cbrush_t *out;
	int      i, count;
	int      shaderNum;

	in = ( dbrush_t * )( cmod_base + l->fileofs );

	if ( l->filelen % sizeof( *in ) )
	{
		Sys::Drop( "MOD_LoadBmodel: funny lump size" );
	}

	count = l->filelen / sizeof( *in );

	cm.brushes = ( cbrush_t * ) CM_Alloc( ( BOX_BRUSHES + count ) * sizeof( *cm.brushes ) );
	cm.numBrushes = count;

	out = cm.brushes;

	for ( i = 0; i < count; i++, out++, in++ )
	{
		out->sides = cm.brushsides + LittleLong( in->firstSide );
		out->numsides = LittleLong( in->numSides );

		shaderNum = LittleLong( in->shaderNum );

		if ( shaderNum < 0 || shaderNum >= cm.numShaders )
		{
			Sys::Drop( "CMod_LoadBrushes: bad shaderNum: %i", shaderNum );
		}

		out->contents = cm.shaders[ shaderNum ].contentFlags;

		CM_BoundBrush( out );
	}
}

/*
=================
CMod_LoadLeafs
=================
*/
static void CMod_LoadLeafs(const byte *const cmod_base, const lump_t *l)
{
	int     i;
	cLeaf_t *out;
	dleaf_t *in;
	int     count;

	in = ( dleaf_t * )( cmod_base + l->fileofs );

	if ( l->filelen % sizeof( *in ) )
	{
		Sys::Drop( "MOD_LoadBmodel: funny lump size" );
	}

	count = l->filelen / sizeof( *in );

	if ( count < 1 )
	{
		Sys::Drop( "Map with no leafs" );
	}

	cm.leafs = ( cLeaf_t * ) CM_Alloc( ( BOX_LEAFS + count ) * sizeof( *cm.leafs ) );
	cm.numLeafs = count;

	out = cm.leafs;

	for ( i = 0; i < count; i++, in++, out++ )
	{
		out->cluster = LittleLong( in->cluster );
		out->area = LittleLong( in->area );
		out->firstLeafBrush = cm.leafbrushes + LittleLong( in->firstLeafBrush );
		out->numLeafBrushes = LittleLong( in->numLeafBrushes );
		out->firstLeafSurface = cm.leafsurfaces + LittleLong( in->firstLeafSurface );
		out->numLeafSurfaces = LittleLong( in->numLeafSurfaces );

		if ( out->cluster >= cm.numClusters )
		{
			cm.numClusters = out->cluster + 1;
		}

		if ( out->area >= cm.numAreas )
		{
			cm.numAreas = out->area + 1;
		}
	}

	cm.areas = ( cArea_t * ) CM_Alloc( cm.numAreas * sizeof( *cm.areas ) );
	cm.areaPortals = ( int * ) CM_Alloc( cm.numAreas * cm.numAreas * sizeof( *cm.areaPortals ) );
}

/*
=================
CMod_LoadPlanes
=================
*/
static void CMod_LoadPlanes(const byte *const cmod_base, const lump_t *l)
{
	int      i, j;
	cplane_t *out;
	dplane_t *in;
	int      count;

	in = ( dplane_t * )( cmod_base + l->fileofs );

	if ( l->filelen % sizeof( *in ) )
	{
		Sys::Drop( "MOD_LoadBmodel: funny lump size" );
	}

	count = l->filelen / sizeof( *in );

	if ( count < 1 )
	{
		Sys::Drop( "Map with no planes" );
	}

	cm.planes = ( cplane_t * ) CM_Alloc( ( BOX_PLANES + count ) * sizeof( *cm.planes ) );
	cm.numPlanes = count;

	out = cm.planes;

	for ( i = 0; i < count; i++, in++, out++ )
	{
		for ( j = 0; j < 3; j++ )
		{
			out->normal[ j ] = LittleFloat( in->normal[ j ] );
		}

		out->dist = LittleFloat( in->dist );
		out->type = PlaneTypeForNormal( out->normal );
		SetPlaneSignbits( out );
	}
}

/*
=================
CMod_LoadLeafBrushes
=================
*/
static void CMod_LoadLeafBrushes(const byte *const cmod_base, const lump_t *l)
{
	int i;
	int *out;
	int *in;
	int count;

	in = ( int * )( cmod_base + l->fileofs );

	if ( l->filelen % sizeof( *in ) )
	{
		Sys::Drop( "MOD_LoadBmodel: funny lump size" );
	}

	count = l->filelen / sizeof( *in );

	// ydnar: more than <count> brushes are stored in leafbrushes...
	cm.leafbrushes = ( int * ) CM_Alloc( ( BOX_LEAF_BRUSHES + count ) * sizeof( *cm.leafbrushes ) );
	cm.numLeafBrushes = count;

	out = cm.leafbrushes;

	for ( i = 0; i < count; i++, in++, out++ )
	{
		*out = LittleLong( *in );
	}
}

/*
=================
CMod_LoadLeafSurfaces
=================
*/
static void CMod_LoadLeafSurfaces(const byte *const cmod_base, const lump_t *l)
{
	int i;
	int *out;
	int *in;
	int count;

	in = ( int * )( cmod_base + l->fileofs );

	if ( l->filelen % sizeof( *in ) )
	{
		Sys::Drop( "MOD_LoadBmodel: funny lump size" );
	}

	count = l->filelen / sizeof( *in );

	cm.leafsurfaces = ( int * ) CM_Alloc( count * sizeof( *cm.leafsurfaces ) );
	cm.numLeafSurfaces = count;

	out = cm.leafsurfaces;

	for ( i = 0; i < count; i++, in++, out++ )
	{
		*out = LittleLong( *in );
	}
}

/*
=================
CMod_LoadBrushSides
=================
*/
static void CMod_LoadBrushSides(const byte *const cmod_base, const lump_t *l)
{
	int          i;
	cbrushside_t *out;
	dbrushside_t *in;
	int          count;
	int          num;
	int          shaderNum;

	in = ( dbrushside_t * )( cmod_base + l->fileofs );

	if ( l->filelen % sizeof( *in ) )
	{
		Sys::Drop( "MOD_LoadBmodel: funny lump size" );
	}

	count = l->filelen / sizeof( *in );

	cm.brushsides = ( cbrushside_t * ) CM_Alloc( ( BOX_SIDES + count ) * sizeof( *cm.brushsides ) );
	cm.numBrushSides = count;

	out = cm.brushsides;

	for ( i = 0; i < count; i++, in++, out++ )
	{
		num = LittleLong( in->planeNum );
		out->plane = &cm.planes[ num ];
		shaderNum = LittleLong( in->shaderNum );

		if ( shaderNum < 0 || shaderNum >= cm.numShaders )
		{
			Sys::Drop( "CMod_LoadBrushSides: bad shaderNum: %i", shaderNum );
		}

		out->surfaceFlags = cm.shaders[ shaderNum ].surfaceFlags;
	}
}

/*
=================
CMod_LoadEntityString
=================
*/
static void CMod_LoadEntityString(const byte *const cmod_base, const lump_t *l, std::string &externalEntities)
{
	const char *p, *token;
	char keyname[ MAX_TOKEN_CHARS ];
	char value[ MAX_TOKEN_CHARS ];

	if ( externalEntities.empty() )
	{
		cm.entityString = ( char * ) CM_Alloc( l->filelen + 1);
		cm.numEntityChars = l->filelen;
		memcpy( cm.entityString, cmod_base + l->fileofs, l->filelen );
		cm.entityString[l->filelen] = '\0';
	}
	else
	{
		int len = externalEntities.length();
		cm.entityString = ( char * ) CM_Alloc( len + 1 );
		cm.numEntityChars = len;
		memcpy( cm.entityString, externalEntities.c_str(), len + 1 );
	}

	p = cm.entityString;

	// only parse the world spawn
	while (true)
	{
		// parse key
		token = COM_ParseExt2( &p, true );

		if ( !*token )
		{
			Log::Warn( "unexpected end of entities string while parsing worldspawn" );
			break;
		}

		if ( *token == '{' )
		{
			continue;
		}

		if ( *token == '}' )
		{
			break;
		}

		Q_strncpyz( keyname, token, sizeof( keyname ) );

		// parse value
		token = COM_ParseExt2( &p, false );

		if ( !*token )
		{
			continue;
		}

		Q_strncpyz( value, token, sizeof( value ) );

		// check for per-poly collision support
		if ( !Q_stricmp( keyname, "perPolyCollision" ) && !Q_stricmp( value, "1" ) )
		{
			Log::Notice( "map features per poly collision detection" );
			cm.perPolyCollision = true;
			continue;
		}

		if ( !Q_stricmp( keyname, "classname" ) && Q_stricmp( value, "worldspawn" ) )
		{
			Log::Warn( "expected worldspawn, found '%s'", value );
			break;
		}
	}
}

/*
=================
CMod_LoadVisibility
=================
*/
static const int VIS_HEADER = 8;
static void CMod_LoadVisibility(const byte *const cmod_base, const lump_t *l)
{
	int len = l->filelen;

	if ( !len )
	{
		cm.clusterBytes = ( cm.numClusters + 31 ) & ~31;
		cm.visibility = ( byte * ) CM_Alloc( cm.clusterBytes );
		memset( cm.visibility, 255, cm.clusterBytes );
		return;
	}

	const byte *buf = cmod_base + l->fileofs;

	cm.vised = true;
	cm.visibility = ( byte * ) CM_Alloc( len - VIS_HEADER );
	cm.numClusters = LittleLong( ( ( int * ) buf ) [ 0 ] );
	cm.clusterBytes = LittleLong( ( ( int * ) buf ) [ 1 ] );
	memcpy( cm.visibility, buf + VIS_HEADER, len - VIS_HEADER );
}

//==================================================================

/*
=================
CMod_LoadSurfaces
=================
*/
static const int MAX_PATCH_SIZE  = 64;
static const int MAX_PATCH_VERTS = ( MAX_PATCH_SIZE * MAX_PATCH_SIZE );
static void CMod_LoadSurfaces(const byte *const cmod_base, const lump_t *surfs, const lump_t *verts, const lump_t *indexesLump)
{
	drawVert_t    *dv, *dv_p;
	dsurface_t    *in;
	int           count;
	int           i;
	cSurface_t    *surface;
	int           numVertexes;
	static vec3_t vertexes[ SHADER_MAX_VERTEXES ];
	int           width, height;
	int           shaderNum;
	int           numIndexes;
	static int    indexes[ SHADER_MAX_INDEXES ];
	int           *index;
	int           *index_p;

	in = ( dsurface_t * )( cmod_base + surfs->fileofs );

	if ( surfs->filelen % sizeof( *in ) )
	{
		Sys::Drop( "CMod_LoadSurfaces: funny lump size" );
	}

	cm.numSurfaces = count = surfs->filelen / sizeof( *in );
	cm.surfaces = ( cSurface_t ** ) CM_Alloc( cm.numSurfaces * sizeof( cm.surfaces[ 0 ] ) );

	dv = ( drawVert_t * )( cmod_base + verts->fileofs );

	if ( verts->filelen % sizeof( *dv ) )
	{
		Sys::Drop( "CMod_LoadSurfaces: funny lump size" );
	}

	index = ( int * )( cmod_base + indexesLump->fileofs );

	if ( indexesLump->filelen % sizeof( *index ) )
	{
		Sys::Drop( "CMod_LoadSurfaces: funny lump size" );
	}

	// scan through all the surfaces
	for ( i = 0; i < count; i++, in++ )
	{
		if ( LittleLong( in->surfaceType ) == mapSurfaceType_t::MST_PATCH )
		{
			// FIXME: check for non-colliding patches
			cm.surfaces[ i ] = surface = ( cSurface_t * ) CM_Alloc( sizeof( *surface ) );
			surface->type = mapSurfaceType_t::MST_PATCH;

			// load the full drawverts onto the stack
			width = LittleLong( in->patchWidth );
			height = LittleLong( in->patchHeight );
			numVertexes = width * height;

			if ( numVertexes > MAX_PATCH_VERTS )
			{
				Sys::Drop( "CMod_LoadSurfaces: MAX_PATCH_VERTS" );
			}

			dv_p = dv + LittleLong( in->firstVert );

			for ( int j = 0; j < numVertexes; j++, dv_p++ )
			{
				vertexes[ j ][ 0 ] = LittleFloat( dv_p->xyz[ 0 ] );
				vertexes[ j ][ 1 ] = LittleFloat( dv_p->xyz[ 1 ] );
				vertexes[ j ][ 2 ] = LittleFloat( dv_p->xyz[ 2 ] );
			}

			shaderNum = LittleLong( in->shaderNum );
			surface->contents = cm.shaders[ shaderNum ].contentFlags;
			surface->surfaceFlags = cm.shaders[ shaderNum ].surfaceFlags;

			// create the internal facet structure
			surface->sc = CM_GeneratePatchCollide( width, height, vertexes );
		}
		else if ( LittleLong( in->surfaceType ) == mapSurfaceType_t::MST_TRIANGLE_SOUP && ( cm.perPolyCollision || cm_forceTriangles.Get() ) )
		{
			// FIXME: check for non-colliding triangle soups

			cm.surfaces[ i ] = surface = ( cSurface_t * ) CM_Alloc( sizeof( *surface ) );
			surface->type = mapSurfaceType_t::MST_TRIANGLE_SOUP;

			// load the full drawverts onto the stack
			numVertexes = LittleLong( in->numVerts );

			if ( numVertexes > SHADER_MAX_VERTEXES )
			{
				Sys::Drop( "CMod_LoadSurfaces: SHADER_MAX_VERTEXES" );
			}

			dv_p = dv + LittleLong( in->firstVert );

			for ( int j = 0; j < numVertexes; j++, dv_p++ )
			{
				vertexes[ j ][ 0 ] = LittleFloat( dv_p->xyz[ 0 ] );
				vertexes[ j ][ 1 ] = LittleFloat( dv_p->xyz[ 1 ] );
				vertexes[ j ][ 2 ] = LittleFloat( dv_p->xyz[ 2 ] );
			}

			numIndexes = LittleLong( in->numIndexes );

			if ( numIndexes > SHADER_MAX_INDEXES )
			{
				Sys::Drop( "CMod_LoadSurfaces: SHADER_MAX_INDEXES" );
			}

			index_p = index + LittleLong( in->firstIndex );

			for ( int j = 0; j < numIndexes; j++, index_p++ )
			{
				indexes[ j ] = LittleLong( *index_p );

				if ( indexes[ j ] < 0 || indexes[ j ] >= numVertexes )
				{
					Sys::Drop( "CMod_LoadSurfaces: Bad index in trisoup surface" );
				}
			}

			shaderNum = LittleLong( in->shaderNum );
			surface->contents = cm.shaders[ shaderNum ].contentFlags;
			surface->surfaceFlags = cm.shaders[ shaderNum ].surfaceFlags;

			// create the internal facet structure
			surface->sc = CM_GenerateTriangleSoupCollide( numVertexes, vertexes, numIndexes, indexes );
		}
	}
}

//==================================================================

/*
==================
CM_LoadMap

Loads in the map and all submodels
==================
*/
void CM_LoadMap(Str::StringRef name)
{
	dheader_t       header;

	cmLog.Debug( "CM_LoadMap(%s)", name);

	std::string mapFile = "maps/" + name + ".bsp";

	std::error_code err;
	std::string mapData = FS::PakPath::ReadFile(mapFile, err);
	if (err) {
		Sys::Drop("Could not load %s: %s (code: %d)", mapFile.c_str(), err.message(), err.value() );
	}

	std::string externalEntities = FS::PakPath::ReadFile( "maps/" + name + ".ent", err );
	if ( err )
	{
		const std::error_code notFound( Util::ordinal( FS::filesystem_error::no_such_file ), FS::filesystem_category() );
		if ( err != notFound )
		{
			Sys::Drop( "Could not read file 'maps/%s.ent': %s", name.c_str(), err.message() );
		}
		externalEntities = "";
	}

	// clear collision map data
	CM_ClearMap();

	if ( !name[ 0 ] )
	{
		cm.numLeafs = 1;
		cm.numClusters = 1;
		cm.numAreas = 1;
		cm.cmodels = ( cmodel_t * ) CM_Alloc( sizeof( *cm.cmodels ) );
		return;
	}

	header = * ( dheader_t * ) mapData.data();

	for (unsigned i = 0; i < sizeof( dheader_t ) / 4; i++ )
	{
		( ( int * ) &header ) [ i ] = LittleLong( ( ( int * ) &header ) [ i ] );
	}

	if ( header.version != BSP_VERSION && header.version != BSP_VERSION_Q3 )
	{
		Sys::Drop( "CM_LoadMap: %s has wrong version number (%i should be %i for ET or %i for Q3)",
		           name.c_str(), header.version, BSP_VERSION, BSP_VERSION_Q3 );
	}

	const byte *const cmod_base = reinterpret_cast<const byte*>(mapData.data());

	// load into heap
	CMod_LoadShaders(cmod_base, &header.lumps[LUMP_SHADERS]);
	CMod_LoadLeafBrushes(cmod_base, &header.lumps[LUMP_LEAFBRUSHES]);
	CMod_LoadLeafSurfaces(cmod_base, &header.lumps[LUMP_LEAFSURFACES]);
	CMod_LoadLeafs(cmod_base, &header.lumps[LUMP_LEAFS]);
	CMod_LoadPlanes(cmod_base, &header.lumps[LUMP_PLANES]);
	CMod_LoadBrushSides(cmod_base, &header.lumps[LUMP_BRUSHSIDES]);
	CMod_LoadBrushes(cmod_base, &header.lumps[LUMP_BRUSHES]);
	CMod_LoadSubmodels(cmod_base, &header.lumps[LUMP_MODELS]);
	CMod_LoadNodes(cmod_base, &header.lumps[LUMP_NODES]);
	CMod_LoadEntityString(cmod_base, &header.lumps[LUMP_ENTITIES], externalEntities);
	CMod_LoadVisibility(cmod_base, &header.lumps[LUMP_VISIBILITY]);
	CMod_LoadSurfaces(cmod_base,
					  &header.lumps[LUMP_SURFACES], &header.lumps[LUMP_DRAWVERTS], &header.lumps[LUMP_DRAWINDEXES]);

	CM_InitBoxHull();

	CM_FloodAreaConnections();
}

/*
==================
CM_ClearMap
==================
*/
void CM_ClearMap()
{
	CM_FreeAll();
	ResetStruct( cm );
}

/*
==================
CM_ClipHandleToModel
==================
*/
cmodel_t       *CM_ClipHandleToModel( clipHandle_t handle )
{
	if ( handle < 0 )
	{
		Sys::Drop( "CM_ClipHandleToModel: bad handle %i", handle );
	}

	if ( handle < cm.numSubModels )
	{
		return &cm.cmodels[ handle ];
	}

	if ( handle == BOX_MODEL_HANDLE || handle == CAPSULE_MODEL_HANDLE )
	{
		return &box_model;
	}

	Sys::Drop( "CM_ClipHandleToModel: bad handle %i (max %d)", handle, cm.numSubModels );
}

/*
==================
CM_InlineModel
==================
*/
clipHandle_t CM_InlineModel( int index )
{
	if ( index < 0 || index >= cm.numSubModels )
	{
		Sys::Drop( "CM_InlineModel: bad number" );
	}

	return index;
}

int CM_NumInlineModels()
{
	return cm.numSubModels;
}

char           *CM_EntityString()
{
	return cm.entityString;
}

int CM_LeafCluster( int leafnum )
{
	if ( leafnum < 0 || leafnum >= cm.numLeafs )
	{
		Sys::Drop( "CM_LeafCluster: bad number" );
	}

	return cm.leafs[ leafnum ].cluster;
}

int CM_LeafArea( int leafnum )
{
	if ( leafnum < 0 || leafnum >= cm.numLeafs )
	{
		Sys::Drop( "CM_LeafArea: bad number" );
	}

	return cm.leafs[ leafnum ].area;
}

//=======================================================================

/*
===================
CM_InitBoxHull

Set up the planes and nodes so that the six floats of a bounding box
can just be stored out and get a proper clipping hull structure.
===================
*/
void CM_InitBoxHull()
{
	int          i;
	int          side;
	cplane_t     *p;
	cbrushside_t *s;

	box_planes = &cm.planes[ cm.numPlanes ];

	box_brush = &cm.brushes[ cm.numBrushes ];
	box_brush->numsides = 6;
	box_brush->sides = cm.brushsides + cm.numBrushSides;
	box_brush->contents = CONTENTS_BODY;

	box_model.leaf.numLeafBrushes = 1;
	box_model.leaf.firstLeafBrush = cm.leafbrushes + cm.numLeafBrushes;
	cm.leafbrushes[ cm.numLeafBrushes ] = cm.numBrushes;

	for ( i = 0; i < 6; i++ )
	{
		side = i & 1;

		// brush sides
		s = &cm.brushsides[ cm.numBrushSides + i ];
		s->plane = cm.planes + ( cm.numPlanes + i * 2 + side );
		s->surfaceFlags = 0;

		// planes
		p = &box_planes[ i * 2 ];
		p->type = i >> 1;
		p->signbits = 0;
		VectorClear( p->normal );
		p->normal[ i >> 1 ] = 1;

		p = &box_planes[ i * 2 + 1 ];
		p->type = 3 + ( i >> 1 );
		p->signbits = 0;
		VectorClear( p->normal );
		p->normal[ i >> 1 ] = -1;

		SetPlaneSignbits( p );
	}
}

/*
===================
CM_TempBoxModel

To keep everything totally uniform, bounding boxes are turned into small
BSP trees instead of being compared directly.
Capsules are handled differently though.
===================
*/
clipHandle_t CM_TempBoxModel( const vec3_t mins, const vec3_t maxs, bool capsule )
{
	VectorCopy( mins, box_model.mins );
	VectorCopy( maxs, box_model.maxs );

	if ( capsule )
	{
		return CAPSULE_MODEL_HANDLE;
	}

	box_planes[ 0 ].dist = maxs[ 0 ];
	box_planes[ 1 ].dist = -maxs[ 0 ];
	box_planes[ 2 ].dist = mins[ 0 ];
	box_planes[ 3 ].dist = -mins[ 0 ];
	box_planes[ 4 ].dist = maxs[ 1 ];
	box_planes[ 5 ].dist = -maxs[ 1 ];
	box_planes[ 6 ].dist = mins[ 1 ];
	box_planes[ 7 ].dist = -mins[ 1 ];
	box_planes[ 8 ].dist = maxs[ 2 ];
	box_planes[ 9 ].dist = -maxs[ 2 ];
	box_planes[ 10 ].dist = mins[ 2 ];
	box_planes[ 11 ].dist = -mins[ 2 ];

	VectorCopy( mins, box_brush->bounds[ 0 ] );
	VectorCopy( maxs, box_brush->bounds[ 1 ] );

	return BOX_MODEL_HANDLE;
}

/*
===================
CM_ModelBounds
===================
*/
void CM_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs )
{
	cmodel_t *cmod;

	cmod = CM_ClipHandleToModel( model );
	VectorCopy( cmod->mins, mins );
	VectorCopy( cmod->maxs, maxs );
}
