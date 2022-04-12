
subdir('nacl')


minizip_srcs = files(
  'minizip/ioapi.c',
  'minizip/ioapi.h',
  'minizip/unzip.c',
  'minizip/unzip.h',
)

# minizip is used by both the vm files and the engine.
# this is why we provide two versions in the case of a nacl build
minizip_lib = static_library('minizip', minizip_srcs,
  build_by_default: false, # build only if needed
  include_directories: include_directories('minizip'))


if host_machine.system() != 'pnacl'

  findlocale_srcs = files(
    'findlocale/findlocale.c',
  )

  # This is only used for the engine.
  findlocale_lib = static_library('findlocale', findlocale_srcs,
    include_directories: include_directories('findlocale'),
    build_by_default: false, # build only if needed
  )


  mumblelink_srcs = files(
    'mumblelink/libmumblelink.cpp',
    'mumblelink/libmumblelink.h',
  )

  # This is only used for the engine.
  mumblelink_lib = static_library('mumblelink', mumblelink_srcs,
    include_directories: include_directories('mumblelink'),
    build_by_default: false, # build only if needed
  )

endif # host_machine.system() == 'pnacl'

#set(PDCURSESLIST
#    ${LIB_DIR}/pdcurses/pdcurses/addch.c
#    ${LIB_DIR}/pdcurses/pdcurses/addchstr.c
#    ${LIB_DIR}/pdcurses/pdcurses/addstr.c
#    ${LIB_DIR}/pdcurses/pdcurses/attr.c
#    ${LIB_DIR}/pdcurses/pdcurses/beep.c
#    ${LIB_DIR}/pdcurses/pdcurses/bkgd.c
#    ${LIB_DIR}/pdcurses/pdcurses/border.c
#    ${LIB_DIR}/pdcurses/pdcurses/clear.c
#    ${LIB_DIR}/pdcurses/pdcurses/color.c
#    ${LIB_DIR}/pdcurses/pdcurses/debug.c
#    ${LIB_DIR}/pdcurses/pdcurses/delch.c
#    ${LIB_DIR}/pdcurses/pdcurses/deleteln.c
#    ${LIB_DIR}/pdcurses/pdcurses/deprec.c
#    ${LIB_DIR}/pdcurses/pdcurses/getch.c
#    ${LIB_DIR}/pdcurses/pdcurses/getstr.c
#    ${LIB_DIR}/pdcurses/pdcurses/getyx.c
#    ${LIB_DIR}/pdcurses/pdcurses/inch.c
#    ${LIB_DIR}/pdcurses/pdcurses/inchstr.c
#    ${LIB_DIR}/pdcurses/pdcurses/initscr.c
#    ${LIB_DIR}/pdcurses/pdcurses/inopts.c
#    ${LIB_DIR}/pdcurses/pdcurses/insch.c
#    ${LIB_DIR}/pdcurses/pdcurses/insstr.c
#    ${LIB_DIR}/pdcurses/pdcurses/instr.c
#    ${LIB_DIR}/pdcurses/pdcurses/kernel.c
#    ${LIB_DIR}/pdcurses/pdcurses/mouse.c
#    ${LIB_DIR}/pdcurses/pdcurses/move.c
#    ${LIB_DIR}/pdcurses/pdcurses/outopts.c
#    ${LIB_DIR}/pdcurses/pdcurses/overlay.c
#    ${LIB_DIR}/pdcurses/pdcurses/pad.c
#    ${LIB_DIR}/pdcurses/pdcurses/panel.c
#    ${LIB_DIR}/pdcurses/pdcurses/printw.c
#    ${LIB_DIR}/pdcurses/pdcurses/refresh.c
#    ${LIB_DIR}/pdcurses/pdcurses/scanw.c
#    ${LIB_DIR}/pdcurses/pdcurses/scroll.c
#    ${LIB_DIR}/pdcurses/pdcurses/scr_dump.c
#    ${LIB_DIR}/pdcurses/pdcurses/slk.c
#    ${LIB_DIR}/pdcurses/pdcurses/termattr.c
#    ${LIB_DIR}/pdcurses/pdcurses/terminfo.c
#    ${LIB_DIR}/pdcurses/pdcurses/touch.c
#    ${LIB_DIR}/pdcurses/pdcurses/util.c
#    ${LIB_DIR}/pdcurses/pdcurses/window.c
#    ${LIB_DIR}/pdcurses/win32a/pdcclip.c
#    ${LIB_DIR}/pdcurses/win32a/pdcdisp.c
#    ${LIB_DIR}/pdcurses/win32a/pdcgetsc.c
#    ${LIB_DIR}/pdcurses/win32a/pdckbd.c
#    ${LIB_DIR}/pdcurses/win32a/pdcscrn.c
#    ${LIB_DIR}/pdcurses/win32a/pdcsetsc.c
#    ${LIB_DIR}/pdcurses/win32a/pdcutil.c
#)

