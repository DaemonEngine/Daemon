# common to both daemon and unvanquished
daemon_daemonlib_common_srcs = files(
  'common/Assert.h',
  'common/Color.h',
  'common/Color.cpp',
  'common/Command.cpp',
  'common/Command.h',
  'common/Common.h',
  'common/Compiler.h',
  'common/Cvar.cpp',
  'common/Cvar.h',
  'common/Debugger.cpp',
  'common/Defs.h',
  'common/DisjointSets.h',
  'common/Endian.h',
  'common/FileSystem.cpp',
  'common/FileSystem.h',
  'common/IPC/Channel.h',
  'common/IPC/CommandBuffer.cpp',
  'common/IPC/CommandBuffer.h',
  'common/IPC/Common.h',
  'common/IPC/CommonSyscalls.h',
  'common/IPC/Primitives.cpp',
  'common/IPC/Primitives.h',
  'common/KeyIdentification.cpp',
  'common/KeyIdentification.h',
  'common/LineEditData.cpp',
  'common/LineEditData.h',
  'common/Log.cpp',
  'common/Log.h',
  'common/Math.h',
  'common/Optional.h',
  'common/Platform.h',
  'common/Serialize.h',
  'common/String.cpp',
  'common/String.h',
  'common/System.cpp',
  'common/System.h',
  'common/Util.h',
  'common/cm/cm_load.cpp',
  'common/cm/cm_local.h',
  'common/cm/cm_patch.cpp',
  'common/cm/cm_patch.h',
  'common/cm/cm_plane.cpp',
  'common/cm/cm_polylib.cpp',
  'common/cm/cm_polylib.h',
  'common/cm/cm_public.h',
  'common/cm/cm_test.cpp',
  'common/cm/cm_trace.cpp',
  'common/cm/cm_trisoup.cpp',
  'common/math/Vector.h',
  'engine/qcommon/q_math.cpp',
  'engine/qcommon/q_shared.cpp',
  'engine/qcommon/q_shared.h',
  'engine/qcommon/q_unicode.cpp',
  'engine/qcommon/q_unicode.h',
  'engine/qcommon/unicode_data.h',
)

# things common to all daemon builds, whether the client, server or ttyclient
daemon_common_srcs = files(
  'engine/qcommon/cmd.cpp',
  'engine/qcommon/common.cpp',
  'engine/qcommon/crypto.cpp',
  'engine/qcommon/crypto.h',
  'engine/qcommon/cvar.cpp',
  'engine/qcommon/cvar.h',
  'engine/qcommon/files.cpp',
  'engine/qcommon/huffman.cpp',
  'engine/qcommon/msg.cpp',
  'engine/qcommon/net_chan.cpp',
  'engine/qcommon/net_ip.cpp',
  'engine/qcommon/net_types.h',
  'engine/qcommon/print_translated.h',
  'engine/qcommon/qcommon.h',
  'engine/qcommon/qfiles.h',
  'engine/qcommon/SurfaceFlags.h',
  'engine/qcommon/sys.h',
  'engine/qcommon/translation.cpp',
) + daemon_daemonlib_common_srcs

