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
# browser
##

add_executable(browser
    array.c
    bif.c
    browser.c
    cons.c
    file.c
    func.c
    lex.c
    num.c
    obj.c
    output.c
    parse.c
    prim.c
    ptr.c
    range.c
    sdo.c
    stc.c
    str.c
    switch.c
    sym.c)

add_dependencies(browser silo)

# make it put the executable in the root bin dir not a tools/browser subdir
set_target_properties(browser PROPERTIES
                      RUNTIME_OUTPUT_DIRECTORY ${Silo_BINARY_DIR}/bin)

target_include_directories(browser PRIVATE
    ${silo_build_include_dir}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Silo_SOURCE_DIR}/src/silo
    ${Silo_SOURCE_DIR}/src/filters)

target_link_libraries(browser $<TARGET_LINKER_FILE:silo>)

if(UNIX)
    target_link_libraries(browser m)
endif()

target_link_libraries(browser ${CMAKE_DL_LIBS})

if(SILO_ENABLE_HDF5 AND HDF5_FOUND)
    target_link_libraries(browser ${HDF5_C_LIBRARIES})
endif()

if(SILO_BUILD_FOR_ASAN)
    target_compile_options(browser PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer -g -O1)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID STREQUAL "GNU")
        target_link_options(browser PRIVATE -Wl,--no-as-needed -fsanitize=address,undefined)
    else()
        target_link_options(browser PRIVATE -fsanitize=address,undefined)
    endif()
endif()

install(TARGETS browser DESTINATION ${CMAKE_INSTALL_BINDIR} EXPORT ${silo_targets_name}
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                    GROUP_READ GROUP_WRITE GROUP_EXECUTE
                    WORLD_READ             WORLD_EXECUTE)

if(NOT WIN32)
    install(FILES silodiff silofile
            DESTINATION ${CMAKE_INSTALL_BINDIR}
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                        GROUP_READ GROUP_WRITE GROUP_EXECUTE
                        WORLD_READ             WORLD_EXECUTE)
endif()


