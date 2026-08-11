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
# Uses find_package to find the hdf5 library
#   If SILO_HDF5_DIR is defined, uses it to tell CMake where to look.
###

if(DEFINED SILO_HDF5_DIR AND EXISTS ${SILO_HDF5_DIR})
    # help cmake find the hdf5 we want to use
    set(HDF5_ROOT ${SILO_HDF5_DIR})
endif()

function(_silo_hdf5_detect_zlib_support_from_header out_var)
    set(_header_candidates)
    foreach(_include_dir ${HDF5_INCLUDE_DIRS} ${HDF5_INCLUDE_DIR})
        if(_include_dir)
            list(APPEND _header_candidates
                "${_include_dir}/H5pubconf.h"
                "${_include_dir}/hdf5/H5pubconf.h")
        endif()
    endforeach()
    list(REMOVE_DUPLICATES _header_candidates)

    foreach(_header IN LISTS _header_candidates)
        if(EXISTS "${_header}")
            file(STRINGS "${_header}" _zlib_markers
                 REGEX "^#define H5_HAVE_FILTER_DEFLATE 1$|^#define H5_HAVE_ZLIB_H 1$")
            if(_zlib_markers)
                set(${out_var} TRUE PARENT_SCOPE)
                return()
            endif()
        endif()
    endforeach()

    set(${out_var} FALSE PARENT_SCOPE)
endfunction()

find_package(HDF5)

# Prefer the concrete imported HDF5 C library targets before HDF5::HDF5.
# CMake's FindHDF5 module may synthesize HDF5::HDF5 with a flattened raw
# libhdf5.a link interface even when the upstream package also exports
# hdf5-static/hdf5-shared with the full transitive dependency set.
unset(HDF5_C_TARGET)
set(HDF5_HAS_IMPORTED_C_TARGET FALSE)
foreach(_hdf5_c_target hdf5-shared hdf5-static hdf5::hdf5-shared hdf5::hdf5-static hdf5::hdf5 HDF5::HDF5)
    if(TARGET ${_hdf5_c_target})
        set(HDF5_C_TARGET ${_hdf5_c_target})
        set(HDF5_HAS_IMPORTED_C_TARGET TRUE)
        break()
    endif()
endforeach()

if(NOT HDF5_FOUND)
    include(FindPackageHandleStandardArgs)
    
    set(_hints)
    if(DEFINED SILO_HDF5_DIR AND EXISTS ${SILO_HDF5_DIR})
        set(_hints HINTS ${SILO_HDF5_DIR}/include ${SILO_HDF5_DIR}/lib ${SILO_HDF5_DIR}/lib64)
    endif()
    
    find_path(HDF5_INCLUDE_DIR NAMES hdf5.h ${_hints} PATH_SUFFIXES hdf5)
    find_library(HDF5_LIBRARY NAMES hdf5 ${_hints} PATH_SUFFIXES hdf5)
    
    FIND_PACKAGE_HANDLE_STANDARD_ARGS(HDF5 REQUIRED_VARS HDF5_LIBRARY HDF5_INCLUDE_DIR)
    
    if(HDF5_FOUND AND NOT TARGET HDF5::HDF5)
        set(HDF5_LIBRARIES ${HDF5_LIBRARY})
        set(HDF5_INCLUDE_DIRS ${HDF5_INCLUDE_DIR})
        set(HDF5_C_LIBRARIES ${HDF5_LIBRARY})
        
        add_library(HDF5::HDF5 UNKNOWN IMPORTED)
        set_target_properties(HDF5::HDF5 PROPERTIES
            IMPORTED_LOCATION "${HDF5_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${HDF5_INCLUDE_DIR}")
        set(HDF5_C_TARGET HDF5::HDF5)
    endif()
    
endif()

if(HDF5_FOUND)
    # needed for config.h
    set(HAVE_HDF5_H 1)
    set(HAVE_HDF5_DRIVER 1)
    set(HAVE_LIBHDF5 1)
    set(HDF5_PROVIDES_ZLIB_SUPPORT FALSE)

    if(DEFINED HDF5_ENABLE_Z_LIB_SUPPORT)
        set(HDF5_PROVIDES_ZLIB_SUPPORT ${HDF5_ENABLE_Z_LIB_SUPPORT})
    elseif(NOT HDF5_HAS_IMPORTED_C_TARGET)
        _silo_hdf5_detect_zlib_support_from_header(HDF5_PROVIDES_ZLIB_SUPPORT)
    endif()


    # On Windows need to have hdf5's dll installed with browser/silex
    # in order for the executables to work
    if(WIN32)
        if(HDF5_C_TARGET)
            get_target_property(HDF5_DLL ${HDF5_C_TARGET} IMPORTED_LOCATION_RELEASE)
        endif()
        if(HDF5_DLL AND (SILO_ENABLE_SILEX OR SILO_ENABLE_BROWSER))
            install(FILES ${HDF5_DLL} DESTINATION ${CMAKE_INSTALL_BINDIR}
                    PERMISSIONS OWNER_READ OWNER_WRITE
                                GROUP_READ GROUP_WRITE
                                WORLD_READ)

            add_custom_command(TARGET copy_deps POST_BUILD
                 COMMAND ${CMAKE_COMMAND} -E copy_if_different
                 ${HDF5_DLL} ${Silo_BINARY_DIR}/bin/$<$<BOOL:${is_multi_config}>:$<CONFIG>>/)
        endif()
    endif()

else()
    message(FATAL_ERROR "An explicit request for HDF5 was made but HDF5 was not found. You may want to try setting SILO_HDF5_DIR")
endif()