# TODO: this doesn't seem to be different from daemon_common_srcs?
daemon_engine_srcs = files(
  'engine/framework/Application.cpp',
  'engine/framework/Application.h',
  'engine/framework/BaseCommands.cpp',
  'engine/framework/BaseCommands.h',
  'engine/framework/CommandBufferHost.cpp',
  'engine/framework/CommandBufferHost.h',
  'engine/framework/CommandSystem.cpp',
  'engine/framework/CommandSystem.h',
  'engine/framework/CommonVMServices.cpp',
  'engine/framework/CommonVMServices.h',
  'engine/framework/ConsoleField.cpp',
  'engine/framework/ConsoleField.h',
  'engine/framework/ConsoleHistory.cpp',
  'engine/framework/ConsoleHistory.h',
  'engine/framework/CrashDump.h',
  'engine/framework/CrashDump.cpp',
  'engine/framework/CvarSystem.cpp',
  'engine/framework/CvarSystem.h',
  'engine/framework/LogSystem.cpp',
  'engine/framework/LogSystem.h',
  'engine/framework/Resource.cpp',
  'engine/framework/Resource.h',
  'engine/framework/System.cpp',
  'engine/framework/System.h',
  'engine/framework/VirtualMachine.cpp',
  'engine/framework/VirtualMachine.h',
  'engine/framework/Crypto.cpp',
  'engine/framework/Crypto.h',
  'engine/framework/Rcon.cpp',
  'engine/framework/Rcon.h',
  'engine/framework/Network.h',
  'engine/framework/Network.cpp',
  'engine/qcommon/md5.cpp',
  'engine/sys/con_common.h',
  'engine/sys/con_common.cpp',
  'engine/sys/sys_events.h',
)

assert(host_machine.system() == 'linux' or host_machine.system() == 'pnacl')
#host_machine.cpu
#if (WIN32)
#    set(ENGINELIST ${ENGINELIST}
#        ${ENGINE_DIR}/sys/con_passive.cpp
#    )
#else()
#    set(ENGINELIST ${ENGINELIST}
#        ${ENGINE_DIR}/sys/con_tty.cpp
daemon_engine_srcs += files('engine/sys/con_tty.cpp')
#    )
#endif()
#
#if (USE_CURSES)
#    set(ENGINELIST ${ENGINELIST}
#        ${ENGINE_DIR}/sys/con_curses.cpp
#    )
#endif()

daemon_renderer_srcs = files(
  'engine/renderer/gl_shader.cpp',
  'engine/renderer/gl_shader.h',
  'engine/renderer/iqm.h',
  'engine/renderer/shaders.cpp',
  'engine/renderer/tr_animation.cpp',
  'engine/renderer/tr_backend.cpp',
  'engine/renderer/tr_bsp.cpp',
  'engine/renderer/tr_cmds.cpp',
  'engine/renderer/tr_curve.cpp',
  'engine/renderer/tr_decals.cpp',
  'engine/renderer/tr_fbo.cpp',
  'engine/renderer/tr_flares.cpp',
  'engine/renderer/tr_font.cpp',
  'engine/renderer/InternalImage.cpp',
  'engine/renderer/InternalImage.h',
  'engine/renderer/tr_image.cpp',
  'engine/renderer/tr_image.h',
  'engine/renderer/tr_image_crn.cpp',
  'engine/renderer/tr_image_dds.cpp',
  'engine/renderer/tr_image_jpg.cpp',
  'engine/renderer/tr_image_ktx.cpp',
  'engine/renderer/tr_image_png.cpp',
  'engine/renderer/tr_image_tga.cpp',
  'engine/renderer/tr_image_webp.cpp',
  'engine/renderer/tr_init.cpp',
  'engine/renderer/tr_light.cpp',
  'engine/renderer/tr_local.h',
  'engine/renderer/tr_main.cpp',
  'engine/renderer/tr_marks.cpp',
  'engine/renderer/tr_mesh.cpp',
  'engine/renderer/tr_model.cpp',
  'engine/renderer/tr_model_iqm.cpp',
  'engine/renderer/tr_model_md3.cpp',
  'engine/renderer/tr_model_md5.cpp',
  'engine/renderer/tr_model_skel.cpp',
  'engine/renderer/tr_model_skel.h',
  'engine/renderer/tr_noise.cpp',
  'engine/renderer/tr_public.h',
  'engine/renderer/tr_scene.cpp',
  'engine/renderer/tr_shade.cpp',
  'engine/renderer/tr_shader.cpp',
  'engine/renderer/tr_shade_calc.cpp',
  'engine/renderer/tr_skin.cpp',
  'engine/renderer/tr_sky.cpp',
  'engine/renderer/tr_surface.cpp',
  'engine/renderer/tr_types.h',
  'engine/renderer/tr_vbo.cpp',
  'engine/renderer/tr_world.cpp',
  'engine/sys/sdl_glimp.cpp',
  'engine/sys/sdl_icon.h',
)

