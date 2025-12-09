if (USE_MUMBLE)
    add_definitions("-DUSE_MUMBLE")
endif()

set(COMMONLIST
    ${COMMON_DIR}/Assert.h
    ${COMMON_DIR}/Color.h
    ${COMMON_DIR}/Color.cpp
    ${COMMON_DIR}/Command.cpp
    ${COMMON_DIR}/Command.h
    ${COMMON_DIR}/Common.h
    ${COMMON_DIR}/Compiler.h
    ${COMMON_DIR}/Cvar.cpp
    ${COMMON_DIR}/Cvar.h
    ${COMMON_DIR}/Debugger.cpp
    ${COMMON_DIR}/Defs.h
    ${COMMON_DIR}/DisjointSets.h
    ${COMMON_DIR}/Endian.h
    ${COMMON_DIR}/FileSystem.cpp
    ${COMMON_DIR}/FileSystem.h
    ${COMMON_DIR}/IPC/Channel.h
    ${COMMON_DIR}/IPC/CommandBuffer.cpp
    ${COMMON_DIR}/IPC/CommandBuffer.h
    ${COMMON_DIR}/IPC/Common.h
    ${COMMON_DIR}/IPC/CommonSyscalls.h
    ${COMMON_DIR}/IPC/Primitives.cpp
    ${COMMON_DIR}/IPC/Primitives.h
    ${COMMON_DIR}/KeyIdentification.cpp
    ${COMMON_DIR}/KeyIdentification.h
    ${COMMON_DIR}/CPPStandard.h
    ${COMMON_DIR}/LineEditData.cpp
    ${COMMON_DIR}/LineEditData.h
    ${COMMON_DIR}/Log.cpp
    ${COMMON_DIR}/Log.h
    ${COMMON_DIR}/Math.h
    ${COMMON_DIR}/Optional.h
    ${COMMON_DIR}/Platform.h
    ${COMMON_DIR}/Serialize.h
    ${COMMON_DIR}/StackTrace.h
    ${COMMON_DIR}/String.cpp
    ${COMMON_DIR}/String.h
    ${COMMON_DIR}/System.cpp
    ${COMMON_DIR}/System.h
    ${COMMON_DIR}/Type.h
    ${COMMON_DIR}/Util.cpp
    ${COMMON_DIR}/Util.h
    ${COMMON_DIR}/cm/cm_load.cpp
    ${COMMON_DIR}/cm/cm_local.h
    ${COMMON_DIR}/cm/cm_patch.cpp
    ${COMMON_DIR}/cm/cm_patch.h
    ${COMMON_DIR}/cm/cm_plane.cpp
    ${COMMON_DIR}/cm/cm_polylib.cpp
    ${COMMON_DIR}/cm/cm_polylib.h
    ${COMMON_DIR}/cm/cm_public.h
    ${COMMON_DIR}/cm/cm_test.cpp
    ${COMMON_DIR}/cm/cm_trace.cpp
    ${COMMON_DIR}/cm/cm_trisoup.cpp
    ${COMMON_DIR}/math/Vector.h
    ${ENGINE_DIR}/qcommon/q_math.cpp
    ${ENGINE_DIR}/qcommon/q_shared.cpp
    ${ENGINE_DIR}/qcommon/q_shared.h
    ${ENGINE_DIR}/qcommon/q_unicode.cpp
    ${ENGINE_DIR}/qcommon/q_unicode.h
    ${ENGINE_DIR}/qcommon/unicode_data.h
)
if (DAEMON_PARENT_SCOPE_DIR)
    set(COMMONLIST ${COMMONLIST} PARENT_SCOPE)
endif()

# Tests for code shared by engine and gamelogic
set(COMMONTESTLIST
    ${LIB_DIR}/tinyformat/TinyformatTest.cpp
    ${COMMON_DIR}/ColorTest.cpp
    ${COMMON_DIR}/CvarTest.cpp
    ${COMMON_DIR}/FileSystemTest.cpp
    ${COMMON_DIR}/StringTest.cpp
    ${COMMON_DIR}/cm/unittest.cpp
    ${COMMON_DIR}/MathTest.cpp
    ${COMMON_DIR}/UtilTest.cpp
    ${ENGINE_DIR}/qcommon/q_math_test.cpp
)

if (USE_VULKAN)
    include (${ENGINE_DIR}/renderer-vulkan/src.cmake)
else()
    include (${ENGINE_DIR}/renderer/src.cmake)
endif()

