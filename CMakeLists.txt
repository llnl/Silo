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

cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

###
# grab the version string
###
file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/SILO_VERSION SILO_VERSION)
# Strip suffix
string(REGEX REPLACE "-.*" "" SILO_VERSION "${SILO_VERSION}")

###
# Set shell path separator char
###
if (WIN32)
    set(_envvar_path_sep ";")
else()
    set(_envvar_path_sep ":")
endif()

###-----------------------------------------------------------------------------
# project command will automatically create cmake vars for major, minor,
# patch and tweak from passed VERSION
###-----------------------------------------------------------------------------
project(Silo VERSION ${SILO_VERSION} LANGUAGES CXX C)

###-----------------------------------------------------------------------------
# location for Silo CMake includes
###-----------------------------------------------------------------------------
set(CMAKE_MODULE_PATH ${Silo_SOURCE_DIR}/CMake)

###-----------------------------------------------------------------------------
# If not already set, use a default build type of Release
###-----------------------------------------------------------------------------

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Set to one of Debug, Release (default), RelWithDebInfo, MinSizeRel." FORCE)
endif()

###-----------------------------------------------------------------------------
# Output directories.
###-----------------------------------------------------------------------------

if(NOT DEFINED CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY lib CACHE INTERNAL "Output directory for libraries.")
endif()

if(NOT DEFINED CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY lib CACHE INTERNAL "Output directory for archives.")
endif()

if(NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin CACHE INTERNAL "Output directory for executables.")
endif()

# turn on folders (VS IDE, ignored otherwise)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Query if the generator is multi-config or not.
get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

#
# create an include dir for generated include files
#
set(silo_build_include_dir ${Silo_BINARY_DIR}/include)
file(MAKE_DIRECTORY ${silo_build_include_dir})

###---------------------------------------------------------------------------
# User options
###---------------------------------------------------------------------------

option(SILO_ENABLE_SHARED  "Build silo shared library." ON)
option(SILO_ENABLE_SILOCK "Enable building of silock" ON)
option(SILO_ENABLE_SILEX "Enable building of silex (requires Qt6)" OFF)
option(SILO_ENABLE_BROWSER "Enable building of browser" ON)
option(SILO_ENABLE_FORTRAN "Enable Fortran interface to Silo" ON)
option(SILO_ENABLE_HDF5 "Enable hdf5 support" ON)
option(SILO_ENABLE_JSON "Enable experimental json features" OFF)
option(SILO_ENABLE_PYTHON_MODULE "Enable python module" OFF)
option(SILO_ENABLE_INSTALL_LITE_HEADERS "Enable installation of PDB Lite headers" OFF)
option(SILO_BUILD_FOR_BSD_LICENSE  "Build BSD licensed version of Silo" ON)
option(SILO_BUILD_FOR_ASAN "Build with flags to run GNU/Clang address sanitizer" OFF)

# Use CMake standard variables for these...
option(BUILD_TESTING "Enable tests" OFF)

include(CMakeDependentOption)

# only turn on the visibility of SILO_ENABLE_ZFP if SILO_ENABLE_HDF5 is ON
# in which case SILO_ENABLE_ZFP defaults to ON
CMAKE_DEPENDENT_OPTION(SILO_ENABLE_ZFP "Enable Lindstrom array compression" ON
                       "SILO_ENABLE_HDF5; NOT WIN32" OFF)

# only turn on the visibility of SILO_ENABLE_FPZIP if
# SILO_BUILD_FOR_BSD is OFF AND SILO_ENABLE_HDF5 is ON
# in which case SILO_ENABLE_FPZIP defaults to ON
CMAKE_DEPENDENT_OPTION(SILO_ENABLE_FPZIP "Enable Lindstrom float 1,2,3D array compression" ON
                       "NOT SILO_BUILD_FOR_BSD_LICENSE;SILO_ENABLE_HDF5" OFF)

# only turn on the visibility of SILO_ENABLE_HZIP if
# SILO_BUILD_FOR_BSD is OFF AND SILO_ENABLE_HDF5 is ON
# in which case SILO_ENABLE_FPZIP defaults to ON
CMAKE_DEPENDENT_OPTION(SILO_ENABLE_HZIP "Enable Lindstrom hex/quad mesh compression" ON
                       "NOT SILO_BUILD_FOR_BSD_LICENSE;SILO_ENABLE_HDF5" OFF)

#
# Conflicting option settings
#
if(NOT SILO_ENABLE_SHARED AND SILO_ENABLE_PYTHON_MODULE)
    message(FATAL "Python module requires SILO_ENABLE_SHARED=ON")
endif()

##
#  Set up a default INSTALL prefix that is peer to the build directory
##
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX ${Silo_BINARY_DIR}/../SiloInstall  CACHE PATH "install prefix" FORCE)
endif()

set(BUILD_SHARED_LIBS ${SILO_ENABLE_SHARED})

###---------------------------------------------------------------------------
# Include any config-site <host>.cmake files if available
#    the config-site file can override any of the default SILO_ settings
#    as well as CMAKE_INSTALL_PREFIX
###---------------------------------------------------------------------------
include(SiloHelpers)

