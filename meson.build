# TODO: check whatever cmake does with msvc "incremental linking"

project('daemon', ['c', 'cpp'],
  version: '0.53.0',
  license: 'GPL-3.0-or-later',
  meson_version: '>= 0.56',
  default_options: ['buildtype=debugoptimized', 'b_ndebug=if-release', 'cpp_std=c++14']
)

daemon_public_incs = [
  include_directories('libs'),
  include_directories('src'),
]
daemon_private_incs = [
  include_directories('src/engine'),
  include_directories('libs/nacl'),
  include_directories('libs/breakpad/src'),
  include_directories('/home/afontain/unv/Unvanquished/build-nixos/embed_data/'), # TODO handle shaders compilation
]

daemon_gamelogic_incs = [
  #include_directories('/home/afontain/unv/Unvanquished/src'),
  include_directories('../../src'), # FIXME: take the path from the gamelogic project
] + daemon_public_incs + daemon_private_incs

# FIXME: aaaaah openal nixpkg bug
add_project_link_arguments('-L/nix/store/iccwkh7yacv40zyqnj1mah73616mid5s-openal-soft-1.21.1/lib', '-lopenal', language: 'cpp')
add_project_arguments('-I/nix/store/iccwkh7yacv40zyqnj1mah73616mid5s-openal-soft-1.21.1/include', '-I/nix/store/iccwkh7yacv40zyqnj1mah73616mid5s-openal-soft-1.21.1/include/AL', language: 'cpp')

subdir('libs')
subdir('external_deps')
subdir('src')


###
# Daemon Engine binaries
###

if host_machine.system() != 'pnacl'

  # TODO: only require the dependencies for the binaries we build

  openal_dep     = dependency('openal')
  sdl_dep        = dependency('sdl2')
  vorbisfile_dep = dependency('vorbisfile')
  opusfile_dep   = dependency('opusfile')
  thread_dep     = dependency('threads')
  libcurl_dep    = dependency('libcurl')
  freetype_dep   = dependency('freetype2')
  libjpeg_dep    = dependency('libjpeg')
  libpng_dep     = dependency('libpng')
  webp_dep       = dependency('libwebp')
  opengl_dep     = dependency('opengl')
  glew_dep       = dependency('glew')
  hogweed_dep    = dependency('hogweed')
  nettle_dep     = dependency('nettle')
  # TODO: if not found, use bundled? CMake does that, is it useful?
  zlib_dep       = dependency('zlib')

  daemon_cpp = meson.get_compiler('cpp')
  dl_dep  = daemon_cpp.find_library('dl',  required: false)
  gmp_dep = daemon_cpp.find_library('gmp', required: false)
  rt_dep  = daemon_cpp.find_library('rt',  required: false)

  # Deps for each binary
  deps_dedicated = [
    thread_dep, dl_dep, gmp_dep, rt_dep, hogweed_dep, nettle_dep, zlib_dep
  ]
  deps_tty = deps_dedicated + [libcurl_dep]
  deps_graphical = deps_tty + [
    openal_dep, sdl_dep, vorbisfile_dep, opusfile_dep, freetype_dep,
    libjpeg_dep, libpng_dep, webp_dep, opengl_dep, glew_dep
  ]

  libs = [nacl_native_lib, minizip_lib, findlocale_lib]

  # TODO: don't require all the dependencies the graphical build needs if the graphical build is not required
  daemon = executable('daemon', daemon_graphical_client_srcs,
    cpp_args: ['-DBUILD_GRAPHICAL_CLIENT', '-DBUILD_ENGINE'] + nacl_args,
    dependencies: deps_graphical,
    link_with: libs + [mumblelink_lib],
    include_directories: daemon_public_incs + daemon_private_incs,
    install: true,
    build_by_default: get_option('build_client'),
  )

  daemon_tty = executable('daemon-tty', daemon_tty_client_srcs,
    cpp_args: ['-DBUILD_TTY_CLIENT', '-DBUILD_ENGINE'] + nacl_args,
    dependencies: deps_tty,
    link_with: libs + [mumblelink_lib],
    include_directories: daemon_public_incs + daemon_private_incs,
    install: true,
    build_by_default: get_option('build_tty_client'),
  )

  daemonded = executable('daemonded', daemon_dedicated_server_srcs,
    cpp_args: ['-DBUILD_SERVER', '-DBUILD_ENGINE'] + nacl_args,
    dependencies: deps_dedicated,
    link_with: libs,
    include_directories: daemon_public_incs + daemon_private_incs,
    install: true,
    build_by_default: get_option('build_server'),
  )

  ## of course windows has to do it differently
  #copy_bin = find_program(host_machine.system() == 'windows' ? 'copy' : 'cp')

  #foreach name, target : copied_targets
  #  custom_target(name,
  #    input: target['source'],
  #    output: name,
  #    build_by_default: target['condition'],
  #    command: [copy_bin, '@INPUT@', '@OUTPUT@'],
  #  )
  #endforeach

