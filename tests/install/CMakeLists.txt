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
cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

#
# This whole project is to test whether `make install` installs Silo correctly
#
project(SiloInstallTest)

include(CTest)

find_package(Silo CONFIG REQUIRED)

if(SILO_ENABLE_INSTALL_LITE_HEADERS)
    add_executable(pdb_lite_test pdb_lite_test.c)
    target_compile_definitions(pdb_lite_test PRIVATE PDB_LITE)
    target_include_directories(pdb_lite_test PRIVATE ${SILO_INCLUDE_DIR})
    target_link_libraries(pdb_lite_test PRIVATE silo)
    if(UNIX AND NOT APPLE)
        target_link_libraries(pdb_lite_test PRIVATE m)
    endif()
    add_test(NAME pdb_lite_test COMMAND $<TARGET_FILE:pdb_lite_test>)
endif()

#
# C Test for both PDB and HDF5 drivers
# If HDF5 driver not present, test will be logged as skipped by CTest
#
add_executable(c_test c_test.c)
target_include_directories(c_test PRIVATE ${SILO_INCLUDE_DIR})
target_link_libraries(c_test PRIVATE silo)
add_test(NAME c_test COMMAND $<TARGET_FILE:c_test>)
add_test(NAME c_test-hdf5 COMMAND $<TARGET_FILE:c_test> DB_HDF5)
set_tests_properties(c_test-hdf5 PROPERTIES SKIP_RETURN_CODE 17)

#
# Cxx Test for both PDB and HDF5 drivers
# If HDF5 driver not present, test will be logged as skipped by CTest
#
add_executable(cxx_test cxx_test.cxx)
target_include_directories(cxx_test PRIVATE ${SILO_INCLUDE_DIR})
target_link_libraries(cxx_test PRIVATE silo)
add_test(NAME cxx_test COMMAND $<TARGET_FILE:cxx_test>)
add_test(NAME cxx_test-hdf5 COMMAND $<TARGET_FILE:cxx_test> DB_HDF5)
set_tests_properties(cxx_test-hdf5 PROPERTIES SKIP_RETURN_CODE 17)

if(SILO_ENABLE_FORTRAN)
    enable_language(Fortran)

    add_executable(f77_test f77_test.f)
    target_include_directories(f77_test PRIVATE ${SILO_INCLUDE_DIR})
    target_link_libraries(f77_test PRIVATE silo)
    add_test(NAME f77_test COMMAND $<TARGET_FILE:f77_test>)
    add_test(NAME f77_test-hdf5 COMMAND $<TARGET_FILE:f77_test> DB_HDF5)
    set_tests_properties(f77_test-hdf5 PROPERTIES SKIP_RETURN_CODE 17)

    add_executable(f90_test f90_test.f90)
    target_include_directories(f90_test PRIVATE ${SILO_INCLUDE_DIR})
    target_link_libraries(f90_test PRIVATE silo)
    add_test(NAME f90_test COMMAND $<TARGET_FILE:f90_test>)
    add_test(NAME f90_test-hdf5 COMMAND $<TARGET_FILE:f90_test> DB_HDF5)
    set_tests_properties(f90_test-hdf5 PROPERTIES SKIP_RETURN_CODE 17)
endif()

if(SILO_ENABLE_PYTHON_MODULE)
    find_package(Python REQUIRED COMPONENTS Interpreter)
    add_test(NAME py_test COMMAND ${Python_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/py_test.py")
    set_tests_properties(py_test PROPERTIES ENVIRONMENT "PYTHONPATH=$<TARGET_FILE_DIR:SiloPy>")
endif()