set(GLSL_EMBED_DIR "${ENGINE_DIR}/renderer/glsl_source")
set(GLSL_EMBED_LIST
    # Common shader libraries
    common.glsl
    common_cp.glsl
    fogEquation_fp.glsl
    shaderProfiler_vp.glsl
    shaderProfiler_fp.glsl
    # Material system shaders
    material_cp.glsl
    material_vp.glsl
    material_fp.glsl
    clearSurfaces_cp.glsl
    cull_cp.glsl
    depthReduction_cp.glsl
    processSurfaces_cp.glsl
    
    # Screen-space shaders
    screenSpace_vp.glsl
    blur_fp.glsl
    cameraEffects_fp.glsl
    contrast_fp.glsl
    fogGlobal_fp.glsl
    fxaa_fp.glsl
    fxaa3_11_fp.glsl
    motionblur_fp.glsl
    ssao_fp.glsl
    
    # Lighting shaders
    depthtile1_vp.glsl
    depthtile1_fp.glsl
    depthtile2_fp.glsl
    lighttile_vp.glsl
    lighttile_fp.glsl
    computeLight_fp.glsl
    reliefMapping_fp.glsl

    # Common vertex shader libraries
    deformVertexes_vp.glsl
    vertexAnimation_vp.glsl
    vertexSimple_vp.glsl
    vertexSkinning_vp.glsl

    # Regular shaders
    fogQuake3_vp.glsl
    fogQuake3_fp.glsl
    generic_vp.glsl
    generic_fp.glsl
    heatHaze_vp.glsl
    heatHaze_fp.glsl
    lightMapping_vp.glsl
    lightMapping_fp.glsl
    liquid_vp.glsl
    liquid_fp.glsl
    portal_vp.glsl
    portal_fp.glsl
    reflection_CB_vp.glsl
    reflection_CB_fp.glsl
    screen_vp.glsl
    screen_fp.glsl
    skybox_vp.glsl
    skybox_fp.glsl
)

set(SERVERLIST
    ${ENGINE_DIR}/server/server.h
    ${ENGINE_DIR}/server/sg_api.h
    ${ENGINE_DIR}/server/sg_msgdef.h
    ${ENGINE_DIR}/server/sv_bot.cpp
    ${ENGINE_DIR}/server/sv_ccmds.cpp
    ${ENGINE_DIR}/server/sv_client.cpp
    ${ENGINE_DIR}/server/sv_init.cpp
    ${ENGINE_DIR}/server/sv_main.cpp
    ${ENGINE_DIR}/server/sv_net_chan.cpp
    ${ENGINE_DIR}/server/sv_sgame.cpp
    ${ENGINE_DIR}/server/sv_snapshot.cpp
    ${ENGINE_DIR}/server/CryptoChallenge.cpp
    ${ENGINE_DIR}/server/CryptoChallenge.h
)

set(ENGINELIST
    ${ENGINE_DIR}/framework/Application.cpp
    ${ENGINE_DIR}/framework/Application.h
    ${ENGINE_DIR}/framework/ApplicationInternals.h
    ${ENGINE_DIR}/framework/BaseCommands.cpp
    ${ENGINE_DIR}/framework/BaseCommands.h
    ${ENGINE_DIR}/framework/CommandBufferHost.cpp
    ${ENGINE_DIR}/framework/CommandBufferHost.h
    ${ENGINE_DIR}/framework/CommandSystem.cpp
    ${ENGINE_DIR}/framework/CommandSystem.h
    ${ENGINE_DIR}/framework/CommonVMServices.cpp
    ${ENGINE_DIR}/framework/CommonVMServices.h
    ${ENGINE_DIR}/framework/ConsoleField.cpp
    ${ENGINE_DIR}/framework/ConsoleField.h
    ${ENGINE_DIR}/framework/ConsoleHistory.cpp
    ${ENGINE_DIR}/framework/ConsoleHistory.h
    ${ENGINE_DIR}/framework/CrashDump.h
    ${ENGINE_DIR}/framework/CrashDump.cpp
    ${ENGINE_DIR}/framework/CvarSystem.cpp
    ${ENGINE_DIR}/framework/CvarSystem.h
    ${ENGINE_DIR}/framework/LogSystem.cpp
    ${ENGINE_DIR}/framework/LogSystem.h
    ${ENGINE_DIR}/framework/Resource.cpp
    ${ENGINE_DIR}/framework/Resource.h
    ${ENGINE_DIR}/framework/System.cpp
    ${ENGINE_DIR}/framework/System.h
    ${ENGINE_DIR}/framework/VirtualMachine.cpp
    ${ENGINE_DIR}/framework/VirtualMachine.h
    ${ENGINE_DIR}/framework/Crypto.cpp
    ${ENGINE_DIR}/framework/Crypto.h
    ${ENGINE_DIR}/framework/Rcon.cpp
    ${ENGINE_DIR}/framework/Rcon.h
    ${ENGINE_DIR}/framework/Network.h
    ${ENGINE_DIR}/framework/Network.cpp
    ${ENGINE_DIR}/qcommon/md5.cpp
    ${ENGINE_DIR}/sys/con_common.h
    ${ENGINE_DIR}/sys/con_common.cpp
    ${ENGINE_DIR}/sys/sys_events.h
    ${ENGINE_DIR}/RefAPI.h
)

if (WIN32)
    set(ENGINELIST ${ENGINELIST}
        ${ENGINE_DIR}/sys/con_passive.cpp
    )
else()
    set(ENGINELIST ${ENGINELIST}
        ${ENGINE_DIR}/sys/con_tty.cpp
    )
endif()