# if user passed -DSILO_CONFIG_SITE to cmake, use it, otherwise
# see if there is a config-site file for this host
if(DEFINED SILO_CONFIG_SITE AND EXISTS ${SILO_CONFIG_SITE})
    include(${SILO_CONFIG_SITE})
else()
    if(WIN32)
        if(EXISTS ${Silo_SOURCE_DIR}/config-site/$ENV{COMPUTERNAME}.cmake)
            include(${Silo_SOURCE_DIR}/config-site/$ENV{COMPUTERNAME}.cmake)
        endif()
    else()
        if(EXISTS ${Silo_SOURCE_DIR}/config-site/$ENV{HOST}.cmake)
            include(${Silo_SOURCE_DIR}/config-site/$ENV{HOST}.cmake)
        elseif(EXISTS ${Silo_SOURCE_DIR}/config-site/$ENV{HOSTNAME}.cmake)
            include(${Silo_SOURCE_DIR}/config-site/$ENV{HOSTNAME}.cmake)
        endif()
    endif()
endif()

###---------------------------------------------------------------------------
# Create a custom target to copy dendencies on Windows
###---------------------------------------------------------------------------

if(WIN32 AND (SILO_ENABLE_SILEX OR SILO_ENABLE_BROWSER))
    add_custom_target(copy_deps ALL)
    set_target_properties(copy_deps PROPERTIES FOLDER utility)
endif()

###-----------------------------------------------------------------------------
# Define and populate CMAKE_INSTALL_*DIR variables
# Needs to be before the inclusion of SiloFind modules, as some of them may
# attempt to use the variables set.
###-----------------------------------------------------------------------------
include(GNUInstallDirs)

###---------------------------------------------------------------------------
# Find necessary packages
###---------------------------------------------------------------------------

##
# Perl, needed to generate lite_pdb.h and lite_score.h
##
find_program(perl_exe NAMES perl perl.exe REQUIRED)


##
# HDF5
##
if(SILO_ENABLE_HDF5)
    if(SILO_ZLIB_DIR)
        # ensure HDF5 finds the correct zlib
        set(ZLIB_ROOT ${SILO_ZLIB_DIR})
    endif()
    include(SiloFindHDF5)
endif()

##
# Zlib  needed for hzip (non-BSD)
#     On Windows, if HDF5 was built with zlib support, need to find the zlib package
#     so that the correct path to the dll can be found and copied to the necessary places
##
if(SILO_ENABLE_HZIP OR (WIN32 AND HDF5_ENABLE_Z_LIB_SUPPORT))
    include(SiloFindZlib)
endif()

##
# szip  if HDF5 was built with szip support on Windows (needed to find dll's)
##
if(WIN32 AND HDF5_ENABLE_SZIP_SUPPORT)
    include(SiloFindSzip)
endif()


###-----------------------------------------------------------------------------
# check for needed includes/functions/symbols
###-----------------------------------------------------------------------------

if(WIN32)
    # windows doesn't have off64_t, but still need to define its SIZEOF
    if(CMAKE_CL_64)
       set(SIZEOF_OFF64_T 8)
    else()
       set(SIZEOF_OFF64_T 4)
    endif()
else()
    include(${CMAKE_ROOT}/Modules/CheckTypeSize.cmake)
    set(CMAKE_REQUIRED_DEFINITIONS -D_LARGEFILE64_SOURCE)
    check_type_size("off64_t" SIZEOF_OFF64_T)
    if(NOT SIZEOF_OFF64_T)
        set(SIZEOF_OFF64_T ${CMAKE_SIZEOF_VOID_P})
    endif()
endif()


##
# Add the needed CMake modules
##
include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckSymbolExists)

##
# check_include_file
##
check_include_file(dlfcn.h HAVE_DLFCN_H)
check_include_file(fcntl.h HAVE_FCNTL_H)
check_include_file(fnmatch.h HAVE_FNMATCH_H)
check_include_file(history.h HAVE_HISTORY_H)
check_include_file(ieeefp.h HAVE_IEEEFP_H)
check_include_file(inttypes.h HAVE_INTTYPES_H)
check_include_file(memory.h HAVE_MEMORY_H)
check_include_file(readline.h HAVE_READLINE_H)
check_include_file(readline/history.h HAVE_READLINE_HISTORY_H)
check_include_file(readline/readline.h HAVE_READLINE_READLINE_H)
check_include_file(stdarg.h HAVE_STDARG_H)
check_include_file(stdint.h HAVE_STDINT_H)
check_include_file(stdlib.h HAVE_STDLIB_H)
check_include_file(strings.h HAVE_STRINGS_H)
check_include_file(string.h HAVE_STRING_H)
check_include_file(sys/fcntl.h HAVE_SYS_FCNTL_H)
check_include_file(sys/stat.h HAVE_SYS_STAT_H)
check_include_file(sys/time.h HAVE_SYS_TIME_H)
check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_include_file(unistd.h HAVE_UNISTD_H)

