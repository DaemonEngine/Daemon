nacl_build = get_option('gamelogic_build_type') == 'nacl'

###
# Determining compile flags
###

# TODO: if this is needed for pnacl builds too, should this be passed over from the host?
message(build_machine.system() + ' ' + host_machine.cpu() + ' ' + host_machine.cpu_family())
nacl_args = ['-DNACL_BUILD_ARCH=x86']
nacl_args += ['-DNACL_BUILD_SUBARCH=' + (build_machine.cpu()=='x86_64' ? '64' : '32')]
nacl_args += ['-DNACL_WINDOWS=' + (build_machine.system()=='windows' ? '1' : '0')]
nacl_args += ['-DNACL_LINUX='   + (build_machine.system()=='linux'   ? '1' : '0')]
nacl_args += ['-DNACL_ANDROID=' + (build_machine.system()=='android' ? '1' : '0')] # not very useful, I know
nacl_args += ['-DNACL_OSX='     + (build_machine.system()=='darwin'  ? '1' : '0')]

nacl_module_srcs = files(
  'native_client/src/shared/imc/nacl/nacl_imc.cc',
  'native_client/src/shared/imc/nacl_imc_common.cc',
  'native_client/src/untrusted/nacl/imc_accept.c',
  'native_client/src/untrusted/nacl/imc_connect.c',
  'native_client/src/untrusted/nacl/imc_makeboundsock.c',
  'native_client/src/untrusted/nacl/imc_mem_obj_create.c',
  'native_client/src/untrusted/nacl/imc_recvmsg.c',
  'native_client/src/untrusted/nacl/imc_sendmsg.c',
  'native_client/src/untrusted/nacl/imc_socketpair.c',
)

if nacl_build
  nacl_module_lib = static_library('nacl-module-lib', nacl_module_srcs,
    c_args: nacl_args,
    cpp_args: nacl_args,
    build_by_default: false, # build only if needed
  )
endif


if build_machine.system() == 'darwin'
  nacl_native_srcs = files(
    'native_client/src/shared/imc/nacl_imc_common.cc',
    'native_client/src/shared/imc/posix/nacl_imc_posix.cc',
    'native_client/src/shared/imc/osx/nacl_imc.cc',
  )
elif build_machine.system() == 'linux'
  nacl_native_srcs = files(
    'native_client/src/shared/imc/nacl_imc_common.cc',
    'native_client/src/shared/imc/posix/nacl_imc_posix.cc',
    'native_client/src/shared/imc/linux/nacl_imc.cc',
  )
elif build_machine.system() == 'windows'
  nacl_native_srcs = files(
    'native_client/src/shared/imc/nacl_imc_common.cc',
    'native_client/src/shared/imc/win/nacl_imc.cc',
    'native_client/src/shared/imc/win/nacl_shm.cc',
  )
else
  error('Building NaCl host support is not yet supported. It may very well be possible though, please get in touch.')
endif

nacl_native_lib = static_library('nacl-native-lib', nacl_native_srcs,
  c_args: nacl_args,
  cpp_args: nacl_args,
  build_by_default: false, # build only if needed
)
