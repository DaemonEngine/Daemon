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
    ${COMMON_DIR}/LineEditData.cpp
    ${COMMON_DIR}/LineEditData.h
    ${COMMON_DIR}/Log.cpp
    ${COMMON_DIR}/Log.h
    ${COMMON_DIR}/Math.h
    ${COMMON_DIR}/Optional.h
    ${COMMON_DIR}/Platform.h
    ${COMMON_DIR}/Serialize.h
    ${COMMON_DIR}/String.cpp
    ${COMMON_DIR}/String.h
    ${COMMON_DIR}/System.cpp
    ${COMMON_DIR}/System.h
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

set(RENDERERLIST
    ${ENGINE_DIR}/renderer/DetectGLVendors.cpp
    ${ENGINE_DIR}/renderer/DetectGLVendors.h
    ${ENGINE_DIR}/renderer/gl_shader.cpp
    ${ENGINE_DIR}/renderer/gl_shader.h
    ${ENGINE_DIR}/renderer/iqm.h
    ${ENGINE_DIR}/renderer/ShadeCommon.h
    ${ENGINE_DIR}/renderer/shaders.cpp
    ${ENGINE_DIR}/renderer/tr_animation.cpp
    ${ENGINE_DIR}/renderer/tr_backend.cpp
    ${ENGINE_DIR}/renderer/tr_bsp.cpp
    ${ENGINE_DIR}/renderer/tr_cmds.cpp
    ${ENGINE_DIR}/renderer/tr_curve.cpp
    ${ENGINE_DIR}/renderer/tr_fbo.cpp
    ${ENGINE_DIR}/renderer/tr_font.cpp
    ${ENGINE_DIR}/renderer/GeometryCache.cpp
    ${ENGINE_DIR}/renderer/GeometryCache.h
    ${ENGINE_DIR}/renderer/GeometryOptimiser.cpp
    ${ENGINE_DIR}/renderer/GeometryOptimiser.h
    ${ENGINE_DIR}/renderer/InternalImage.cpp
    ${ENGINE_DIR}/renderer/InternalImage.h
    ${ENGINE_DIR}/renderer/Material.cpp
    ${ENGINE_DIR}/renderer/Material.h
    ${ENGINE_DIR}/renderer/TextureManager.cpp
    ${ENGINE_DIR}/renderer/TextureManager.h
    ${ENGINE_DIR}/renderer/tr_image.cpp
    ${ENGINE_DIR}/renderer/tr_image.h
    ${ENGINE_DIR}/renderer/tr_image_crn.cpp
    ${ENGINE_DIR}/renderer/tr_image_dds.cpp
    ${ENGINE_DIR}/renderer/tr_image_jpg.cpp
    ${ENGINE_DIR}/renderer/tr_image_ktx.cpp
    ${ENGINE_DIR}/renderer/tr_image_png.cpp
    ${ENGINE_DIR}/renderer/tr_image_tga.cpp
    ${ENGINE_DIR}/renderer/tr_image_webp.cpp
    ${ENGINE_DIR}/renderer/tr_init.cpp
    ${ENGINE_DIR}/renderer/tr_light.cpp
    ${ENGINE_DIR}/renderer/tr_local.h
    ${ENGINE_DIR}/renderer/tr_main.cpp
    ${ENGINE_DIR}/renderer/tr_marks.cpp
    ${ENGINE_DIR}/renderer/tr_mesh.cpp
    ${ENGINE_DIR}/renderer/tr_model.cpp
    ${ENGINE_DIR}/renderer/tr_model_iqm.cpp
    ${ENGINE_DIR}/renderer/tr_model_md3.cpp
    ${ENGINE_DIR}/renderer/tr_model_md5.cpp
    ${ENGINE_DIR}/renderer/tr_model_skel.cpp
    ${ENGINE_DIR}/renderer/tr_model_skel.h
    ${ENGINE_DIR}/renderer/tr_noise.cpp
    ${ENGINE_DIR}/renderer/tr_public.h
    ${ENGINE_DIR}/renderer/tr_scene.cpp
    ${ENGINE_DIR}/renderer/tr_shade.cpp
    ${ENGINE_DIR}/renderer/tr_shader.cpp
    ${ENGINE_DIR}/renderer/tr_shade_calc.cpp
    ${ENGINE_DIR}/renderer/tr_skin.cpp
    ${ENGINE_DIR}/renderer/tr_sky.cpp
    ${ENGINE_DIR}/renderer/tr_surface.cpp
    ${ENGINE_DIR}/renderer/tr_types.h
    ${ENGINE_DIR}/renderer/tr_vbo.cpp
    ${ENGINE_DIR}/renderer/tr_video.cpp
    ${ENGINE_DIR}/renderer/tr_world.cpp
    ${ENGINE_DIR}/sys/sdl_glimp.cpp
    ${ENGINE_DIR}/sys/sdl_icon.h
)