endif # host_machine.system() != 'pnacl'


###
# Runtime dependencies
###

#if get_option('build_client') or get_option('build_tty_client') or get_option('build_server')
#
#    # The NaCl loader and IRT are required to load .nexe files
#    add_custom_command(TARGET runtime_deps PRE_BUILD
#        COMMAND ${CMAKE_COMMAND} -E copy_if_different
#            ${DEPS_DIR}/sel_ldr${CMAKE_EXECUTABLE_SUFFIX}
#            ${FULL_OUTPUT_DIR}/nacl_loader${CMAKE_EXECUTABLE_SUFFIX}
#    )
#    add_custom_command(TARGET runtime_deps PRE_BUILD
#        COMMAND ${CMAKE_COMMAND} -E copy_if_different
#            ${DEPS_DIR}/irt_core-${ARCH}.nexe
#            ${FULL_OUTPUT_DIR}/irt_core-${ARCH}.nexe
#    )
#
#    # Linux uses a bootstrap program to reserve address space
#    if (LINUX)
#        add_custom_command(TARGET runtime_deps PRE_BUILD
#            COMMAND ${CMAKE_COMMAND} -E copy_if_different
#                ${DEPS_DIR}/nacl_helper_bootstrap
#                ${FULL_OUTPUT_DIR}/nacl_helper_bootstrap
#        )
#    endif()
#
#    # Win32 requires nacl_loader64.exe in order to run on Win64
#    if (WIN32 AND ARCH STREQUAL x86)
#        add_custom_command(TARGET runtime_deps PRE_BUILD
#            COMMAND ${CMAKE_COMMAND} -E copy_if_different
#                ${DEPS_DIR}/sel_ldr64${CMAKE_EXECUTABLE_SUFFIX}
#                ${FULL_OUTPUT_DIR}/nacl_loader64${CMAKE_EXECUTABLE_SUFFIX}
#        )
#        add_custom_command(TARGET runtime_deps PRE_BUILD
#            COMMAND ${CMAKE_COMMAND} -E copy_if_different
#                ${DEPS_DIR}/irt_core-x86_64.nexe
#                ${FULL_OUTPUT_DIR}/irt_core-x86_64.nexe
#        )
#    endif()
#
#    # Mac requires some libraries from external_deps
#    if (APPLE)
#        add_custom_command(TARGET runtime_deps PRE_BUILD
#            COMMAND ${CMAKE_COMMAND} -E copy_directory
#                ${DEPS_DIR}/SDL2.framework
#                ${FULL_OUTPUT_DIR}/SDL2.framework
#        )
#        file(GLOB RUNTIME_LIBS ${DEPS_DIR}/lib/*.dylib)
#        foreach(RUNTIME_LIB ${RUNTIME_LIBS})
#            add_custom_command(TARGET runtime_deps PRE_BUILD
#                COMMAND ${CMAKE_COMMAND} -E copy_if_different
#                    ${RUNTIME_LIB}
#                    ${FULL_OUTPUT_DIR}
#            )
#        endforeach()
#    endif()
#
#    # Windows requires some libraries from external_deps
#    if (WIN32)
#        file(GLOB RUNTIME_LIBS ${DEPS_DIR}/bin/*.dll)
#        foreach(RUNTIME_LIB ${RUNTIME_LIBS})
#            add_custom_command(TARGET runtime_deps PRE_BUILD
#                COMMAND ${CMAKE_COMMAND} -E copy_if_different
#                    ${RUNTIME_LIB}
#                    ${FULL_OUTPUT_DIR}
#            )
#        endforeach()
#    endif()
#
#    if (BUILD_CLIENT)
#        add_dependencies(client runtime_deps)
#    endif()
#    if (BUILD_SERVER)
#        add_dependencies(server runtime_deps)
#    endif()
#    if (BUILD_TTY_CLIENT)
#        add_dependencies(ttyclient runtime_deps)
#    endif()
#endif()
