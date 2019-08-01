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

getProjectVersion(afl_version)
message(STATUS "AFL version: ${afl_version}")

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
  set(CPACK_GENERATOR "ZIP")
else()
  set(CPACK_GENERATOR "DEB;TGZ;ZIP")
endif()

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "AFL")
set(CPACK_PACKAGE_VENDOR "Google")
set(CPACK_PACKAGE_DESCRIPTION "Security-oriented fuzzer using compile-time instrumentation and genetic algorithms")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "CMake ${CMake_VERSION_MAJOR}.${CMake_VERSION_MINOR}")

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
