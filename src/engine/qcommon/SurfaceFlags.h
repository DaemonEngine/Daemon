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

// this file is used by both engine and game code
// see engine/qcommon/q_shared.h

// those values may be used by q3map2 map compiler

// game may override some of them
// see Unvanquished's sgame/CustomSurfaceFlags.h

// content flags and surface flags are stored on separate bitfield,
// multiple bits can be set a the same time

// historically this file defines things the Wolf:ET way
// because Daemon derivates from ET:XreaL

// surface flags
#define CONTENTS_SOLID              BIT( 0 )  // 0x1
// undefined                        BIT( 1 )  // 0x2
#define CONTENTS_LIGHTGRID          BIT( 2 )  // 0x4
#define CONTENTS_LAVA               BIT( 3 )  // 0x8
#define CONTENTS_SLIME              BIT( 4 )  // 0x10
#define CONTENTS_WATER              BIT( 5 )  // 0x20
#define CONTENTS_FOG                BIT( 6 )  // 0x40
#define CONTENTS_MISSILECLIP        BIT( 7 )  // 0x80
#define CONTENTS_ITEM               BIT( 8 )  // 0x100
// undefined                        BIT( 9 )  // 0x200
// undefined                        BIT( 10 ) // 0x400
// undefined                        BIT( 11 ) // 0x800
// undefined                        BIT( 12 ) // 0x1000     // Unvanquished stores CONTENTS_NOALIENBUILD there
// undefined                        BIT( 13 ) // 0x2000     // Unvanquished stores CONTENTS_NOHUMANBUILD there
#define CONTENTS_MOVER              BIT( 14 ) // 0x4000     // Unvanquished stores CONTENTS_NOBUILD there
#define CONTENTS_AREAPORTAL         BIT( 15 ) // 0x8000
#define CONTENTS_PLAYERCLIP         BIT( 16 ) // 0x10000
#define CONTENTS_MONSTERCLIP        BIT( 17 ) // 0x20000
#define CONTENTS_TELEPORTER         BIT( 18 ) // 0x40000
#define CONTENTS_JUMPPAD            BIT( 19 ) // 0x80000
#define CONTENTS_CLUSTERPORTAL      BIT( 20 ) // 0x100000
#define CONTENTS_DONOTENTER         BIT( 21 ) // 0x200000
#define CONTENTS_DONOTENTER_LARGE   BIT( 22 ) // 0x400000
// undefined                        BIT( 23 ) // 0x800000
#define CONTENTS_ORIGIN             BIT( 24 ) // 0x1000000  // removed from entity before BSP computation
#define CONTENTS_BODY               BIT( 25 ) // 0x2000000  // not used by brushes (must not), used by game entities
#define CONTENTS_CORPSE             BIT( 26 ) // 0x4000000
#define CONTENTS_DETAIL             BIT( 27 ) // 0x8000000  // non-solid brushes excluded from structural BSP
#define CONTENTS_STRUCTURAL         BIT( 28 ) // 0x10000000 // solid brushes from BSP
#define CONTENTS_TRANSLUCENT        BIT( 29 ) // 0x20000000 // contained surfaces will not be consumed
#define CONTENTS_TRIGGER            BIT( 30 ) // 0x40000000
#define CONTENTS_NODROP             BIT( 31 ) // 0x80000000 // delete bodies or items whean dropped, used on things like lava or pit of death to prevent unnecessary polygon pileups

