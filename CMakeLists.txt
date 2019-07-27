cmake_minimum_required(VERSION 3.10)
project(afl)

include(cmake/options.cmake)
include(cmake/utils.cmake)
include(cmake/packaging.cmake)

set(common_header_list
  alloc-inl.h
  config.h
  debug.h
  types.h
)

function(aflMain)
  printUserMessages()

  generateCompilerTester()
  generateSettingsTarget()

  generateCompilerWrappers()
  generateAssemblerWrapper()
  generateAFLTargets()

  generateBuildTester()
  generateInstallTargets()
endfunction()

function(printUserMessages)
  if("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_C_COMPILER_ID}" STREQUAL "AppleClang")
    message(WARNING "LLVM users: see llvm_mode/README.llvm for a faster alternative to afl-gcc.")
  endif()

  if("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
    message(WARNING "Fuzzing on MacOS X is slow because of the unusually high overhead of fork() on this OS. Consider using Linux or *BSD. You can also use VirtualBox (virtualbox.org) to put AFL inside a Linux or *BSD VM.")
  endif()
endfunction()

function(generateCompilerWrappers)
  # Generate the base wrapper
  add_executable(afl-gcc
    ${common_header_list}
    afl-gcc.c
  )

  dependOnCompilerTest(afl-gcc)
  target_link_libraries(afl-gcc PRIVATE c_settings)

  # Create the symbolic links
  set(wrapper_list
    afl-g++
    afl-clang
    afl-clang++
  )

  foreach(wrapper ${wrapper_list})
    set(link_destination "${CMAKE_CURRENT_BINARY_DIR}/${wrapper}")

    add_custom_command(
      OUTPUT "${link_destination}"
      COMMAND "${CMAKE_COMMAND}" -E create_symlink afl-gcc "${wrapper}"
      COMMENT "Generating: '${wrapper}' symlink"
      WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
      VERBATIM
    )

    add_custom_target(
      "${wrapper}_generator"
      DEPENDS "${link_destination}"
    )

    add_dependencies(afl-gcc "${wrapper}_generator")
  endforeach()
endfunction()

function(generateAssemblerWrapper)
  # Generate the base executable
  add_executable(afl-as
    ${common_header_list}
    afl-as.h
    afl-as.c
  )

  dependOnCompilerTest(afl-as)
  target_link_libraries(afl-as PRIVATE c_settings)

  # Generate the symbolic link
  set(link_destination "${CMAKE_CURRENT_BINARY_DIR}/as")

  add_custom_command(
    OUTPUT "${link_destination}"
    COMMAND "${CMAKE_COMMAND}" -E create_symlink afl-as as
    COMMENT "Generating: 'as' symlink"
    VERBATIM
  )

  add_custom_target(as_generator
    DEPENDS "${link_destination}"
  )

  add_dependencies(afl-as as_generator)
endfunction()

function(generateAFLTargetHelper base_target_name)
  set(target_name "afl-${base_target_name}")
  set(source_file_name "${target_name}.c")

  add_executable("${target_name}"
    ${common_header_list}
    "${source_file_name}"
  )

  dependOnCompilerTest("${target_name}")
  target_link_libraries("${target_name}" PRIVATE c_settings)
endfunction()

function(generateAFLTargets)
  set(afl_target_list
    fuzz
    showmap
    tmin
    analyze
    gotcpu
  )

  foreach(afl_target ${afl_target_list})
    generateAFLTargetHelper("${afl_target}")
  endforeach()
endfunction()

function(generateBuildTester)
  # Select the wrapper
  if("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_C_COMPILER_ID}" STREQUAL "AppleClang")
    set(compiler "afl-clang")
  else()
    set(compiler "afl-gcc")
  endif()

  set(compiler "${CMAKE_CURRENT_BINARY_DIR}/${compiler}")

  # Gather the flags we have both from the environment and from our settings
  get_target_property(compile_option_list c_settings INTERFACE_COMPILE_OPTIONS)
  if("${compile_option_list}" STREQUAL "compile_option_list-NOTFOUND")
    unset(compile_option_list)
  endif()

  list(APPEND compile_option_list $ENV{CFLAGS})

  get_target_property(link_libs c_settings INTERFACE_LINK_LIBRARIES)
  if("${link_libs}" STREQUAL "link_libs-NOTFOUND")
    unset(link_libs)
  endif()

  get_target_property(link_opts c_settings INTERFACE_LINK_OPTIONS)
  if("${link_opts}" STREQUAL "link_opts-NOTFOUND")
    unset(link_opts)
  endif()

  foreach(link_option ${link_libs} ${link_opts})
    if(TARGET "${link_option}")
      continue()
    endif()

    if("${link_option}" STREQUAL "${CMAKE_DL_LIBS}")
      continue()
    endif()

    list(APPEND link_option_list "${link_option}")
  endforeach()

  list(APPEND link_option_list $ENV{LDFLAGS})

  # Compile the test binary
  set(test_binary_path "${CMAKE_CURRENT_BINARY_DIR}/test-instr")

  add_custom_command(
    OUTPUT "${test_binary_path}"
    COMMAND "${CMAKE_COMMAND}" -E env --unset=AFL_USE_ASAN --unset=AFL_USE_MSAN AFL_QUIET=1 AFL_INST_RATIO=100 AFL_PATH="${CMAKE_CURRENT_BINARY_DIR}" "${compiler}" ${compile_option_list} "${CMAKE_CURRENT_SOURCE_DIR}/test-instr.c" -o "${test_binary_path}" ${link_option_list}
    COMMENT "Generating: test-instr"
    VERBATIM
  )

  add_custom_target(test-instr_generator
    DEPENDS "${test_binary_path}"
  )

  add_dependencies(test-instr_generator
    afl-gcc
    afl-as
  )

  # Create the afl-showmap runners
  set(showmap_output_path0 "${CMAKE_CURRENT_BINARY_DIR}/.test-instr0")
  set(showmap_output_path1 "${CMAKE_CURRENT_BINARY_DIR}/.test-instr1")

  add_custom_command(
    OUTPUT "${showmap_output_path0}" "${showmap_output_path1}"
    COMMAND "${CMAKE_COMMAND}" -E echo "0" | "$<TARGET_FILE:afl-showmap>" -m none -q -o "${showmap_output_path0}" "${test_binary_path}"
    COMMAND "${CMAKE_COMMAND}" -E echo "1" | "$<TARGET_FILE:afl-showmap>" -m none -q -o "${showmap_output_path1}" "${test_binary_path}"
    COMMAND "${CMAKE_COMMAND}" -E compare_files "${showmap_output_path0}" "${showmap_output_path1}"
    COMMENT "Testing the build"
    VERBATIM
  )

  add_custom_target(
    test_build
    DEPENDS "${showmap_output_path0}" "${showmap_output_path1}"
  )

  add_dependencies(test_build test-instr_generator)
endfunction()

function(generateInstallTargets)
  # Targets, symlinks, scripts
  set(target_list
    afl-gcc
    afl-fuzz
    afl-showmap
    afl-tmin
    afl-analyze
    afl-gotcpu
    afl-as
  )

  set(source_file_list
    #afl-plot
    #afl-cmin
    afl-whatsup
  )

  set(binary_file_list
    afl-clang
    afl-clang++
    afl-g++
    as
  )

  set(helper_binary_file_list
    afl-as
  )

  install(TARGETS ${target_list} DESTINATION "bin")

  foreach(source_file ${source_file_list})
    install(FILES ${source_file_list} DESTINATION "bin" PERMISSIONS WORLD_READ WORLD_EXECUTE)
  endforeach()

  foreach(binary_file ${binary_file_list})
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${binary_file}" DESTINATION "bin")
  endforeach()

  foreach(binary_file ${helper_binary_file_list})
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${binary_file}" DESTINATION "lib/afl")
  endforeach()

  # Documentation
  install(FILES "docs/README" "docs/ChangeLog" DESTINATION "share/doc/afl")
  install(DIRECTORY "docs/" DESTINATION "share/doc/afl" FILES_MATCHING PATTERN "*.txt")

  # Tests
  install(DIRECTORY "testcases/" DESTINATION "share/afl" FILES_MATCHING PATTERN "*")
  install(DIRECTORY "dictionaries/" DESTINATION "share/afl" FILES_MATCHING PATTERN "*")
endfunction()

aflMain()