#set(GLSLSOURCELIST
#    ${ENGINE_DIR}/renderer/glsl_source/skybox_vp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/ssao_fp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/ssao_vp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/vertexAnimation_vp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/vertexSimple_vp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/vertexSkinning_vp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/vertexSprite_vp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/blurX_fp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/blurX_vp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/blurY_fp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/blurY_vp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/cameraEffects_fp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/cameraEffects_vp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/computeLight_fp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/contrast_fp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/contrast_vp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/debugShadowMap_fp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/debugShadowMap_vp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/deformVertexes_vp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/depthtile1_fp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/depthtile1_vp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/depthtile2_fp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/depthtile2_vp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/dispersion_C_fp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/dispersion_C_vp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/fogGlobal_fp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/fogGlobal_vp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/fogQuake3_fp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/fogQuake3_vp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/forwardLighting_fp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/forwardLighting_vp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/fxaa_fp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/fxaa_vp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/fxaa3_11_fp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/generic_fp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/generic_vp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/heatHaze_fp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/heatHaze_vp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/lightMapping_fp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/lightMapping_vp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/lighttile_fp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/lighttile_vp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/liquid_fp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/liquid_vp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/motionblur_fp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/motionblur_vp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/portal_fp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/portal_vp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/reflection_CB_fp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/reflection_CB_vp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/refraction_C_fp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/refraction_C_vp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/reliefMapping_fp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/screen_fp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/screen_vp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/shadowFill_fp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/shadowFill_vp.glsl
#    ${ENGINE_DIR}/renderer/glsl_source/skybox_fp.glsl
#)

# The files that are used by all kind of daemon that can be a server
# FIXME: (which is actually all kind of daemon, graphical, tty and dedicated server, so maybe it could be simplified out)
# 
daemon_server_shared_srcs = files(
  'engine/server/server.h',
  'engine/server/sg_api.h',
  'engine/server/sg_msgdef.h',
  'engine/server/sv_bot.cpp',
  'engine/server/sv_ccmds.cpp',
  'engine/server/sv_client.cpp',
  'engine/server/sv_init.cpp',
  'engine/server/sv_main.cpp',
  'engine/server/sv_net_chan.cpp',
  'engine/server/sv_sgame.cpp',
  'engine/server/sv_snapshot.cpp',
  'engine/server/CryptoChallenge.cpp',
  'engine/server/CryptoChallenge.h',
) + daemon_common_srcs + daemon_engine_srcs

# The files that are used by all kind of daemon builds that can be a client
daemon_client_shared_srcs = files(
  'engine/client/cg_api.h',
  'engine/client/cg_msgdef.h',
  'engine/client/client.h',
  'engine/client/cl_avi.cpp',
  'engine/client/cl_cgame.cpp',
  'engine/client/cl_console.cpp',
  'engine/client/cl_download.cpp',
  'engine/client/cl_input.cpp',
  'engine/client/cl_main.cpp',
  'engine/client/cl_parse.cpp',
  'engine/client/cl_scrn.cpp',
  'engine/client/dl_main.cpp',
  'engine/client/hunk_allocator.cpp',
  'engine/client/key_identification.h',
  'engine/client/keycodes.h',
  'engine/client/keys.h',
  'engine/client/ClientApplication.cpp',
) + daemon_common_srcs