# Tests runnable for any engine variant
set(ENGINETESTLIST ${COMMONTESTLIST}
    ${ENGINE_DIR}/framework/CommandSystemTest.cpp
)

set(QCOMMONLIST
    ${ENGINE_DIR}/qcommon/cmd.cpp
    ${ENGINE_DIR}/qcommon/common.cpp
    ${ENGINE_DIR}/qcommon/crypto.cpp
    ${ENGINE_DIR}/qcommon/crypto.h
    ${ENGINE_DIR}/qcommon/cvar.cpp
    ${ENGINE_DIR}/qcommon/cvar.h
    ${ENGINE_DIR}/qcommon/files.cpp
    ${ENGINE_DIR}/qcommon/huffman.cpp
    ${ENGINE_DIR}/qcommon/msg.cpp
    ${ENGINE_DIR}/qcommon/net_chan.cpp
    ${ENGINE_DIR}/qcommon/net_ip.cpp
    ${ENGINE_DIR}/qcommon/net_types.h
    ${ENGINE_DIR}/qcommon/print_translated.h
    ${ENGINE_DIR}/qcommon/qcommon.h
    ${ENGINE_DIR}/qcommon/qfiles.h
    ${ENGINE_DIR}/qcommon/SurfaceFlags.h
    ${ENGINE_DIR}/qcommon/sys.h
    ${ENGINE_DIR}/qcommon/translation.cpp
)

if (USE_CURSES)
    set(ENGINELIST ${ENGINELIST}
        ${ENGINE_DIR}/sys/con_curses.cpp
    )
endif()

set(CLIENTBASELIST
    ${ENGINE_DIR}/client/cg_api.h
    ${ENGINE_DIR}/client/cg_msgdef.h
    ${ENGINE_DIR}/client/client.h
    ${ENGINE_DIR}/client/cl_avi.cpp
    ${ENGINE_DIR}/client/cl_cgame.cpp
    ${ENGINE_DIR}/client/cl_console.cpp
    ${ENGINE_DIR}/client/cl_download.cpp
    ${ENGINE_DIR}/client/cl_input.cpp
    ${ENGINE_DIR}/client/cl_main.cpp
    ${ENGINE_DIR}/client/cl_parse.cpp
    ${ENGINE_DIR}/client/cl_scrn.cpp
    ${ENGINE_DIR}/client/cl_serverlist.cpp
    ${ENGINE_DIR}/client/cl_serverstatus.cpp
    ${ENGINE_DIR}/client/dl_main.cpp
    ${ENGINE_DIR}/client/hunk_allocator.cpp
    ${ENGINE_DIR}/client/key_identification.h
    ${ENGINE_DIR}/client/keycodes.h
    ${ENGINE_DIR}/client/keys.h
)

set(CLIENTLIST
    ${ENGINE_DIR}/audio/ALObjects.cpp
    ${ENGINE_DIR}/audio/ALObjects.h
    ${ENGINE_DIR}/audio/Audio.cpp
    ${ENGINE_DIR}/audio/Audio.h
    ${ENGINE_DIR}/audio/AudioData.h
    ${ENGINE_DIR}/audio/AudioPrivate.h
    ${ENGINE_DIR}/audio/Emitter.cpp
    ${ENGINE_DIR}/audio/Emitter.h
    ${ENGINE_DIR}/audio/OggCodec.cpp
    ${ENGINE_DIR}/audio/OpusCodec.cpp
    ${ENGINE_DIR}/audio/Sample.cpp
    ${ENGINE_DIR}/audio/Sample.h
    ${ENGINE_DIR}/audio/Sound.cpp
    ${ENGINE_DIR}/audio/Sound.h
    ${ENGINE_DIR}/audio/SoundCodec.cpp
    ${ENGINE_DIR}/audio/SoundCodec.h
    ${ENGINE_DIR}/audio/WavCodec.cpp
    ${ENGINE_DIR}/botlib/bot_debug.h
    ${ENGINE_DIR}/client/cl_keys.cpp
    ${ENGINE_DIR}/client/key_binding.cpp
    ${ENGINE_DIR}/client/key_identification.cpp
    ${ENGINE_DIR}/sys/sdl_input.cpp
    ${RENDERERLIST}
)

if (APPLE)
    set(CLIENTLIST ${CLIENTLIST} ${ENGINE_DIR}/sys/DisableAccentMenu.m)
endif()

set(CLIENTTESTLIST ${ENGINETESTLIST}
)

set(TTYCLIENTLIST
    ${ENGINE_DIR}/null/NullAudio.cpp
    ${ENGINE_DIR}/null/NullKeyboard.cpp
    ${ENGINE_DIR}/null/null_input.cpp
    ${ENGINE_DIR}/null/null_renderer.cpp
)

set(DEDSERVERLIST
    ${ENGINE_DIR}/null/NullKeyboard.cpp
    ${ENGINE_DIR}/null/null_client.cpp
    ${ENGINE_DIR}/null/null_input.cpp
)

set(WIN_RC ${ENGINE_DIR}/sys/windows-resource/icon.rc)