##
# check_function_exists
##
check_function_exists(fclose   HAVE_FCLOSE_POINTER)
check_function_exists(fflush   HAVE_FFLUSH_POINTER)
check_function_exists(fnmatch  HAVE_FNMATCH)
check_function_exists(fopen    HAVE_FOPEN_POINTER)
check_function_exists(fpclass  HAVE_FPCLASS)
check_function_exists(fprintf  HAVE_FPRINTF_POINTER)
check_function_exists(fread    HAVE_FREAD_POINTER)
check_function_exists(fseek    HAVE_FSEEK_POINTER)
check_function_exists(ftell    HAVE_FTELL_POINTER)
check_function_exists(fwrite   HAVE_FWRITE_POINTER)
check_function_exists(setvbuf  HAVE_SETVBUF_POINTER)
check_function_exists(strerror HAVE_STRERROR)

##
# check_symbol_exists
##
check_symbol_exists(isnan "math.h" HAVE_ISNAN)
check_symbol_exists(memmove "memory.h" HAVE_MEMMOVE)
check_symbol_exists(add_history "readline.h" HAVE_READLINE_HISTORY)
check_symbol_exists(stat64 "sys/stat.h" HAVE_STAT64)
check_symbol_exists(stat "sys/stat.h" HAVE_STAT)

if (HAVE_STAT64)
    add_definitions(-DHAVE_STAT64)
endif()

if (HAVE_STAT)
    add_definitions(-DHAVE_STAT)
endif()

###-----------------------------------------------------------------------------
# Set flags & options from config-site files after everything is set up since
# setting them beforehand can upset some of CMake's own find routines.
###-----------------------------------------------------------------------------
if(DEFINED SILO_C_FLAGS)
    string(APPEND CMAKE_C_FLAGS " ${SILO_C_FLAGS}")
endif()

if(DEFINED SILO_CXX_FLAGS)
    string(APPEND CMAKE_CXX_FLAGS " ${SILO_CXX_FLAGS}")
endif()


###-----------------------------------------------------------------------------
# Create silo.h from silo.h.in
###-----------------------------------------------------------------------------
set(SILO_VERS_MAJ ${Silo_VERSION_MAJOR})
set(SILO_VERS_MIN ${Silo_VERSION_MINOR})
set(SILO_VERS_PAT ${Silo_VERSION_PATCH})
set(SILO_VERS_TAG Silo_version_${SILO_VERS_MAJ}_${SILO_VERS_MIN})
if(SILO_VERS_PAT)
    string(APPEND SILO_VERS_TAG "_${SILO_VERS_PAT}")
endif()

#
# Historically, ABI compatability in Silo is across changes only
# in the last (patch) digit. Minor version changes can cause ABI
# compatibility failures. If that policy is changed in the future
# this SOVERSIONing scheme should be adjusted.
#
set(SILO_SOVERSION ${Silo_VERSION_MAJOR}${Silo_VERSION_MINOR})

configure_file(${Silo_SOURCE_DIR}/src/silo/silo.h.in
               ${silo_build_include_dir}/silo.h)


