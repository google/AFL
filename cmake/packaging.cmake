getProjectVersion(afl_version)
message(STATUS "AFL version: ${afl_version}")

SET(CPACK_GENERATOR "DEB;TGZ;ZIP")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "AFL")
set(CPACK_PACKAGE_VENDOR "Google")
set(CPACK_PACKAGE_DESCRIPTION "Security-oriented fuzzer using compile-time instrumentation and genetic algorithms")
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "CMake ${CMake_VERSION_MAJOR}.${CMake_VERSION_MINOR}")

execute_process(
  COMMAND git config --global user.name
  RESULT_VARIABLE user_name
)

execute_process(
  COMMAND git config --global user.email
  RESULT_VARIABLE user_email
)

if(NOT "${user_name}" STREQUAL "" AND NOT "${user_email}" STREQUAL "")
  set(CPACK_PACKAGE_CONTACT "${user_name} <${user_email}>")
else()
  message(WARNING "Setting a dummy maintainer")
  set(CPACK_PACKAGE_CONTACT "N/A")
endif()

string(REPLACE "." ";" version_components "${afl_version}")
list(GET version_components 0 CPACK_PACKAGE_VERSION_MAJOR)

list(GET version_components 1 version_components)
string(SUBSTRING "${version_components}" 0 2 CPACK_PACKAGE_VERSION_MINOR)
string(SUBSTRING "${version_components}" 2 1 CPACK_PACKAGE_VERSION_PATCH)

include(CPack)

message(STATUS "When creating packages, make sure that `make package` is run as root!")
