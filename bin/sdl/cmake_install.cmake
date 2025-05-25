# Install script for directory: /Users/felixjaschul/Downloads/3d-code/lib/SDL

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set path to fallback-tool for dependency-resolution.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/Users/felixjaschul/Downloads/3d-code/bin/sdl/libSDL2-2.0.0.dylib")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libSDL2-2.0.0.dylib" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libSDL2-2.0.0.dylib")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" -x "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libSDL2-2.0.0.dylib")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/Users/felixjaschul/Downloads/3d-code/bin/sdl/libSDL2-2.0.dylib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/Users/felixjaschul/Downloads/3d-code/bin/sdl/libSDL2main.a")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libSDL2main.a" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libSDL2main.a")
    execute_process(COMMAND "/usr/bin/ranlib" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libSDL2main.a")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/Users/felixjaschul/Downloads/3d-code/bin/sdl/libSDL2.a")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libSDL2.a" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libSDL2.a")
    execute_process(COMMAND "/usr/bin/ranlib" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libSDL2.a")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/Users/felixjaschul/Downloads/3d-code/bin/sdl/libSDL2_test.a")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libSDL2_test.a" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libSDL2_test.a")
    execute_process(COMMAND "/usr/bin/ranlib" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libSDL2_test.a")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/SDL2/SDL2Targets.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/SDL2/SDL2Targets.cmake"
         "/Users/felixjaschul/Downloads/3d-code/bin/sdl/CMakeFiles/Export/f084604df1a27ef5b4fef7c7544737d1/SDL2Targets.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/SDL2/SDL2Targets-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/SDL2/SDL2Targets.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/SDL2" TYPE FILE FILES "/Users/felixjaschul/Downloads/3d-code/bin/sdl/CMakeFiles/Export/f084604df1a27ef5b4fef7c7544737d1/SDL2Targets.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^()$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/SDL2" TYPE FILE FILES "/Users/felixjaschul/Downloads/3d-code/bin/sdl/CMakeFiles/Export/f084604df1a27ef5b4fef7c7544737d1/SDL2Targets-noconfig.cmake")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/SDL2/SDL2mainTargets.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/SDL2/SDL2mainTargets.cmake"
         "/Users/felixjaschul/Downloads/3d-code/bin/sdl/CMakeFiles/Export/f084604df1a27ef5b4fef7c7544737d1/SDL2mainTargets.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/SDL2/SDL2mainTargets-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/SDL2/SDL2mainTargets.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/SDL2" TYPE FILE FILES "/Users/felixjaschul/Downloads/3d-code/bin/sdl/CMakeFiles/Export/f084604df1a27ef5b4fef7c7544737d1/SDL2mainTargets.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^()$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/SDL2" TYPE FILE FILES "/Users/felixjaschul/Downloads/3d-code/bin/sdl/CMakeFiles/Export/f084604df1a27ef5b4fef7c7544737d1/SDL2mainTargets-noconfig.cmake")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/SDL2/SDL2staticTargets.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/SDL2/SDL2staticTargets.cmake"
         "/Users/felixjaschul/Downloads/3d-code/bin/sdl/CMakeFiles/Export/f084604df1a27ef5b4fef7c7544737d1/SDL2staticTargets.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/SDL2/SDL2staticTargets-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/SDL2/SDL2staticTargets.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/SDL2" TYPE FILE FILES "/Users/felixjaschul/Downloads/3d-code/bin/sdl/CMakeFiles/Export/f084604df1a27ef5b4fef7c7544737d1/SDL2staticTargets.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^()$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/SDL2" TYPE FILE FILES "/Users/felixjaschul/Downloads/3d-code/bin/sdl/CMakeFiles/Export/f084604df1a27ef5b4fef7c7544737d1/SDL2staticTargets-noconfig.cmake")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/SDL2/SDL2testTargets.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/SDL2/SDL2testTargets.cmake"
         "/Users/felixjaschul/Downloads/3d-code/bin/sdl/CMakeFiles/Export/f084604df1a27ef5b4fef7c7544737d1/SDL2testTargets.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/SDL2/SDL2testTargets-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/SDL2/SDL2testTargets.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/SDL2" TYPE FILE FILES "/Users/felixjaschul/Downloads/3d-code/bin/sdl/CMakeFiles/Export/f084604df1a27ef5b4fef7c7544737d1/SDL2testTargets.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^()$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/SDL2" TYPE FILE FILES "/Users/felixjaschul/Downloads/3d-code/bin/sdl/CMakeFiles/Export/f084604df1a27ef5b4fef7c7544737d1/SDL2testTargets-noconfig.cmake")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Devel" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/SDL2" TYPE FILE FILES
    "/Users/felixjaschul/Downloads/3d-code/bin/sdl/SDL2Config.cmake"
    "/Users/felixjaschul/Downloads/3d-code/bin/sdl/SDL2ConfigVersion.cmake"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/cmake/sdlfind.cmake"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/SDL2" TYPE FILE FILES
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_assert.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_atomic.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_audio.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_bits.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_blendmode.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_clipboard.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_copying.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_cpuinfo.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_egl.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_endian.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_error.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_events.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_filesystem.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_gamecontroller.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_gesture.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_guid.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_haptic.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_hidapi.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_hints.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_joystick.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_keyboard.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_keycode.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_loadso.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_locale.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_log.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_main.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_messagebox.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_metal.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_misc.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_mouse.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_mutex.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_name.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_opengl.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_opengl_glext.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_opengles.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_opengles2.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_opengles2_gl2.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_opengles2_gl2ext.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_opengles2_gl2platform.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_opengles2_khrplatform.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_pixels.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_platform.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_power.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_quit.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_rect.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_render.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_rwops.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_scancode.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_sensor.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_shape.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_stdinc.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_surface.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_system.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_syswm.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_test.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_test_assert.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_test_common.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_test_compare.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_test_crc32.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_test_font.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_test_fuzzer.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_test_harness.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_test_images.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_test_log.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_test_md5.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_test_memory.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_test_random.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_thread.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_timer.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_touch.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_types.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_version.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_video.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/SDL_vulkan.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/begin_code.h"
    "/Users/felixjaschul/Downloads/3d-code/lib/SDL/include/close_code.h"
    "/Users/felixjaschul/Downloads/3d-code/bin/sdl/include/SDL2/SDL_revision.h"
    "/Users/felixjaschul/Downloads/3d-code/bin/sdl/include-config-/SDL2/SDL_config.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/licenses/SDL2" TYPE FILE FILES "/Users/felixjaschul/Downloads/3d-code/lib/SDL/LICENSE.txt")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig" TYPE FILE FILES "/Users/felixjaschul/Downloads/3d-code/bin/sdl/sdl2.pc")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  
            execute_process(COMMAND /opt/homebrew/bin/cmake -E create_symlink
              "libSDL2-2.0.dylib" "libSDL2.dylib"
              WORKING_DIRECTORY "/Users/felixjaschul/Downloads/3d-code/bin/sdl")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE FILE FILES "/Users/felixjaschul/Downloads/3d-code/bin/sdl/libSDL2.dylib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE PROGRAM FILES "/Users/felixjaschul/Downloads/3d-code/bin/sdl/sdl2-config")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/aclocal" TYPE FILE FILES "/Users/felixjaschul/Downloads/3d-code/lib/SDL/sdl2.m4")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/Users/felixjaschul/Downloads/3d-code/bin/sdl/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
if(CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_COMPONENT MATCHES "^[a-zA-Z0-9_.+-]+$")
    set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
  else()
    string(MD5 CMAKE_INST_COMP_HASH "${CMAKE_INSTALL_COMPONENT}")
    set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INST_COMP_HASH}.txt")
    unset(CMAKE_INST_COMP_HASH)
  endif()
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/Users/felixjaschul/Downloads/3d-code/bin/sdl/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
