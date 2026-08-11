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


# NAME: name of test, required
# LANG: language of source files (optional)
#       e.g. for CXX source files with '.C' extension
# SRC:  list of source files, required
function(silo_add_test)
    set(options)
    set(oneValueArgs NAME LANG)
    set(multiValueArgs SRC)
    cmake_parse_arguments(sat "${options}" "${oneValueArgs}"
        "${multiValueArgs}" ${ARGN} )

    if("NAME" IN_LIST sat_KEYWORDS_MISSING_VALUES)
        message(WARNING "silo_add_test: NAME argument is missing a value.")
        return()
    elseif(NOT DEFINED sat_NAME)
        message(WARNING "silo_add_test requires NAME argument.")
        return()
    endif()
    if("SRC" IN_LIST sat_KEYWORDS_MISSING_VALUES)
        message(WARNING "silo_add_test: SRC argument is missing a value.")
        return()
    elseif(NOT DEFINED sat_SRC)
        message(WARNING "silo_add_test: requires SRC argument")
        return()
    endif()


    add_executable(${sat_NAME} ${sat_SRC})
    if(${is_multi_config})
        # change output directory using generator expression to avoid
        # VS adding per-configuration subdir
        set_target_properties(${sat_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY
            ${silo_test_output_dir}/$<$<CONFIG:Debug>:>)
    else()
        set_target_properties(${sat_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY
            ${silo_test_output_dir})
    endif()
    if("LANG" IN_LIST sat_KEYWORDS_MISSING_VALUES)
        message(WARNING "silo_add_test: LANG argument is missing a value.")
        return()
    elseif(DEFINED sat_LANG)
        set_property(SOURCE ${sat_SRC} PROPERTY
                      LANGUAGE ${sat_LANG})
    endif()

    add_dependencies(${sat_NAME} silo)
    target_compile_definitions(${sat_NAME} PRIVATE PDB_LITE)

    target_link_libraries(${sat_NAME} $<TARGET_LINKER_FILE:silo>)
    if(UNIX)
        target_link_libraries(${sat_NAME} dl m)
    endif()

    target_include_directories(${sat_NAME}  PRIVATE
        ${silo_build_include_dir}
        ${Silo_SOURCE_DIR}/src/silo
        ${SILO_TESTS_SOURCE_DIR})

    set_target_properties(${sat_NAME} PROPERTIES FOLDER testing/tests)

    if(SILO_ENABLE_HDF5 AND HDF5_FOUND)
        silo_target_link_hdf5(${sat_NAME})
    endif()

    if(WIN32)
        target_compile_definitions(${sat_NAME} PRIVATE PDB_LITE)
    endif()
endfunction()
