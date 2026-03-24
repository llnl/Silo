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

##
# silex
##

# find Qt first
include(SiloFindQt6)

if(NOT Qt6_FOUND)
    message(WARNING "Silex depends on Qt6, but it could not be found.  Silex will not be built")
    return()
endif()

set(silex_src_files
    Explorer.cpp
    main.cpp
    SiloArrayView.cpp
    SiloDirTreeView.cpp
    SiloDirView.cpp
    SiloFile.cpp
    SiloObjectView.cpp
    SiloValueView.cpp
    SiloView.cpp)

if(WIN32)
    list(APPEND silex_src_files resources/silex.rc)
endif()

add_executable(silex  ${silex_src_files})

add_dependencies(silex silo)
# make it put the executable in the root bin dir not a tools/silex subdir
set_target_properties(silex PROPERTIES AUTOMOC ON
                      RUNTIME_OUTPUT_DIRECTORY ${Silo_BINARY_DIR}/bin)
target_link_libraries(silex $<TARGET_LINKER_FILE:silo>
    Qt6::Core Qt6::Gui Qt6::Widgets)
target_include_directories(silex PRIVATE
     ${silo_build_include_dir}
     ${Silo_SOURCE_DIR}/src/silo
     ${Silo_SOURCE_DIR}/tools/silex)

install(TARGETS silex DESTINATION ${CMAKE_INSTALL_BINDIR} EXPORT ${silo_targets_name}
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                    GROUP_READ GROUP_WRITE GROUP_EXECUTE
                    WORLD_READ             WORLD_EXECUTE)


### ---------------------------------------------------------------------
# Ensure necessary Qt dependencies (like platform plugins) are deployed
### ---------------------------------------------------------------------

##
#  MacOS can possbily be done similar to Windows using macdeployqt if its needed
##

##
#  Windows
##
if(WIN32 AND TARGET Qt6::windeployqt)
    # execute windeployqt directory after build
    # creates tmp dir used for installation
    # also deploys to build dir so silex can be run from build
    # Remove compiler-runtime from being deployed, it installs vc_redist
    # executable instead of the vcruntime dlls.
    add_custom_command(TARGET silex
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_CURRENT_BINARY_DIR}/windeployqt"
        COMMAND set PATH=%PATH%$<SEMICOLON>${qt5_install_prefix}/bin
        COMMAND Qt6::windeployqt --verbose 0 --no-compiler-runtime --no-system-d3d-compiler --dir "${CMAKE_CURRENT_BINARY_DIR}/windeployqt" "$<TARGET_FILE:silex>"
        COMMAND Qt6::windeployqt --verbose 0 --no-compiler-runtime --no-system-d3d-compiler --dir "${Silo_BINARY_DIR}/bin/$<CONFIG>" "$<TARGET_FILE:silex>"
    )

    # copy deployment directory during installation
    install(
        DIRECTORY
        "${CMAKE_CURRENT_BINARY_DIR}/windeployqt/"
        DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()


if(LINUX) # ??
   # only necessary for install?

   # change output name to silex.exe
   # write silex script that sets LD_LIBRARY_PATH appropriately as in:
   #!/bin/sh
   #env LD_LIBRARY_PATH=/usr/gapps/silo/4.11/toss_3_x86_64_ib_gcc.6.1.0/libso:/usr/workspace/visit/visit/thirdparty_shared/3.2.0/toss3/qt/5.14.2/linux-x86_64_gcc-6.1/lib /usr/gapps/silo/4.11/toss_3_x86_64_ib_gcc.6.1.0/bin/silex.exe

   # should we copy Qt's .so's or go ahead and extract the lib path somehow for use
   # in the script?

   # can possibly use configure_file and @CMAKE_INSTALL_PREFIX@ @QT5_LIB_DIR@
endif()

