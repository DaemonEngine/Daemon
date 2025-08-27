/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2024, Daemon Developers
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

#include "common/Common.h"
#include "engine/client/cg_msgdef.h"

#include "shared/VMMain.h"
#include "shared/CommandBufferClient.h"
#include "shared/CommonProxies.h"
#include "shared/client/cg_api.h"

static bool mapLoaded;
static vec3_t viewAngles;
static refdef_t refdef;

// Unvanquished default
static Cvar::Cvar<float> dg_fovY("dg_fovY", "vertical field of view (degrees)", Cvar::NONE, 73.739792);
// Unvanquished default at 4:3 aspect ratio
static Cvar::Cvar<float> dg_fovX("dg_fovX", "horizontal field of view (degrees)", Cvar::NONE, 90);

static Cvar::Cvar<int> dg_timeInitial(
	"dg_timeInitial", "time to use for first frame of rendering", Cvar::NONE, 0);

// There is no server time, and the client doesn't tell us what time it has
static void UpdateTime()
{
	static int lastTime;
	if (refdef.time == 0)
		refdef.time = dg_timeInitial.Get();
	int time = Sys::Milliseconds();
	int fixedFrame;
	if (Str::ParseInt(fixedFrame, Cvar::GetValue("common.fixedFrameTime")) && fixedFrame > 0) {
		refdef.time += fixedFrame;
	} else {
		float timescale = 1;
		Str::ToFloat(Cvar::GetValue("timescale"), timescale);
		refdef.time += std::max<int>(1, timescale * (time - lastTime));
	}
	lastTime = time;
}

void VM::VMHandleSyscall(uint32_t id, Util::Reader reader) {
	int major = id >> 16;
	int minor = id & 0xffff;
	if (major == VM::QVM) {
		switch (minor) {
		case CG_STATIC_INIT:
			IPC::HandleMsg<CGameStaticInitMsg>(VM::rootChannel, std::move(reader), [] (int milliseconds) {
				VM::InitializeProxies(milliseconds);
				FS::Initialize();
				cmdBuffer.Init();
			});
			break;

		case CG_ROCKET_VM_INIT:
			IPC::HandleMsg<CGameRocketInitMsg>(VM::rootChannel, std::move(reader), [] (const WindowConfig& gl) {
				refdef.width = gl.vidWidth;
				refdef.height = gl.vidHeight;
			});
			break;

		case CG_ROCKET_FRAME:
			IPC::HandleMsg<CGameRocketFrameMsg>(VM::rootChannel, std::move(reader), [] (const cgClientState_t&) {
				if (mapLoaded) {
					refdef.fov_x = dg_fovX.Get();
					refdef.fov_y = dg_fovY.Get();
					AnglesToAxis(viewAngles, refdef.viewaxis);
					UpdateTime();
					trap_R_RenderScene(&refdef);
				} else {
					trap_R_SetColor(Color::MdGrey);
					static qhandle_t h = trap_R_RegisterShader("white", RSF_DEFAULT);
					trap_R_DrawStretchPic(0, 0, refdef.width, refdef.height, 0, 0, 1, 1, h);
				}

				cmdBuffer.TryFlush();
			});
			break;

		case CG_KEY_DOWN_EVENT: // only engine->cgame call with a reply
			IPC::HandleMsg<CGameKeyDownEventMsg>(VM::rootChannel, std::move(reader), [] (Keyboard::Key, bool, bool& consumed) {
				consumed = false;
			});
			break;

		default:
			{
				Util::Writer writer;
				writer.Write<uint32_t>(IPC::ID_RETURN);
				VM::rootChannel.SendMsg(writer);
			}
			break;
		}
	} else if (major < VM::LAST_COMMON_SYSCALL) {
		VM::HandleCommonSyscall(major, minor, std::move(reader), VM::rootChannel);
	} else {
		Sys::Drop("unhandled VM major syscall number %i", major);
	}
}

class LoadMapCmd : public Cmd::StaticCmd
{
public:
	LoadMapCmd() : StaticCmd("loadmap", "load a map for viewing") {}

	void Run(const Cmd::Args& args) const override
	{
		if (args.Argc() == 2) {
			const FS::PakInfo* pak = FS::FindPak("map-" + args.Argv(1));
			if (!pak) {
				Print("map %s not found", args.Argv(1));
				return;
			}

			if (mapLoaded) {
				// need to restart FS
				trap_SendConsoleCommand(Str::Format(
					"disconnect\nloadmap %s", Cmd::Escape(args.Argv(1))).c_str());
			} else {
				FS::PakPath::LoadPak(*pak);
				// need to restart renderer because shader scanning was done earlier, but
				// WITHOUT restarting the filesystem
				trap_SendConsoleCommand(Str::Format(
					"vid_restart\nloadmap -restarted %s", Cmd::Escape(args.Argv(1))).c_str());
			}
		} else if (args.Argc() == 3 && Str::IsIEqual(args.Argv(1), "-restarted")) {
			// All layers of the color grade image are pre-populated with a neutral cgrade when
			// the renderer starts (though there is no way to get it back later)
			refdef.gradingWeights[0] = 1.0f;

			trap_R_LoadWorldMap(("maps/" + args.Argv(2) + ".bsp").c_str());

			// This is done by EndRegistration but there is no way to invoke that
			bool reflection;
			if (Cvar::ParseCvarValue(Cvar::GetValue("r_reflectionMapping"), reflection)
			    && reflection) {
				trap_SendConsoleCommand("buildcubemaps");
			}

			mapLoaded = true;
		} else {
			PrintUsage(args, "<map name>");
		}
	}
};
static LoadMapCmd loadmapRegistration;

class SetViewPosCmd : public Cmd::StaticCmd
{
public:
	SetViewPosCmd() : StaticCmd("setviewpos", "set viewing position & angle") {}

	void Run(const Cmd::Args& args) const override
	{
		if (args.Argc() < 2 || args.Argc() > 7) {
			PrintUsage(args, "x y z yaw pitch roll",
				"All arguments are optional. '-' can be used instead of a number to not set one.");
			return;
		}

		// Although PITCH is 0 and YAW is 1, we use the order yaw,pitch,roll which matches
		// Unvanquished, and is also the order the rotations are actually applied in
		float* values[6] { refdef.vieworg + 0, refdef.vieworg + 1, refdef.vieworg + 2,
		                   viewAngles + YAW, viewAngles + PITCH, viewAngles + ROLL };

		for (int i = 0; i < args.Argc() - 1; i++) {
			if (args.Argv(i + 1) != "-" && !Str::ToFloat(args.Argv(i + 1), *values[i])) {
				Print("invalid number '%s'", args.Argv(i + 1));
			}
		}
	}
};
static SetViewPosCmd setviewposRegistration;

bool ConsoleCommand()
{
	ASSERT_UNREACHABLE();
}
void CompleteCommand(int)
{
	ASSERT_UNREACHABLE();
}

void VM::GetNetcodeTables(NetcodeTable&, int&)
{
	Sys::Drop("GetNetcodeTables not implemented");
}