if host_machine.system() == 'pnacl'
  # This is only used for the NaCl modules. For all other targets, the host zlib
  # is used (sourced from external_deps if not available on target platform).
  # TODO: this may change with the other, non-linux platform

  zlib_incs = [ include_directories('zlib') ]

  zlib_srcs = files(
    'zlib/adler32.c',
    'zlib/compress.c',
    'zlib/crc32.c',
    'zlib/crc32.h',
    'zlib/deflate.c',
    'zlib/deflate.h',
    'zlib/gzclose.c',
    'zlib/gzguts.h',
    'zlib/gzlib.c',
    'zlib/gzread.c',
    'zlib/gzwrite.c',
    'zlib/infback.c',
    'zlib/inffast.c',
    'zlib/inffast.h',
    'zlib/inffixed.h',
    'zlib/inflate.c',
    'zlib/inflate.h',
    'zlib/inftrees.c',
    'zlib/inftrees.h',
    'zlib/trees.c',
    'zlib/trees.h',
    'zlib/uncompr.c',
    'zlib/zconf.h',
    'zlib/zlib.h',
    'zlib/zutil.c',
    'zlib/zutil.h',
  )

  zlib_lib = static_library('zlib-native', minizip_srcs,
    build_by_default: false, # build only if needed
    include_directories: zlib_incs,
  )

  zlib_dep = declare_dependency(
    include_directories: zlib_incs,
    link_with: zlib_lib,
  )

endif # host_machine.system() == 'pnacl'

