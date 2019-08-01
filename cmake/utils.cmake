#
# Copyright 2013-present Google Inc. All rights reserved.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at:
#   http://www.apache.org/licenses/LICENSE-2.0
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

cmake_minimum_required(VERSION 3.10)

function(getProjectVersion output_variable)
  file(READ "${CMAKE_SOURCE_DIR}/config.h" config_header_contents)
  if("${config_header_contents}" STREQUAL "")
    set("${output_variable}" "${output_variable}-NOTFOUND" PARENT_SCOPE)
    return()
  endif()

  string(FIND "${config_header_contents}" "#define VERSION" definition_index)
  if(${definition_index} EQUAL -1)
    set("${output_variable}" "${output_variable}-NOTFOUND" PARENT_SCOPE)
    return()
  endif()

  string(SUBSTRING "${config_header_contents}" ${definition_index} -1 config_header_contents)

  string(FIND "${config_header_contents}" "\"" version_start_index)
  if(${version_start_index} EQUAL -1)
    set("${output_variable}" "${output_variable}-NOTFOUND" PARENT_SCOPE)
    return()
  endif()

  math(EXPR version_start_index "${version_start_index} + 1")

  string(SUBSTRING "${config_header_contents}" ${version_start_index} -1 config_header_contents)

  string(FIND "${config_header_contents}" "\"" version_end_index)
  if(${version_end_index} EQUAL -1)
    set("${output_variable}" "${output_variable}-NOTFOUND" PARENT_SCOPE)
    return()
  endif()

  string(SUBSTRING "${config_header_contents}" 0 ${version_end_index} version_field)
 
  if("${version_field}" STREQUAL "")
    set("${output_variable}" "${output_variable}-NOTFOUND" PARENT_SCOPE)
    return()
  endif()

  set("${output_variable}" "${version_field}" PARENT_SCOPE)
endfunction()

function(generateCompilerTester)
  if(AFL_NO_X86)
    message(WARNING "Note: skipping x86 compilation checks (AFL_NO_X86 set)")
    return()
  endif()

  set(compiler_test_source "main() { __asm__(\"xorb %al, %al\"); }")
  set(compiler_test_source_path "${CMAKE_CURRENT_BINARY_DIR}/test_x86.c")

  add_custom_command(
    OUTPUT "${compiler_test_source_path}"
    COMMAND "${CMAKE_COMMAND}" -E echo "${compiler_test_source}" > "${compiler_test_source_path}"
    COMMENT "Generating: test_x86.c"
    VERBATIM
  )

  add_custom_target(
    test_x86_generator
    DEPENDS "${compiler_test_source_path}"
  )

  set(compiler_test_output_path "${CMAKE_CURRENT_BINARY_DIR}/test_x86.o")
  set(compiler_test_log_path "${CMAKE_CURRENT_BINARY_DIR}/test_x86.txt")

  add_custom_command(
    OUTPUT "${compiler_test_output_path}"
    COMMAND "${CMAKE_C_COMPILER}" -w -x c -c "${compiler_test_source_path}" -o "${compiler_test_output_path}" > "${compiler_test_log_path}"
    COMMENT "Testing compiler: ${CMAKE_C_COMPILER} (log: ${compiler_test_log_path})"
    VERBATIM
  )

  add_custom_target(
    test_x86_runner
    DEPENDS "${compiler_test_output_path}"
  )

  add_dependencies(test_x86_runner test_x86_generator)
endfunction()

function(dependOnCompilerTest target_name)
  add_dependencies("${target_name}" test_x86_runner)
endfunction()

function(generateSettingsTarget)
  add_library(c_settings INTERFACE)

  target_compile_options(c_settings INTERFACE
    -O3
    -funroll-loops
    -Wall
    -g
    -Wno-pointer-sign
  )

  target_compile_definitions(c_settings INTERFACE
    _FORTIFY_SOURCE=2
    AFL_PATH=\"${CMAKE_INSTALL_PREFIX}/lib/afl\"
    DOC_PATH=\"${CMAKE_INSTALL_PREFIX}/share/doc/afl\"
    BIN_PATH=\"${CMAKE_INSTALL_PREFIX}/bin\"
  )

  target_link_libraries(c_settings INTERFACE ${CMAKE_DL_LIBS})
endfunction()