# The final list of sources used by the gui client
daemon_graphical_client_srcs = files(
  'engine/audio/ALObjects.cpp',
  'engine/audio/ALObjects.h',
  'engine/audio/Audio.cpp',
  'engine/audio/Audio.h',
  'engine/audio/AudioData.h',
  'engine/audio/AudioPrivate.h',
  'engine/audio/Emitter.cpp',
  'engine/audio/Emitter.h',
  'engine/audio/OggCodec.cpp',
  'engine/audio/OpusCodec.cpp',
  'engine/audio/Sample.cpp',
  'engine/audio/Sample.h',
  'engine/audio/Sound.cpp',
  'engine/audio/Sound.h',
  'engine/audio/SoundCodec.cpp',
  'engine/audio/SoundCodec.h',
  'engine/audio/WavCodec.cpp',
  'engine/botlib/bot_debug.h',
  'engine/client/cl_keys.cpp',
  'engine/client/key_binding.cpp',
  'engine/client/key_identification.cpp',
  'engine/sys/sdl_input.cpp',
) + daemon_client_shared_srcs + daemon_renderer_srcs + daemon_server_shared_srcs

# The final list of sources used by the tty client
daemon_tty_client_srcs = files(
  'engine/null/NullAudio.cpp',
  'engine/null/NullKeyboard.cpp',
  'engine/null/null_input.cpp',
  'engine/null/null_renderer.cpp',
) + daemon_client_shared_srcs + daemon_server_shared_srcs

# The final list of sources used by the dedicated server
daemon_dedicated_server_srcs = files(
  'engine/null/NullKeyboard.cpp',
  'engine/null/null_client.cpp',
  'engine/null/null_input.cpp',
  'engine/server/ServerApplication.cpp',
) + daemon_server_shared_srcs

# TODO: figure out what this is
#WIN_RC = files('engine/sys/daemon.rc')

# TODO: check what about crash_server_*.cpp

# The list of files used by both the client and server gamelogic
daemonlib_sharedgame_srcs = files(
  'shared/CommandBufferClient.cpp',
  'shared/CommandBufferClient.h',
  'shared/CommonProxies.cpp',
  'shared/CommonProxies.h',
  'shared/VMMain.cpp',
  'shared/VMMain.h',
) + daemon_daemonlib_common_srcs #+ daemon_engine_srcs

# Additional list of files used by client gamelogic only
daemonlib_clientgame_srcs = files(
  'shared/client/cg_api.cpp',
  'shared/client/cg_api.h',
)

# Additional list of files used by server gamelogic only
daemonlib_servergame_srcs = files(
  'shared/server/sg_api.cpp',
  'shared/server/sg_api.h',
)

daemonlib_sharedgame = static_library('daemonlib_sharedgame',
  daemonlib_sharedgame_srcs,
  cpp_args: ['-DBUILD_VM'] + nacl_args,
  build_by_default: false, # build only if needed, typically only if building the gamelogic
  link_with: [nacl_native_lib],
  include_directories: daemon_gamelogic_incs,
)

daemonlib_sharedgame_for_dll = static_library('daemonlib_sharedgame_for_dll',
  daemonlib_sharedgame_srcs,
  cpp_args: ['-DBUILD_VM', '-DBUILD_VM_IN_PROCESS'] + nacl_args,
  build_by_default: false, # build only if needed, typically only if building the gamelogic
  link_with: [nacl_native_lib],
  include_directories: daemon_gamelogic_incs,
)

daemonlib_clientgame = static_library('daemonlib_clientgame',
  daemonlib_clientgame_srcs,
  cpp_args: ['-DBUILD_VM', '-DBUILD_CGAME'] + nacl_args,
  build_by_default: false, # build only if needed, typically only if building the gamelogic
  link_with: [nacl_native_lib],
  include_directories: daemon_gamelogic_incs,
)

daemonlib_servergame = static_library('daemonlib_servergame',
  daemonlib_servergame_srcs,
  cpp_args: ['-DBUILD_VM', '-DBUILD_SGAME'] + nacl_args,
  build_by_default: false, # build only if needed, typically only if building the gamelogic
  link_with: [nacl_native_lib],
  include_directories: daemon_gamelogic_incs,
)