# TODO: breakpad
#
#if (WIN32)
#    set(BREAKPAD_COMMON_LIST
#        ${LIB_DIR}/breakpad/src/common/windows/guid_string.cc
#        ${LIB_DIR}/breakpad/src/common/windows/guid_string.h
#        ${LIB_DIR}/breakpad/src/common/windows/http_upload.cc
#        ${LIB_DIR}/breakpad/src/common/windows/http_upload.h
#        ${LIB_DIR}/breakpad/src/common/windows/string_utils.cc
#    )
#    set(BREAKPAD_CRASHGENERATIONSERVER_LIST
#        ${LIB_DIR}/breakpad/src/client/windows/crash_generation/client_info.cc
#        ${LIB_DIR}/breakpad/src/client/windows/crash_generation/crash_generation_server.cc
#        ${LIB_DIR}/breakpad/src/client/windows/crash_generation/minidump_generator.cc
#        ${LIB_DIR}/breakpad/src/client/windows/crash_generation/client_info.h
#        ${LIB_DIR}/breakpad/src/client/windows/crash_generation/crash_generation_client.h
#        ${LIB_DIR}/breakpad/src/client/windows/crash_generation/crash_generation_server.h
#        ${LIB_DIR}/breakpad/src/client/windows/crash_generation/minidump_generator.h
#    )
#    set(BREAKPAD_CRASHGENERATIONCLIENT_LIST
#        ${LIB_DIR}/breakpad/src/client/windows/crash_generation/crash_generation_client.h
#        ${LIB_DIR}/breakpad/src/client/windows/crash_generation/crash_generation_client.cc
#        ${LIB_DIR}/breakpad/src/client/windows/crash_generation/crash_generation_server.h
#    )
#    set(BREAKPAD_EXCEPTIONHANDLER_LIST
#        ${LIB_DIR}/breakpad/src/client/windows/handler/exception_handler.cc
#        ${LIB_DIR}/breakpad/src/client/windows/handler/exception_handler.h
#    )
#elseif (LINUX)
#    set(BREAKPAD_LIST
#        ${LIB_DIR}/breakpad/src/client/linux/crash_generation/crash_generation_client.cc
#        ${LIB_DIR}/breakpad/src/client/linux/crash_generation/crash_generation_server.cc
#        ${LIB_DIR}/breakpad/src/client/linux/dump_writer_common/thread_info.cc
#        ${LIB_DIR}/breakpad/src/client/linux/dump_writer_common/ucontext_reader.cc
#        ${LIB_DIR}/breakpad/src/client/linux/handler/exception_handler.cc
#        ${LIB_DIR}/breakpad/src/client/linux/handler/exception_handler.h
#        ${LIB_DIR}/breakpad/src/client/linux/handler/minidump_descriptor.cc
#        ${LIB_DIR}/breakpad/src/client/linux/handler/minidump_descriptor.h
#        ${LIB_DIR}/breakpad/src/client/linux/log/log.cc
#        ${LIB_DIR}/breakpad/src/client/linux/log/log.h
#        ${LIB_DIR}/breakpad/src/client/linux/microdump_writer/microdump_writer.cc
#        ${LIB_DIR}/breakpad/src/client/linux/microdump_writer/microdump_writer.h
#        ${LIB_DIR}/breakpad/src/client/linux/minidump_writer/linux_core_dumper.cc
#        ${LIB_DIR}/breakpad/src/client/linux/minidump_writer/linux_dumper.cc
#        ${LIB_DIR}/breakpad/src/client/linux/minidump_writer/linux_ptrace_dumper.cc
#        ${LIB_DIR}/breakpad/src/client/linux/minidump_writer/minidump_writer.cc
#        ${LIB_DIR}/breakpad/src/client/minidump_file_writer-inl.h
#        ${LIB_DIR}/breakpad/src/client/minidump_file_writer.cc
#        ${LIB_DIR}/breakpad/src/client/minidump_file_writer.h
#        ${LIB_DIR}/breakpad/src/common/convert_UTF.cc
#        ${LIB_DIR}/breakpad/src/common/convert_UTF.h
#        ${LIB_DIR}/breakpad/src/common/md5.cc
#        ${LIB_DIR}/breakpad/src/common/md5.h
#        ${LIB_DIR}/breakpad/src/common/string_conversion.cc
#        ${LIB_DIR}/breakpad/src/common/string_conversion.h
#        ${LIB_DIR}/breakpad/src/common/linux/elf_core_dump.cc
#        ${LIB_DIR}/breakpad/src/common/linux/elfutils.cc
#        ${LIB_DIR}/breakpad/src/common/linux/elfutils.h
#        ${LIB_DIR}/breakpad/src/common/linux/file_id.cc
#        ${LIB_DIR}/breakpad/src/common/linux/file_id.h
#        ${LIB_DIR}/breakpad/src/common/linux/guid_creator.cc
#        ${LIB_DIR}/breakpad/src/common/linux/guid_creator.h
#        ${LIB_DIR}/breakpad/src/common/linux/linux_libc_support.cc
#        ${LIB_DIR}/breakpad/src/common/linux/memory_mapped_file.cc
#        ${LIB_DIR}/breakpad/src/common/linux/safe_readlink.cc
#    )
#endif()