set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
list(APPEND CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib64")
list(APPEND CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
list(APPEND CMAKE_INSTALL_RPATH "${HDF5_LIBDIR}")
list(REMOVE_DUPLICATES CMAKE_INSTALL_RPATH)

###-----------------------------------------------------------------------------
# Define and populate CMAKE_INSTALL_*DIR variables
###-----------------------------------------------------------------------------
include(GNUInstallDirs)

if(SILO_ENABLE_FORTRAN)
    enable_language(Fortran)
    include(FortranCInterface)
    # for config.h
    FortranCInterface_HEADER(${silo_build_include_dir}/silo_FC.h MACRO_NAMESPACE "FC_")
    if(EXISTS ${silo_build_include_dir}/silo_FC.h)
        set(HAVE_SILO_FC_H 1)
        install(FILES ${silo_build_include_dir}/silo_FC.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
                PERMISSIONS OWNER_READ OWNER_WRITE
                            GROUP_READ GROUP_WRITE
                            WORLD_READ)
    endif()
endif()


###-----------------------------------------------------------------------------
# Silo build targets
###-----------------------------------------------------------------------------

if(WIN32)
   add_compile_definitions(_CRT_SECURE_NO_DEPRECATE _USE_MATH_DEFINES NOMINMAX)
else()
   add_compile_definitions(_LARGEFILE64_SOURCE)
endif()

##
# pdb_detect
##
add_executable(pdb_detect ${Silo_SOURCE_DIR}/src/pdb/detect.c)
target_include_directories(pdb_detect PUBLIC ${silo_build_include_dir})

##
# use pdb_detect to create pdform.h
# Add it as a post-build to pdb_detect
##
add_custom_command(TARGET pdb_detect POST_BUILD
    COMMAND  $<TARGET_FILE:pdb_detect> > pdform.h
    BYPRODUCTS ${silo_build_include_dir}/pdform.h
    WORKING_DIRECTORY ${silo_build_include_dir}
    COMMENT "Creating pdform.h using pdb_detect"
    VERBATIM
    USES_TERMINAL)

##
# Use perl to create lite_pdb.h and lite_score.h
# Add as post-build to pdb_detect
##
add_custom_command(TARGET pdb_detect POST_BUILD
    COMMAND  ${perl_exe} ${Silo_SOURCE_DIR}/config/mklite ${Silo_SOURCE_DIR}/src/pdb/pdb.h lite_pdb.h
    BYPRODUCTS ${silo_build_include_dir}/lite_pdb.h
    WORKING_DIRECTORY ${silo_build_include_dir}
    COMMENT "Creating lite_pdb.h using mklite"
    VERBATIM
    USES_TERMINAL)

add_custom_command(TARGET pdb_detect POST_BUILD
    COMMAND  ${perl_exe} ${Silo_SOURCE_DIR}/config/mklite ${Silo_SOURCE_DIR}/src/score/score.h lite_score.h
    BYPRODUCTS ${silo_build_include_dir}/lite_score.h
    WORKING_DIRECTORY ${silo_build_include_dir}
    COMMENT "Creating lite_score.h using mklite"
    VERBATIM
    USES_TERMINAL)

##
# Silo library
##

# Set up silo library source locations
set(silo_library_sources
    ${Silo_SOURCE_DIR}/src/debug/silo_debug.c
    ${Silo_SOURCE_DIR}/src/filters/f_ale3d.c
    ${Silo_SOURCE_DIR}/src/filters/f_sample.c
    ${Silo_SOURCE_DIR}/src/netcdf/api.c
    ${Silo_SOURCE_DIR}/src/netcdf/ent.c
    ${Silo_SOURCE_DIR}/src/netcdf/netcdf.c
    ${Silo_SOURCE_DIR}/src/netcdf/obj.c
    ${Silo_SOURCE_DIR}/src/netcdf/silo_netcdf.c
    ${Silo_SOURCE_DIR}/src/netcdf/table.c
    ${Silo_SOURCE_DIR}/src/pdb/pdb.c
    ${Silo_SOURCE_DIR}/src/pdb/pdbdir.c
    ${Silo_SOURCE_DIR}/src/pdb/pdbio.c
    ${Silo_SOURCE_DIR}/src/pdb/pdbmm.c
    ${Silo_SOURCE_DIR}/src/pdb/pdbx.c
    ${Silo_SOURCE_DIR}/src/pdb/pdconv.c
    ${Silo_SOURCE_DIR}/src/pdb/pdlow.c
    ${Silo_SOURCE_DIR}/src/pdb/pdmemb.c
    ${Silo_SOURCE_DIR}/src/pdb/pdpath.c
    ${Silo_SOURCE_DIR}/src/pdb/pdrdwr.c
    ${Silo_SOURCE_DIR}/src/pdb_drv/silo_pdb.c
    ${Silo_SOURCE_DIR}/src/score/memmove.c
    ${Silo_SOURCE_DIR}/src/score/scctl.c
    ${Silo_SOURCE_DIR}/src/score/scctla.c
    ${Silo_SOURCE_DIR}/src/score/schash.c
    ${Silo_SOURCE_DIR}/src/score/scstr.c
    ${Silo_SOURCE_DIR}/src/silo/alloc.c
    ${Silo_SOURCE_DIR}/src/silo/extface.c
    ${Silo_SOURCE_DIR}/src/silo/silo.c
    ${Silo_SOURCE_DIR}/src/silo/silo_ns.c
    ${Silo_SOURCE_DIR}/src/silo/utils.c
    ${Silo_SOURCE_DIR}/src/taurus/silo_taurus.c
    ${Silo_SOURCE_DIR}/src/taurus/taurus.c
    ${Silo_SOURCE_DIR}/src/unknown/silo_unknown.c)

if(SILO_ENABLE_FORTRAN AND CMAKE_Fortran_COMPILER)
    list(APPEND silo_library_sources ${Silo_SOURCE_DIR}/src/silo/silo_f.c)
endif()

if(SILO_ENABLE_JSON)
    find_path(JSONC_INCLUDE_DIR json-c/json.h PATH_SUFFIXES json-c)
    find_library(JSONC_LIBRARY NAMES json-c)
    if(JSONC_INCLUDE_DIR AND JSONC_LIBRARY)
        set(JSONC_FOUND TRUE)
    endif()

    if(JSONC_FOUND)
        message(STATUS "Found json-c at ${JSONC_LIBRARY}")
        list(APPEND silo_library_sources ${Silo_SOURCE_DIR}/src/silo/silo_json.c)
        set_source_files_properties(${Silo_SOURCE_DIR}/src/silo/silo_json.c PROPERTIES COMPILE_FLAGS "-I${JSONC_INCLUDE_DIR}")
    else()
        message(FATAL_ERROR "json-c not found")
    endif()
endif()

set(silo_library_include_dirs
    ${silo_build_include_dir}
    ${Silo_SOURCE_DIR}/src/debug
    ${Silo_SOURCE_DIR}/src/filters
    ${Silo_SOURCE_DIR}/src/netcdf
    ${Silo_SOURCE_DIR}/src/pdb
    ${Silo_SOURCE_DIR}/src/pdb_drv
    ${Silo_SOURCE_DIR}/src/score
    ${Silo_SOURCE_DIR}/src/silo
    ${Silo_SOURCE_DIR}/src/taurus_drv
    ${Silo_SOURCE_DIR}/src/unknown)

set(SILO_COMPILE_DEFINES SILO_EXPORTS LITE_EXPORTS)

#
# Handle PDB "Proper" source file if enabled. We have to be careful with ordering
# of this source file relative to PDB Lite source files. Basically, we use the PDB
# driver implementation a second time, except mapping all the lite_ symbols back to
# their "proper" equivalents. This means this source file, when enabled, must come
# ahead of any PDB Lite source files.
#
if(SILO_PACT_DIR)
    list(FIND silo_library_sources ${Silo_SOURCE_DIR}/src/pdb/pdb.c _first_pdb_src_idx)
    list(INSERT silo_library_sources ${_first_pdb_src_idx} ${Silo_SOURCE_DIR}/src/pdbp_drv/silo_pdbp.c)
    list(APPEND SILO_COMPILE_DEFINES DB_PDBP)
    set_source_files_properties(${Silo_SOURCE_DIR}/src/pdbp_drv/silo_pdbp.c PROPERTIES INCLUDE_DIRECTORIES "${SILO_PACT_DIR}/include")
    set(HAVE_PDBP_DRIVER 1)
endif()

##
# Add hdf5 support
##
if(SILO_ENABLE_HDF5 AND HDF5_FOUND)
    list(APPEND silo_library_sources
        ${Silo_SOURCE_DIR}/src/hdf5_drv/H5FDsilo.c
        ${Silo_SOURCE_DIR}/src/hdf5_drv/silo_hdf5.c)

    list(APPEND silo_library_include_dirs
        ${Silo_SOURCE_DIR}/src/hdf5_drv)

    list(APPEND SILO_COMPILE_DEFINES
        AS_SILO_BUILTIN
        BIT_STREAM_WORD_TYPE=uint8)

    if(SILO_ENABLE_ZFP)
        set(HAVE_ZFP 1)
        list(APPEND silo_library_sources
            ${Silo_SOURCE_DIR}/src/hdf5_drv/H5Zzfp.c
            ${Silo_SOURCE_DIR}/src/zfp-0.5.5/src/bitstream.c
            ${Silo_SOURCE_DIR}/src/zfp-0.5.5/src/decode1d.c
            ${Silo_SOURCE_DIR}/src/zfp-0.5.5/src/decode1f.c
            ${Silo_SOURCE_DIR}/src/zfp-0.5.5/src/decode1i.c
            ${Silo_SOURCE_DIR}/src/zfp-0.5.5/src/decode1l.c
            ${Silo_SOURCE_DIR}/src/zfp-0.5.5/src/decode2d.c
            ${Silo_SOURCE_DIR}/src/zfp-0.5.5/src/decode2f.c
            ${Silo_SOURCE_DIR}/src/zfp-0.5.5/src/decode2i.c
            ${Silo_SOURCE_DIR}/src/zfp-0.5.5/src/decode2l.c
            ${Silo_SOURCE_DIR}/src/zfp-0.5.5/src/decode3d.c
            ${Silo_SOURCE_DIR}/src/zfp-0.5.5/src/decode3f.c
            ${Silo_SOURCE_DIR}/src/zfp-0.5.5/src/decode3i.c
            ${Silo_SOURCE_DIR}/src/zfp-0.5.5/src/decode3l.c
            ${Silo_SOURCE_DIR}/src/zfp-0.5.5/src/decode4d.c
            ${Silo_SOURCE_DIR}/src/zfp-0.5.5/src/decode4f.c
            ${Silo_SOURCE_DIR}/src/zfp-0.5.5/src/decode4i.c
            ${Silo_SOURCE_DIR}/src/zfp-0.5.5/src/decode4l.c
            ${Silo_SOURCE_DIR}/src/zfp-0.5.5/src/encode1d.c
            ${Silo_SOURCE_DIR}/src/zfp-0.5.5/src/encode1f.c
            ${Silo_SOURCE_DIR}/src/zfp-0.5.5/src/encode1i.c
            ${Silo_SOURCE_DIR}/src/zfp-0.5.5/src/encode1l.c
            ${Silo_SOURCE_DIR}/src/zfp-0.5.5/src/encode2d.c
            ${Silo_SOURCE_DIR}/src/zfp-0.5.5/src/encode2f.c
            ${Silo_SOURCE_DIR}/src/zfp-0.5.5/src/encode2i.c
            ${Silo_SOURCE_DIR}/src/zfp-0.5.5/src/encode2l.c
            ${Silo_SOURCE_DIR}/src/zfp-0.5.5/src/encode3d.c
            ${Silo_SOURCE_DIR}/src/zfp-0.5.5/src/encode3f.c
            ${Silo_SOURCE_DIR}/src/zfp-0.5.5/src/encode3i.c
            ${Silo_SOURCE_DIR}/src/zfp-0.5.5/src/encode3l.c
            ${Silo_SOURCE_DIR}/src/zfp-0.5.5/src/encode4d.c
            ${Silo_SOURCE_DIR}/src/zfp-0.5.5/src/encode4f.c
            ${Silo_SOURCE_DIR}/src/zfp-0.5.5/src/encode4i.c
            ${Silo_SOURCE_DIR}/src/zfp-0.5.5/src/encode4l.c
            ${Silo_SOURCE_DIR}/src/zfp-0.5.5/src/zfp.c)

        list(APPEND silo_library_include_dirs
            ${Silo_SOURCE_DIR}/src/zfp-0.5.5/include
            ${Silo_SOURCE_DIR}/src/zfp-0.5.5/src)

        list(APPEND SILO_COMPILE_DEFINES
            H5_HAVE_FILTER_ZFP
            H5Z_ZFP_AS_LIB)
    endif()

    if(SILO_ENABLE_FPZIP)
        set(HAVE_FPZIP 1)
        list(APPEND SILO_COMPILE_DEFINES
            FPZIP_FP=FPZIP_FP_SAFE)
        list(APPEND silo_library_sources
            ${Silo_SOURCE_DIR}/src/fpzip/error.cpp
            ${Silo_SOURCE_DIR}/src/fpzip/rcdecoder.cpp
            ${Silo_SOURCE_DIR}/src/fpzip/rcencoder.cpp
            ${Silo_SOURCE_DIR}/src/fpzip/rcqsmodel.cpp
            ${Silo_SOURCE_DIR}/src/fpzip/read.cpp
            ${Silo_SOURCE_DIR}/src/fpzip/write.cpp)
        list(APPEND silo_library_include_dirs
            ${Silo_SOURCE_DIR}/src/fpzip)
    endif()

    if(SILO_ENABLE_HZIP)
        set(HAVE_HZIP 1)
        list(APPEND silo_library_sources
            ${Silo_SOURCE_DIR}/src/hzip/hzmdecoder.cpp
            ${Silo_SOURCE_DIR}/src/hzip/hzmencoder.cpp
            ${Silo_SOURCE_DIR}/src/hzip/hzmio.cpp
            ${Silo_SOURCE_DIR}/src/hzip/hzmpredictor.cpp
            ${Silo_SOURCE_DIR}/src/hzip/hzmread.cpp
            ${Silo_SOURCE_DIR}/src/hzip/hzmwrite.cpp
            ${Silo_SOURCE_DIR}/src/hzip/hznio.cpp
            ${Silo_SOURCE_DIR}/src/hzip/hznpredictor.cpp
            ${Silo_SOURCE_DIR}/src/hzip/hznread.cpp
            ${Silo_SOURCE_DIR}/src/hzip/hznwrite.cpp
            ${Silo_SOURCE_DIR}/src/hzip/hzutil.cpp)

        list(APPEND silo_library_include_dirs
            ${Silo_SOURCE_DIR}/src/hzip)
    endif()

    if(ZLIB_FOUND)
        list(APPEND silo_library_include_dirs ${ZLIB_INCLUDE_DIR})
    else()
        list(APPEND SILO_COMPILE_DEFINES WITHOUT_ZLIB)
    endif()
endif()

add_library(silo ${silo_library_sources})
set_target_properties(silo PROPERTIES VERSION ${SILO_VERSION} SOVERSION ${SILO_SOVERSION})

target_compile_definitions(silo PRIVATE ${SILO_COMPILE_DEFINES})
add_dependencies(silo pdb_detect)
target_include_directories(silo PRIVATE ${silo_library_include_dirs})

if(UNIX)
    target_link_libraries(silo m)
endif()

# add list of public headers to be installed
set(silo_public_headers
    ${silo_build_include_dir}/silo.h
    ${Silo_SOURCE_DIR}/src/silo/silo_exports.h
    ${Silo_SOURCE_DIR}/src/silo/pmpio.h)

if(SILO_ENABLE_INSTALL_LITE_HEADERS)
    list(APPEND silo_public_headers
        ${silo_build_include_dir}/lite_pdb.h
        ${silo_build_include_dir}/lite_score.h)
endif()

if(SILO_ENABLE_FORTRAN AND CMAKE_Fortran_COMPILER)
    list(APPEND silo_public_headers
        ${Silo_SOURCE_DIR}/src/silo/silo.inc
        ${Silo_SOURCE_DIR}/src/silo/silo_f9x.inc)
endif()

##
# Add PACT/PDB Proper support
##
if(SILO_PACT_DIR)
    if((EXISTS ${SILO_PACT_DIR}/lib/libpdb.a OR
        EXISTS ${SILO_PACT_DIR}/lib/libpdb.so OR
        EXISTS ${SILO_PACT_DIR}/lib/libpdb.dylib) AND
        EXISTS ${SILO_PACT_DIR}/include/pdb.h AND
        EXISTS ${SILO_PACT_DIR}/include/score.h)

        find_library(PDB_LIB   NAMES pdb   HINTS "${SILO_PACT_DIR}/lib")
        find_library(PML_LIB   NAMES pml   HINTS "${SILO_PACT_DIR}/lib")
        find_library(SCORE_LIB NAMES score HINTS "${SILO_PACT_DIR}/lib")

        if(NOT PDB_LIB OR NOT PML_LIB OR NOT SCORE_LIB)
          message(FATAL_ERROR "PACT libs not found under ${SILO_PACT_DIR}/lib")
        endif()

        target_link_directories(silo PRIVATE "${SILO_PACT_DIR}/lib")
        target_link_libraries(silo ${PDB_LIB} ${PML_LIB} ${SCORE_LIB})
    endif()
endif()

target_link_libraries(silo ${CMAKE_DL_LIBS})

if(WIN32)
    list(APPEND silo_public_headers ${Silo_SOURCE_DIR}/src/silo/silo_win32_compatibility.h)
    # Also install silodiff.bat into bin
    install(FILES ${Silo_SOURCE_DIR}/resources/silodiff.bat DESTINATION ${CMAKE_INSTALL_BINDIR}
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                        GROUP_READ GROUP_WRITE GROUP_EXECUTE
                        WORLD_READ             WORLD_EXECUTE)

    if(SILO_ENABLE_SILEX OR SILO_ENABLE_BROWSER)
        # copy silo dll to build/bin dir
        add_custom_command(TARGET copy_deps POST_BUILD
             COMMAND ${CMAKE_COMMAND} -E copy_if_different
             $<TARGET_FILE:silo> ${Silo_BINARY_DIR}/bin/$<$<BOOL:${is_multi_config}>:$<CONFIG>>/)
    endif()
endif()

install(FILES ${silo_public_headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        PERMISSIONS OWNER_READ OWNER_WRITE
                    GROUP_READ GROUP_WRITE
                    WORLD_READ)

# Symlink old style silo_f77.inc
if(SILO_ENABLE_FORTRAN AND CMAKE_Fortran_COMPILER)
    install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink silo.inc silo_f77.inc
        WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR})")
endif()

if(SILO_ENABLE_HDF5 AND HDF5_FOUND)
    # since we change the ouputname from silo to siloh5 when building
    # with HDF5, will use $<TARGET_LINKER_FILE:silo> in place of plain 'silo'
    # when telling broswer and silex to link with silo
    set_target_properties(silo PROPERTIES OUTPUT_NAME siloh5)
    target_link_libraries(silo ${HDF5_C_LIBRARIES})
    target_include_directories(silo PRIVATE ${HDF5_INCLUDE_DIRS})
endif()

if(SILO_ENABLE_JSON AND JSONC_FOUND)
    target_link_libraries(silo ${JSONC_LIBRARY})
endif()

# setting up for installing Silo CMake config files
set(silo_targets_name "SiloTargets")
install(TARGETS silo EXPORT ${silo_targets_name}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PERMISSIONS OWNER_READ OWNER_WRITE
                    GROUP_READ GROUP_WRITE
                    WORLD_READ)

##
# silex
##
if(SILO_ENABLE_SILEX)
    add_subdirectory(tools/silex)
endif()

##
# browser
##
if(SILO_ENABLE_BROWSER)
    add_subdirectory(tools/browser)
endif()

##
# silock
##
if(SILO_ENABLE_SILOCK AND NOT WIN32)
    add_executable(silock
        ${Silo_SOURCE_DIR}/tools/silock/silock.c)
    target_link_libraries(silock silo)
    if(UNIX)
        target_link_libraries(silock m ${CMAKE_DL_LIBS})
    endif()
    if(SILO_BUILD_FOR_ASAN)
        target_compile_options(silock PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer -g -O1)
        target_link_options(silock PRIVATE -Wl,--no-as-needed -fsanitize=address,undefined)
    endif()
    target_include_directories(silock PRIVATE
        ${silo_build_include_dir}
        ${Silo_SOURCE_DIR}/src/silo)
    install(TARGETS silock DESTINATION ${CMAKE_INSTALL_BINDIR} EXPORT ${silo_targets_name}
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                        GROUP_READ GROUP_WRITE GROUP_EXECUTE
                        WORLD_READ             WORLD_EXECUTE)
endif()

##
# Python module
##
if(SILO_ENABLE_PYTHON_MODULE)
   add_subdirectory(tools/python)
endif()

###-----------------------------------------------------------------------------
# Testing support
###-----------------------------------------------------------------------------
include(CTest)
if(BUILD_TESTING)

    set(_debug_build_types Debug RelWithDebInfo)
    if(CMAKE_BUILD_TYPE IN_LIST _debug_build_types)
        find_program(MEMORYCHECK_COMMAND NAMES valgrind)
        if (MEMORYCHECK_COMMAND)
            set(MEMORYCHECK_COMMAND_OPTIONS
                "--leak-check=full --track-origins=yes --error-exitcode=1")
        endif()
        if(SILO_BUILD_FOR_ASAN)
            string(APPEND CMAKE_C_FLAGS_DEBUG " -fsanitize=address,undefined -fno-omit-frame-pointer -g -O1")
            string(APPEND CMAKE_CXX_FLAGS_DEBUG " -fsanitize=address,undefined -fno-omit-frame-pointer -g -O1")
            string(APPEND CMAKE_EXE_LINKER_FLAGS_DEBUG " -Wl,--no-as-needed -fsanitize=address,undefined")
            string(APPEND CMAKE_SHARED_LINKER_FLAGS_DEBUG " -Wl,--no-as-needed -fsanitize=address,undefined")
            string(APPEND CMAKE_MODULE_LINKER_FLAGS_DEBUG " -Wl,--no-as-needed -fsanitize=address,undefined")
            set(SILO_ASAN_OPTIONS "detect_leaks=1;halt_on_error=0;malloc_context_size=50;fast_unwind_on_malloc=0;detect_stack_use_after_return=1")

            execute_process(
                COMMAND ${CMAKE_C_COMPILER} -print-file-name=libasan.so
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                RESULT_VARIABLE find_libasan_result
                OUTPUT_VARIABLE SILO_LIBASAN_PATH
                ERROR_VARIABLE find_libasan_error
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )
            if(NOT find_libasan_result EQUAL 0)
                message(STATUS "Finding libasan.so failed (${find_libasan_error}). Some tests disabled")
            endif()
            execute_process(
                COMMAND ${CMAKE_C_COMPILER} -print-file-name=libubsan.so
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                RESULT_VARIABLE find_libubsan_result
                OUTPUT_VARIABLE SILO_LIBUBSAN_PATH
                ERROR_VARIABLE find_libubsan_error
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )
            if(NOT find_libasan_result EQUAL 0)
                message(STATUS "Finding libubsan.so failed (${find_libubsan_error}). Some tests disabled")
            endif()

            # Make CTest include a script before running tests in this (sub)tree
            set_property(DIRECTORY PROPERTY TEST_INCLUDE_FILES "${CMAKE_SOURCE_DIR}/tests/CTestAsanEnv.cmake")

        endif()
    endif()

    if(SILO_ENABLE_HDF5)
        # Using h5stat as test case, determine directory holding HDF5 tools
        find_program(_h5stat_exe NAMES h5stat
            HINTS ${HDF5_ROOT} ${HDF5_DIR} ${SILO_HDF5_DIR}
            PATH_SUFFIXES bin ../bin Tools/bin)
        get_filename_component(_h5stat_dir "${_h5stat_exe}" DIRECTORY)
        set(_sh_path "${_h5stat_dir}${_envvar_path_sep}$ENV{PATH}")
    endif()

    add_subdirectory(tests)

endif()

#
# Define a target for testing the installation itself
# We do this regardless of whether BUILD_TESTING is enabled because
# it is a quick way to actually test `make install` has installed
# all the pieces that are needed.
#
set(INSTALL_TEST_BINARY_DIR "${CMAKE_BINARY_DIR}/install-tests")

add_custom_target(installcheck
  COMMAND ${CMAKE_COMMAND} -E rm -rf "${INSTALL_TEST_BINARY_DIR}"
  COMMAND ${CMAKE_COMMAND}
          -S "${CMAKE_SOURCE_DIR}/tests/install"
          -B "${INSTALL_TEST_BINARY_DIR}"
          -DCMAKE_PREFIX_PATH="${CMAKE_INSTALL_PREFIX}/lib/cmake;${HDF5_ROOT};${HDF5_DIR};${SILO_HDF5_DIR}"
          -DSilo_DIR="${CMAKE_INSTALL_PREFIX}/lib/cmake/Silo"
          -Dhdf5_DIR="${HDF5_DIR}"
          -DSILO_ENABLE_FORTRAN=${SILO_ENABLE_FORTRAN}
          -DSILO_ENABLE_INSTALL_LITE_HEADERS=${SILO_ENABLE_INSTALL_LITE_HEADERS}
          -DSILO_ENABLE_PYTHON_MODULE=${SILO_ENABLE_PYTHON_MODULE}
  COMMAND ${CMAKE_COMMAND} --build "${INSTALL_TEST_BINARY_DIR}"
  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
          --test-dir "${INSTALL_TEST_BINARY_DIR}"
  COMMENT "Configuring, building, and running tests against *installed* Silo"
)

###-----------------------------------------------------------------------------
# Create config.h from config.h.in
# Put this near end so we capture any set(SOMEVAR 1) calls above.
###-----------------------------------------------------------------------------

# set up some vars to be passed to config.h.in
set(SILO_BUG "https://github.com/LLNL/Silo/discussions")
set(SILO_NAME "silo")
set(SILO_TARNAME "silo")
set(SILO_URL "https://silo.llnl.gov")

# these are always defined
set(HAVE_PDB_DRIVER 1)
set(HAVE_NETCDF_DRIVER 1)
set(HAVE_TAURUS_DRIVER 1)

set(STDC_HEADERS 1)

configure_file(${Silo_SOURCE_DIR}/CMake/config.h.cmake.in
               ${silo_build_include_dir}/config.h)


#-----------------------------------------------------------------------------
# INSTALL
#-----------------------------------------------------------------------------

###
# Set up the CMAKE install location
###
set(CMAKE_MODULE_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/Silo)

install(EXPORT ${silo_targets_name} DESTINATION ${CMAKE_MODULE_INSTALL_DIR})

include(${CMAKE_ROOT}/Modules/CMakePackageConfigHelpers.cmake)

###
# Set up the include install location for use with the package config files
###
set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR})

configure_package_config_file(${Silo_SOURCE_DIR}/CMake/SiloConfig.cmake.in
  ${Silo_BINARY_DIR}/cmake/SiloConfig.cmake
  INSTALL_DESTINATION ${CMAKE_MODULE_INSTALL_DIR}
  PATH_VARS INCLUDE_INSTALL_DIR
  )

write_basic_package_version_file(
    ${Silo_BINARY_DIR}/cmake/SiloConfigVersion.cmake
    VERSION ${SILO_VERSION}
    COMPATIBILITY SameMajorVersion)
# should compatibility be something else? available values are:
# COMPATIBILITY <AnyNewerVersion|SameMajorVersion|SameMinorVersion|ExactVersion

install(FILES ${Silo_BINARY_DIR}/cmake/SiloConfig.cmake
              ${Silo_BINARY_DIR}/cmake/SiloConfigVersion.cmake
        DESTINATION ${CMAKE_MODULE_INSTALL_DIR}
        PERMISSIONS OWNER_READ OWNER_WRITE
                    GROUP_READ GROUP_WRITE
                    WORLD_READ)