// content flags
#define SURF_NODAMAGE               BIT( 0 )  // 0x1        // falling on this surface does not give damage
#define SURF_SLICK                  BIT( 1 )  // 0x2        // reduce friction
#define SURF_SKY                    BIT( 2 )  // 0x4        // rendered as sky
#define SURF_LADDER                 BIT( 3 )  // 0x8        // player can climb this surface
#define SURF_NOIMPACT               BIT( 4 )  // 0x10       // missile does not explode but disappear, useful for skyboxes
#define SURF_NOMARKS                BIT( 5 )  // 0x20       // missile will explode, but no mark will be left
#define SURF_SPLASH                 BIT( 6 )  // 0x40       // Q3 stores SURF_FLESH there; RTCW stores SURF_CERAMIC there; Wolf:ET stores SURF_SPLASH there
#define SURF_NODRAW                 BIT( 7 )  // 0x80       // renderer will not draw this surface, q3map2 will not paint lightmap either
#define SURF_HINT                   BIT( 7 )  // 0x100      // q3map2 will split BSP using this surface
#define SURF_SKIP                   BIT( 9 )  // 0x200      // ignore this surface, non-closed brushes can be made this way
#define SURF_NOLIGHTMAP             BIT( 10 ) // 0x400      // lightmap will not be painted on such surface
#define SURF_POINTLIGHT             BIT( 11 ) // 0x800      // sample lighting at vertexes
#define SURF_METAL                  BIT( 12 ) // 0x1000     // play clanking footsteps
#define SURF_NOSTEPS                BIT( 13 ) // 0x2000     // do not play footstep sound at all
#define SURF_NONSOLID               BIT( 14 ) // 0x4000     // clears the solid flag, don't collide against curves
#define SURF_LIGHTFILTER            BIT( 15 ) // 0x8000     // used by q3map2 light stage to cast light through this surface color (stained glass)
#define SURF_ALPHASHADOW            BIT( 16 ) // 0x10000    // used by q3map2 light stage to cast shadows behind this surface
#define SURF_NODLIGHT               BIT( 17 ) // 0x20000    // don't paint dynamic light on this surface even if solid (sky, solid lava…)
#define SURF_WOOD                   BIT( 18 ) // 0x40000    // Q3 stores SURF_DUST there (leave a dust trail when walking on); RTCW and Wolf:ET stores SURF_WOOD there
#define SURF_GRASS                  BIT( 19 ) // 0x80000    // RTCW/Wolf:ET; Unvanquished stores SURF_NOALIENBUILD there; Q3 defined no surface flags after BIT(18); QuakeLive stores SURF_SNOW there
#define SURF_GRAVEL                 BIT( 20 ) // 0x100000   // RTCW/Wolf:ET; Unvanquished stores SURF_NOHUMANBUILD there; QuakeLive stores SURF_WOOD there
#define SURF_GLASS                  BIT( 21 ) // 0x200000   // RTCW/Wolf:ET; Unvanquished stores SURF_NOBUILD there; an unknown game stored SURF_SMGROUP there before RTCW
#define SURF_SNOW                   BIT( 22 ) // 0x400000   // RTCW/Wolf:ET
#define SURF_ROOF                   BIT( 23 ) // 0x800000   // RTCW/Wolf:ET
#define SURF_RUBBLE                 BIT( 24 ) // 0x1000000  // RTCW/Wolf:ET
#define SURF_CARPET                 BIT( 25 ) // 0x2000000  // RTCW/Wolf:ET
#define SURF_MONSTERSLICK           BIT( 26 ) // 0x4000000  // reduced friction surface that only affects bots
#define SURF_MONSLICK_W             BIT( 27 ) // 0x8000000  // west monsterslick
#define SURF_MONSLICK_N             BIT( 28 ) // 0x10000000 // north monsterslick
#define SURF_MONSLICK_E             BIT( 29 ) // 0x20000000 // east monsterslick
#define SURF_MONSLICK_S             BIT( 30 ) // 0x40000000 // south monsterslick
#define SURF_LANDMINE               BIT( 31 ) // 0x80000000 // Wolf:ET, landmines can be placed on this surface

// Note about other idTech 3 based games:
// Jedi Knights games (see OpenJK) also define a third bitfield for flags with MATERIAL prefix to tell surface is water, snow, sand, glass, sort grasss, long grass, etc.
// Jedi Knights games also redefine a lot of CONTENTS flags (introducing things like CONTENTS_LADDER) and SURF FLAGS (moving SURF_SKY to BIT(13) for example)
// Smokin'Guns uses a special .tex sidecar files to tweak surfaces flags
