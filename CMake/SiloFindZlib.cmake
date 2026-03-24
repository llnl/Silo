# Copyright (C) 1994-2021 Lawrence Livermore National Security, LLC.
# LLNL-CODE-425250.
# All rights reserved.
#
# This file is part of Silo. For details, see silo.llnl.gov.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#    * Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the disclaimer below.
#    * Redistributions in binary form must reproduce the above copyright
#      notice, this list of conditions and the disclaimer (as noted
#      below) in the documentation and/or other materials provided with
#      the distribution.
#    * Neither the name of the LLNS/LLNL nor the names of its
#      contributors may be used to endorse or promote products derived
#      from this software without specific prior written permission.
#
# THIS SOFTWARE  IS PROVIDED BY  THE COPYRIGHT HOLDERS  AND CONTRIBUTORS
# "AS  IS" AND  ANY EXPRESS  OR IMPLIED  WARRANTIES, INCLUDING,  BUT NOT
# LIMITED TO, THE IMPLIED  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A  PARTICULAR  PURPOSE ARE  DISCLAIMED.  IN  NO  EVENT SHALL  LAWRENCE
# LIVERMORE  NATIONAL SECURITY, LLC,  THE U.S.  DEPARTMENT OF  ENERGY OR
# CONTRIBUTORS BE LIABLE FOR  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR  CONSEQUENTIAL DAMAGES  (INCLUDING, BUT NOT  LIMITED TO,
# PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS  OF USE,  DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER  IN CONTRACT, STRICT LIABILITY,  OR TORT (INCLUDING
# NEGLIGENCE OR  OTHERWISE) ARISING IN  ANY WAY OUT  OF THE USE  OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# This work was produced at Lawrence Livermore National Laboratory under
# Contract No.  DE-AC52-07NA27344 with the DOE.
#
# Neither the  United States Government nor  Lawrence Livermore National
# Security, LLC nor any of  their employees, makes any warranty, express
# or  implied,  or  assumes  any  liability or  responsibility  for  the
# accuracy, completeness,  or usefulness of  any information, apparatus,
# product, or  process disclosed, or  represents that its use  would not
# infringe privately-owned rights.
#
# Any reference herein to  any specific commercial products, process, or
# services by trade name,  trademark, manufacturer or otherwise does not
# necessarily  constitute or imply  its endorsement,  recommendation, or
# favoring  by  the  United  States  Government  or  Lawrence  Livermore
# National Security,  LLC. The views  and opinions of  authors expressed
# herein do not necessarily state  or reflect those of the United States
# Government or Lawrence Livermore National Security, LLC, and shall not
# be used for advertising or product endorsement purposes.
#
##############################################################################


###
# Uses find_package to find the zlib library
#   If SILO_ZLIB_DIR is defined, uses it to tell CMake where to look.
#   Use CONFIG version of find_package if zlib-config.cmake exists.
###
if(SILO_ZLIB_DIR AND EXISTS ${SILO_ZLIB_DIR}/cmake/zlib-config.cmake)
    # this works for zlib with CMake
    find_package(ZLIB PATHS ${SILO_ZLIB_DIR} CONFIG NO_DEFAULT_PATH)
else()
    if(SILO_ZLIB_DIR AND EXISTS ${SILO_ZLIB_DIR})
        # help CMake find the specified zlib
        set(ZLIB_ROOT ${SILO_ZLIB_DIR})
    endif()
    find_package(ZLIB)
endif()

if(ZLIB_FOUND)
    # needed for config.h
    set(HAVE_LIBZ 1)
    set(HAVE_ZLIB_H 1)
    if(WIN32)
        # CMake's find_package sets IMPORTED_LOCATION_RELEASE for ZLIB::ZLIB
        # to the import library instead of the .dll, which is needed for
        # installation alongside silex.
        # Get the dll location the hard way instead.
        #
        cmake_path(REPLACE_EXTENSION ZLIB_LIBRARY "dll" OUTPUT_VARIABLE zlib_dll)
        cmake_path(GET zlib_dll PARENT_PATH zlib_lib_dir)
        cmake_path(GET zlib_dll FILENAME zlib_dll_name)
        cmake_path(SET zlib_dll NORMALIZE ${zlib_lib_dir}/../bin/${zlib_dll_name})

        if(SILO_ENABLE_SILEX OR SILO_ENABLE_BROWSER)
            install(FILES ${zlib_dll} DESTINATION ${CMAKE_INSTALL_BINDIR}
                    PERMISSIONS OWNER_READ OWNER_WRITE
                                GROUP_READ GROUP_WRITE
                                WORLD_READ)

            add_custom_command(TARGET copy_deps POST_BUILD
                 COMMAND ${CMAKE_COMMAND} -E copy_if_different
                 ${zlib_dll} ${Silo_BINARY_DIR}/bin/$<$<BOOL:${is_multi_config}>:$<CONFIG>>/)
        endif()
    endif()
else()
    message(FATAL_ERROR "Could not find zlib, you may want to try setting SILO_ZLIB_DIR")
endif()