set(GLSLSOURCELIST
    # Common shader libraries
    ${ENGINE_DIR}/renderer/glsl_source/common.glsl
    ${ENGINE_DIR}/renderer/glsl_source/common_cp.glsl
    ${ENGINE_DIR}/renderer/glsl_source/shaderProfiler_vp.glsl
    ${ENGINE_DIR}/renderer/glsl_source/shaderProfiler_fp.glsl
    
    # Material system shaders
    ${ENGINE_DIR}/renderer/glsl_source/material_cp.glsl
    ${ENGINE_DIR}/renderer/glsl_source/material_vp.glsl
    ${ENGINE_DIR}/renderer/glsl_source/material_fp.glsl
    ${ENGINE_DIR}/renderer/glsl_source/clearSurfaces_cp.glsl
    ${ENGINE_DIR}/renderer/glsl_source/cull_cp.glsl
    ${ENGINE_DIR}/renderer/glsl_source/depthReduction_cp.glsl
    ${ENGINE_DIR}/renderer/glsl_source/processSurfaces_cp.glsl
    
    # Screen-space shaders
    ${ENGINE_DIR}/renderer/glsl_source/screenSpace_vp.glsl
    ${ENGINE_DIR}/renderer/glsl_source/blur_fp.glsl
    ${ENGINE_DIR}/renderer/glsl_source/cameraEffects_fp.glsl
    ${ENGINE_DIR}/renderer/glsl_source/contrast_fp.glsl
    ${ENGINE_DIR}/renderer/glsl_source/fogGlobal_fp.glsl
    ${ENGINE_DIR}/renderer/glsl_source/fxaa_fp.glsl
    ${ENGINE_DIR}/renderer/glsl_source/fxaa3_11_fp.glsl
    ${ENGINE_DIR}/renderer/glsl_source/motionblur_fp.glsl
    ${ENGINE_DIR}/renderer/glsl_source/ssao_fp.glsl
    
    # Lighting shaders
    ${ENGINE_DIR}/renderer/glsl_source/depthtile1_vp.glsl
    ${ENGINE_DIR}/renderer/glsl_source/depthtile1_fp.glsl
    ${ENGINE_DIR}/renderer/glsl_source/depthtile2_fp.glsl
    ${ENGINE_DIR}/renderer/glsl_source/lighttile_vp.glsl
    ${ENGINE_DIR}/renderer/glsl_source/lighttile_fp.glsl
    ${ENGINE_DIR}/renderer/glsl_source/computeLight_fp.glsl
    ${ENGINE_DIR}/renderer/glsl_source/reliefMapping_fp.glsl

    # Common vertex shader libraries
    ${ENGINE_DIR}/renderer/glsl_source/deformVertexes_vp.glsl
    ${ENGINE_DIR}/renderer/glsl_source/vertexAnimation_vp.glsl
    ${ENGINE_DIR}/renderer/glsl_source/vertexSimple_vp.glsl
    ${ENGINE_DIR}/renderer/glsl_source/vertexSkinning_vp.glsl

    # Regular shaders
    ${ENGINE_DIR}/renderer/glsl_source/debugShadowMap_vp.glsl
    ${ENGINE_DIR}/renderer/glsl_source/debugShadowMap_fp.glsl
    ${ENGINE_DIR}/renderer/glsl_source/fogQuake3_vp.glsl
    ${ENGINE_DIR}/renderer/glsl_source/fogQuake3_fp.glsl
    ${ENGINE_DIR}/renderer/glsl_source/forwardLighting_vp.glsl
    ${ENGINE_DIR}/renderer/glsl_source/forwardLighting_fp.glsl
    ${ENGINE_DIR}/renderer/glsl_source/generic_vp.glsl
    ${ENGINE_DIR}/renderer/glsl_source/generic_fp.glsl
    ${ENGINE_DIR}/renderer/glsl_source/heatHaze_vp.glsl
    ${ENGINE_DIR}/renderer/glsl_source/heatHaze_fp.glsl
    ${ENGINE_DIR}/renderer/glsl_source/lightMapping_vp.glsl
    ${ENGINE_DIR}/renderer/glsl_source/lightMapping_fp.glsl
    ${ENGINE_DIR}/renderer/glsl_source/liquid_vp.glsl
    ${ENGINE_DIR}/renderer/glsl_source/liquid_fp.glsl
    ${ENGINE_DIR}/renderer/glsl_source/portal_vp.glsl
    ${ENGINE_DIR}/renderer/glsl_source/portal_fp.glsl
    ${ENGINE_DIR}/renderer/glsl_source/reflection_CB_vp.glsl
    ${ENGINE_DIR}/renderer/glsl_source/reflection_CB_fp.glsl
    ${ENGINE_DIR}/renderer/glsl_source/screen_vp.glsl
    ${ENGINE_DIR}/renderer/glsl_source/screen_fp.glsl
    ${ENGINE_DIR}/renderer/glsl_source/shadowFill_vp.glsl
    ${ENGINE_DIR}/renderer/glsl_source/shadowFill_fp.glsl
    ${ENGINE_DIR}/renderer/glsl_source/skybox_vp.glsl
    ${ENGINE_DIR}/renderer/glsl_source/skybox_fp.glsl
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
