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

##############################################################################
#
# The CMake/CTest logic here is pretty ugly due to lack of familiarity with
# CMake/CTest prior to implementing this. Cleanup and re-organization is
# desparately needed. That aside, the basic structure/design is the following.
#
#   The main goal is to handle most cases by simply adding new C, Fortran or
#   C++ test code here without requiring any changes to CMakeLists.txt. This
#   works because a list of tests is gathered using a file glob. Then, this
#   list is iterated defining tests and associated compilation logic for
#   each. Two tests are created, one for the PDB driver and, when enabled,
#   one for the HDF5 driver. This is the simple and common case. Each test
#   involves a single source file which is compiled to produce the test.
#   The test is run two ways, one on PDB driver and one on HDF5 driver.
#   Sometimes both runs touch the same file name so HDF5 tests set to
#   DEPEND on PDB tests when both are active.
#
#   Things get more complicated for cases if...
#
#       * A test works only on HDF5 or only on PDB driver
#       * A test is defined only for certain build options
#       * A test needs special command-line arguments
#       * A test client is used to run many different ways
#         producing different files but with the same names
#       * A test requires the data produced by another test
#       * A test involves a shell or python script
#       * A test involves multiple source files
#
#   The C_TEST_SOURCES, CXX_TEST_SOURCES and F_TEST_SOURCES are the main
#   file globbed list of C, C++ and Fortran source files. There are some
#   companion lists...
#
#       HDF5_ONLY_SOURCES - for source files that work only on HDF5
#       PDB_ONLY_SOURCES - for source files that work only on PDB
#       PARALLEL_SOURCES - for tests that require MPI
#       SKIP_TEST_SOURCES - source files skipped from main loop for
#           one of a variety of reasons
#       SPECIAL_TEST_SOURCES - the key difference between this list and
#           SKIP list is that SPECIAL source files still get compiled.
#
#   CMake's RESOURCE_LOCK property is used in some cases but maybe it
#   should be used more liberally. We should probably use REQUIRED_FILES
#   more too. There may be some logic errors here where we wind up overriding
#   a test's properties.
#
# Notes on how to do valgrind testing...
#
# cmake -DCMAKE_INSTALL_PREFIX=`pwd`/my_install -DSILO_BUILD_FOR_BSD_LICENSE:BOOL=OFF -DSILO_ENABLE_HDF5:BOOL=ON -DSILO_ENABLE_FPZIP:BOOL=ON -DBUILD_TESTING:BOOL=ON -DSILO_HDF5_DIR:PATH=/mnt/nvme/mark/silo/hdf5-1.14.4/build/my_install -DCMAKE_BUILD_TYPE:STRING=Debug -S .. -B memcheck-build 
# cmake --build memcheck-build -j 8
# ctest --build memcheck-build -T memcheck --output-on-failure
# ls memcheck-build/
# ctest --build memcheck-build -T memcheck --output-on-failure -L compression
# ctest --build memcheck-build -T memcheck --output-on-failure -L Compression
# ctest --build memcheck-build -T memcheck --output-on-failure -I 122,122
# vi Testing/Temporary/MemoryChecker.122.log 
# cat Testing/Temporary/MemoryChecker.*
##############################################################################


##############################################################################
# In GitHub CI actions for windows, the dir from which the tests are actually
# run is D:/a/Silo/Silo/build/tests and each of the tests are compiled and
# built into D:/a/Silo/Silo/build/tests/bin/Release. The data file copies go
# into D:/a/Silo/Silo/build/all_tests
##############################################################################

###-------------------------------------------------------------------------------------
# Set the output dir for test executables
###-------------------------------------------------------------------------------------

if(WIN32)
    ##
    # For windows, the standard output dir (${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>) causes
    # problems with the script writing used when creating test-runners because the value
    # of $<CONFIG> isn't known until build time.  Instead use a non-configuration location
    # subdir directly off build-dir
    ##
    set(silo_test_output_dir ${Silo_BINARY_DIR}/all_tests/)
    file(MAKE_DIRECTORY ${silo_test_output_dir})
else()
    # runtime output dir is bin, so append bin to current binary dir
    set(silo_test_output_dir ${CMAKE_CURRENT_BINARY_DIR}/bin)
endif()

###-------------------------------------------------------------------------------------
# Copy dependencies needed for running tests.
###-------------------------------------------------------------------------------------
if(WIN32)
    add_custom_target(copy_test_deps ALL)
    set_target_properties(copy_test_deps PROPERTIES FOLDER testing)

    # silo
    add_custom_command(TARGET copy_test_deps POST_BUILD
         COMMAND ${CMAKE_COMMAND} -E copy_if_different
         $<TARGET_FILE:silo> ${silo_test_output_dir})

    # zlib
    if(ZLIB_FOUND AND ZLIB_DLL)
        add_custom_command(TARGET copy_test_deps POST_BUILD
             COMMAND ${CMAKE_COMMAND} -E copy_if_different
             ${ZLIB_DLL} ${silo_test_output_dir})
    endif()

    # szip
    if(SZIP_FOUND AND SZIP_DLL)
        add_custom_command(TARGET copy_test_deps POST_BUILD
             COMMAND ${CMAKE_COMMAND} -E copy_if_different
             ${SZIP_DLL} ${silo_test_output_dir})
    endif()

    # hdf5
    if(HDF5_FOUND AND HDF5_DLL)
            add_custom_command(TARGET copy_test_deps POST_BUILD
                 COMMAND ${CMAKE_COMMAND} -E copy_if_different
                 ${HDF5_DLL} ${silo_test_output_dir})
    endif()

    # python
    if(SILO_ENABLE_PYTHON_MODULE AND Python_EXECUTABLE)
      if(PYTHON_DLL)
            add_custom_command(TARGET copy_test_deps POST_BUILD
                 COMMAND ${CMAKE_COMMAND} -E copy_if_different
                 ${PYTHON_DLL} ${silo_test_output_dir})
      endif()
      if(TARGET SiloPy)
          add_custom_command(TARGET copy_test_deps POST_BUILD
                     COMMAND ${CMAKE_COMMAND} -E copy_if_different
                     $<TARGET_FILE:SiloPy> ${silo_test_output_dir})
      endif()
    endif()

endif()

###-------------------------------------------------------------------------------------
# Scan skip return value from std.c.
# Tests return this value if logic decides the config does not allow it to run properly.
###-------------------------------------------------------------------------------------
file(STRINGS "std.c" _skip_retval_line REGEX "SKIP_RETURN_VALUE")
string(REGEX MATCH "([0-9]+)" _silo_test_skip_retval "${_skip_retval_line}")

###-------------------------------------------------------------------------------------
# Copy data files needed for testing to test output dir.
###-------------------------------------------------------------------------------------

add_custom_target(copy_test_data ALL)
set_target_properties(copy_test_data PROPERTIES FOLDER testing)
add_custom_command(TARGET copy_test_data POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/pion0244.silo
    ${silo_test_output_dir})

add_custom_command(TARGET copy_test_data POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/z1plt.silo
    ${silo_test_output_dir})

add_custom_command(TARGET copy_test_data POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/silo_sh_utils.sh
    ${silo_test_output_dir})

if(SILO_ENABLE_PYTHON_MODULE AND Python_EXECUTABLE)
    # also need the python test files
    add_custom_command(TARGET copy_test_data POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/test_read.py
        ${silo_test_output_dir})
    add_custom_command(TARGET copy_test_data POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/test_write.py
        ${silo_test_output_dir})
    add_custom_command(TARGET copy_test_data POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/test_error.py
        ${silo_test_output_dir})
    add_custom_command(TARGET copy_test_data POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/onehex.py
        ${silo_test_output_dir})
    add_custom_command(TARGET copy_test_data POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/testonehex
        ${silo_test_output_dir})
    add_custom_command(TARGET copy_test_data POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/box_in_a_box.silo
        ${silo_test_output_dir})
    add_custom_command(TARGET copy_test_data POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/scale_quadvar.py
        ${silo_test_output_dir})
    add_custom_command(TARGET copy_test_data POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/test_scale_quadvar
        ${silo_test_output_dir})
endif()

set(SILO_TESTS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

if(SILO_ENABLE_HDF5 AND HDF5_FOUND)
    set(HDF5_ONLY_SOURCES
        compression.c
        grab.c
        largefile.c
        memfile_simple.c
        mk_nasf_h5.c
        partial_io.c
        readstuff.c
        testhdf5.c
        locking.c
    )
endif()

set(PDB_ONLY_SOURCES
    mk_nasf_pdb.c
    pdbtst.c
    testpdb.c
)

set(ROCKET_SOURCES
    rocket.cxx
    rocket_silo.cxx
)
if(VTK_DIR)
    find_package(VTK CONFIG COMPONENTS CommonCore CommonDataModel IOLegacy)
    if (VTK_FOUND)
        list(APPEND ROCKET_SOURCES rocket_vtk.cxx)
    endif()
endif()

set(PARALLEL_SOURCES
    bcastopen.c
    bcastopen_main.c
    multi_file_memfile.c
    pmpio_for_llrt.c
    pmpio_hdf5_test.c
    pmpio_silo_test_mesh.c
    resample.c
)

#
# General case for most tests
#
file(GLOB C_TEST_SOURCES *.c)
file(GLOB CXX_TEST_SOURCES *.cxx)
if(SILO_ENABLE_FORTRAN AND CMAKE_Fortran_COMPILER)
    file(GLOB F_TEST_SOURCES *.f*)
endif()

#
# These are "skipped" in the main loop that adds tests for
# for any source files found via globing for one of several
# possible reasons...
#
#     * Associated test is not working
#     * Source file is used only within other tests
#     * Special dependency logic is required meaning the test is
#       not skipped in the sense it is never run but just skipped
#       from the main (default) build loop and handled seperately
#
set(SKIP_TEST_SOURCES
    ${ROCKET_SOURCES}
    add_amr_mrgtree.c
    cpz1plt.c
    ioperf.c
    ioperf_pdb.c
    ioperf_silo.c
    ioperf_hdf5.c
    ioperf_sec2.c
    ioperf_stdio.c
    largefile_netcdf.c
    listtypes_main.c
    silo_overwrite_multi_example.c
    std.c
    testlib.c
)
if(WIN32)
    list(APPEND SKIP_TEST_SOURCES locking.c)
    list(APPEND SKIP_TEST_SOURCES rocket.cxx rocket_silo.cxx rocket_vtk.cxx)
endif()

if(NOT SILO_ENABLE_SHARED OR WIN32)
    list(APPEND SKIP_TEST_SOURCES pdbtst.c)
endif()
if(NOT VTK_DIR)
    list(APPEND SKIP_TEST_SOURCES rocket_vtk.cxx)
endif()

#
# This test has problems with ASAN.
# There are a handful of others that trigger leaks that we
# then have to disable detection of when ASAN is active.
#
if(SILO_BUILD_FOR_ASAN)
    list(APPEND SKIP_TEST_SOURCES simple.c)
endif()

#
# These are tests that require something other than simply running
# the test exeutable such as...
#
#    * Specific data files need to be read as part of the test
#    * Specific command-line arguments are needed to run the test
#
set(SPECIAL_TEST_SOURCES
    merge_block.c
    test_mat_compression.c
)

set(TESTLIB_SOURCES
    dir.c
    quad.c
)

set(JSON_C_TEST_SOURCES
    json.c
    json_curve.c
)

if(NOT SILO_ENABLE_JSON)
    list(APPEND SKIP_TEST_SOURCES ${JSON_C_TEST_SOURCES})
endif()

#
# Handle special logic for rocket test
#
if(SILO_ENABLE_SHARED AND NOT WIN32)

    add_executable(rocket rocket.cxx)
    target_link_libraries(rocket PRIVATE silo)
    set_target_properties(rocket PROPERTIES ENABLE_EXPORTS ON)
    if(WIN32)
        set_target_properties(rocket PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
    endif()
    if(CMAKE_DL_LIBS AND NOT APPLE)
        target_link_libraries(rocket PRIVATE ${CMAKE_DL_LIBS})
    endif()
    if(UNIX)
        target_link_libraries(rocket PRIVATE m)
    endif()

    # Silo plugin
    add_library(rocket_silo MODULE rocket_silo.cxx)
    target_link_libraries(rocket_silo PRIVATE silo rocket)
    if(SILO_ENABLE_HDF5 AND HDF5_FOUND)
        target_include_directories(rocket_silo PRIVATE ${HDF5_INCLUDE_DIRS})
    endif()
    target_include_directories(rocket_silo PRIVATE
        ${silo_build_include_dir}
        ${Silo_SOURCE_DIR}/src/silo
        ${SILO_TESTS_SOURCE_DIR})
    set_target_properties(rocket_silo PROPERTIES PREFIX "")

    # VTK plugin if requested
    if(VTK_FOUND)
        add_library(rocket_vtk MODULE rocket_vtk.cxx)
        target_link_libraries(rocket_vtk PRIVATE rocket VTK::CommonCore VTK::CommonDataModel VTK::IOLegacy)
        set_target_properties(rocket_vtk PROPERTIES PREFIX "")
        if(COMMAND vtk_module_autoinit)
            vtk_module_autoinit(TARGETS rocket_vtk MODULES VTK::CommonCore VTK::CommonDataModel VTK::IOLegacy)
        endif()
    endif()
    add_test(NAME rocket COMMAND $<TARGET_FILE:rocket>)
    if(SILO_ENABLE_HDF5 AND HDF5_FOUND)
        add_test(NAME rocket-hdf5 COMMAND $<TARGET_FILE:rocket> DB_HDF5)
        set_tests_properties(rocket-hdf5 PROPERTIES DEPENDS rocket)
    endif()
endif()

add_library(testlib_obj OBJECT testlib.c)
target_include_directories(testlib_obj PRIVATE
  ${silo_build_include_dir} ${Silo_SOURCE_DIR}/src/silo)

foreach(src IN LISTS C_TEST_SOURCES CXX_TEST_SOURCES F_TEST_SOURCES)

    cmake_path(GET src FILENAME srcFile)
    cmake_path(GET src STEM base)

    # ignore tests we're skipping
    if(srcFile IN_LIST SKIP_TEST_SOURCES)
        continue()
    endif()

    # Skip any parallel tests if we don't have parallel enabled
    if(NOT SILO_PARALLEL AND srcFile IN_LIST PARALLEL_SOURCES)
        continue()
    endif()

    add_executable(${base} ${src})
    add_dependencies(${base} silo)
    target_compile_definitions(${base} PRIVATE PDB_LITE)

    if(srcFile IN_LIST TESTLIB_SOURCES)
        target_sources(${base} PRIVATE $<TARGET_OBJECTS:testlib_obj>)
    endif()

    target_link_libraries(${base} $<TARGET_LINKER_FILE:silo>)
    if(UNIX)
        target_link_libraries(${base} dl m)
    endif()

    target_include_directories(${base}  PRIVATE
        ${silo_build_include_dir}
        ${Silo_SOURCE_DIR}/src/silo
        ${SILO_TESTS_SOURCE_DIR})

    set_target_properties(${base} PROPERTIES FOLDER testing/tests)

    if(src IN_LIST F_TEST_SOURCES)
      if(CMAKE_Fortran_COMPILER_ID MATCHES "GNU")
        if (CMAKE_Fortran_COMPILER_VERSION VERSION_GREATER_EQUAL 10)
          target_compile_options(${base} PRIVATE
            "$<$<COMPILE_LANGUAGE:Fortran>:-fallow-argument-mismatch>"
            )
        endif()
      endif()
    endif()

    if(SILO_ENABLE_HDF5 AND HDF5_FOUND)
        target_link_libraries(${base} ${HDF5_C_LIBRARIES})
        target_include_directories(${base} PRIVATE ${HDF5_INCLUDE_DIRS})
    endif()

    if(SILO_ENABLE_JSON AND JSONC_FOUND)
        target_link_libraries(${base} ${JSONC_LIBRARY})
        target_include_directories(${base} PRIVATE ${JSONC_INCLUDE_DIR})
    endif()

    #
    # Do not add default test cases for special tests. Those are
    # handled with specific logic below
    #
    if(srcFile IN_LIST SPECIAL_TEST_SOURCES)
        continue()
    endif()

    #
    # Add default tests for HDF5 or PDB drivers or BOTH 
    #
    if(srcFile IN_LIST HDF5_ONLY_SOURCES)
        if(SILO_ENABLE_HDF5 AND HDF5_FOUND)
            add_test(NAME ${base}-hdf5 COMMAND $<TARGET_FILE:${base}> DB_HDF5)
            set_tests_properties(${base}-hdf5 PROPERTIES LABELS "hdf5")
            set_tests_properties(${base}-hdf5 PROPERTIES SKIP_RETURN_CODE ${_silo_test_skip_retval})
            if(src IN_LIST F_TEST_SOURCES)
                set_tests_properties(${base}-hdf5 PROPERTIES LABELS "fortran")
            endif()
        endif()
    elseif(srcFile IN_LIST PDB_ONLY_SOURCES)
        add_test(NAME ${base} COMMAND $<TARGET_FILE:${base}>)
        set_tests_properties(${base} PROPERTIES LABELS "pdb")
        set_tests_properties(${base} PROPERTIES SKIP_RETURN_CODE ${_silo_test_skip_retval})
        if(src IN_LIST F_TEST_SOURCES)
            set_tests_properties(${base} PROPERTIES LABELS "fortran")
        endif()
    else()
        add_test(NAME ${base} COMMAND $<TARGET_FILE:${base}>)
        set_tests_properties(${base} PROPERTIES LABELS "pdb")
        set_tests_properties(${base} PROPERTIES SKIP_RETURN_CODE ${_silo_test_skip_retval})
        if(SILO_PACT_DIR)
            add_test(NAME ${base}-pdbp COMMAND $<TARGET_FILE:${base}> DB_PDBP)
            set_tests_properties(${base}-pdbp PROPERTIES LABELS "pdb-proper")
            set_tests_properties(${base}-pdbp PROPERTIES SKIP_RETURN_CODE ${_silo_test_skip_retval})
            if(src IN_LIST F_TEST_SOURCES)
                set_tests_properties(${base}-pdbp PROPERTIES LABELS "fortran")
            endif()
        endif()
        if(src IN_LIST F_TEST_SOURCES)
            set_tests_properties(${base} PROPERTIES LABELS "fortran")
        endif()
        if(SILO_ENABLE_HDF5 AND HDF5_FOUND)
            add_test(NAME ${base}-hdf5 COMMAND $<TARGET_FILE:${base}> DB_HDF5)
            set_tests_properties(${base}-hdf5 PROPERTIES DEPENDS ${base})
            set_tests_properties(${base}-hdf5 PROPERTIES LABELS "hdf5")
            set_tests_properties(${base}-hdf5 PROPERTIES SKIP_RETURN_CODE ${_silo_test_skip_retval})
            if(src IN_LIST F_TEST_SOURCES)
                set_tests_properties(${base}-hdf5 PROPERTIES LABELS "fortran")
            endif()
        endif()
    endif()


endforeach()

target_sources(listtypes PRIVATE listtypes_main.c listtypes.c)
set_tests_properties(listtypes PROPERTIES DEPENDS ucd REQUIRED_FILES ucd.pdb)
set_tests_properties(listtypes PROPERTIES LABELS "pdb")
if(SILO_ENABLE_HDF5 AND HDF5_FOUND)
    set_tests_properties(listtypes-hdf5 PROPERTIES DEPENDS ucd-hdf5 REQUIRED_FILES ucd.h5)
    set_tests_properties(listtypes-hdf5 PROPERTIES LABELS "hdf5")
endif()

if(SILO_ENABLE_JSON AND JSONC_FOUND)
    add_test(NAME onehexA COMMAND $<TARGET_FILE:onehex> fname=onehex.pdb)
    set_tests_properties(json PROPERTIES DEPENDS onehexA REQUIRED_FILES onehex.pdb)
    set_tests_properties(json PROPERTIES LABELS "pdb;json")
    set_tests_properties(json_curve PROPERTIES DEPENDS testall REQUIRED_FILES curve.pdb)
    set_tests_properties(json_curve PROPERTIES LABELS "pdb;json")
    if(SILO_ENABLE_HDF5 AND HDF5_FOUND)
        add_test(NAME onehexB COMMAND $<TARGET_FILE:onehex> DB_HDF5 fname=onehex.h5)
        set_tests_properties(json-hdf5 PROPERTIES DEPENDS onehexB REQUIRED_FILES onehex.h5)
        set_tests_properties(json-hdf5 PROPERTIES LABELS "hdf5;json")
        set_tests_properties(json_curve-hdf5 PROPERTIES DEPENDS testall-hdf5 REQUIRED_FILES curve.h5)
        set_tests_properties(json_curve-hdf5 PROPERTIES LABELS "hdf5;json")
    endif()
    set_tests_properties(onehexA;json PROPERTIES RESOURCE_LOCK onehex.pdb)
    set_tests_properties(onehexB;json-hdf5 PROPERTIES RESOURCE_LOCK onehex.h5)
    set_tests_properties(json_curve;json_curve-hdf5 PROPERTIES RESOURCE_LOCK "curve.pdb;curve.h5")
endif()

#
# Basic (simple array) compression tests
# All tests here involve a pair of executions of the same test executable.
# First, data is compressed during write. Then, data is uncompressed during read.
# Because all tests write the same named file, we need to lock that resource.
#
if(SILO_ENABLE_HDF5 AND HDF5_FOUND)
    add_test(NAME onehex-split COMMAND $<TARGET_FILE:onehex> split)
    add_test(NAME onehex-custom COMMAND $<TARGET_FILE:onehex> custom)
    set_tests_properties(onehex;onehex-hdf5;onehex-split;onehex-custom PROPERTIES
        RESOURCE_LOCK onehex.silo
        LABELS "hdf5")
    set(COMPRESSION_TESTS "compression-hdf5")
    if(ZLIB_FOUND)
        add_test(NAME compression-generic COMMAND $<TARGET_FILE:compression> generic)
        add_test(NAME compression-gzip COMMAND $<TARGET_FILE:compression> gzip)
        add_test(NAME compression-gzip-read COMMAND $<TARGET_FILE:compression> readonly)
        set_tests_properties(compression-gzip-read PROPERTIES DEPENDS "compression-gzip")
        list(APPEND COMPRESSION_TESTS compression-gzip compression-gzip-read compression-generic)
    endif()
    if(HDF5_ENABLE_SZIP_SUPPORT AND SZIP_FOUND)
        add_test(NAME compression-szip COMMAND $<TARGET_FILE:compression> szip)
        add_test(NAME compression-szip-read COMMAND $<TARGET_FILE:compression> readonly)
        set_tests_properties(compression-szip-read PROPERTIES DEPENDS "compression-szip")
        list(APPEND COMPRESSION_TESTS compression-szip compression-szip-read)
    endif()
    if(SILO_ENABLE_FPZIP)
        add_test(NAME compression-fpzip COMMAND $<TARGET_FILE:compression> fpzip)
        add_test(NAME compression-fpzip-read COMMAND $<TARGET_FILE:compression> readonly)
        set_tests_properties(compression-fpzip-read PROPERTIES DEPENDS "compression-fpzip")
        list(APPEND COMPRESSION_TESTS compression-fpzip compression-fpzip-read)

        add_test(NAME compression-lossy3 COMMAND $<TARGET_FILE:compression> lossy3)
        add_test(NAME compression-lossy3-read COMMAND $<TARGET_FILE:compression> lossy3 readonly)
        set_tests_properties(compression-lossy3-read PROPERTIES DEPENDS "compression-lossy3")
        list(APPEND COMPRESSION_TESTS compression-lossy3 compression-lossy3-read)

        add_test(NAME compression-minratio-fail COMMAND $<TARGET_FILE:compression> minratio1000)
        set_property(TEST compression-minratio-fail PROPERTY WILL_FAIL TRUE)
        add_test(NAME compression-minratio-fallback COMMAND $<TARGET_FILE:compression> minratio1001)
        list(APPEND COMPRESSION_TESTS compression-minratio-fail compression-minratio-fallback)
    endif()
    if(SILO_ENABLE_ZFP)
        add_test(NAME compression-zfp COMMAND $<TARGET_FILE:compression> zfp)
        add_test(NAME compression-zfp-read COMMAND $<TARGET_FILE:compression> zfp readonly)
        set_tests_properties(compression-zfp-read PROPERTIES DEPENDS "compression-zfp")
        list(APPEND COMPRESSION_TESTS compression-zfp compression-zfp-read)
    endif()
    if(COMPRESSION_TESTS)
        set_tests_properties(${COMPRESSION_TESTS} PROPERTIES RESOURCE_LOCK compression.h5)
        set_tests_properties(${COMPRESSION_TESTS} PROPERTIES LABELS "compression")
    endif()
endif()

#
# TestReadMask has a number of dependencies
#
add_test(NAME testall-small COMMAND $<TARGET_FILE:testall> -small)
set_tests_properties(testall-small PROPERTIES DEPENDS "testall")
set_tests_properties(TestReadMask PROPERTIES DEPENDS "testall-small;point;extface"
    REQUIRED_FILES "rect2d.pdb;point2d.pdb;globe.pdb")
set_tests_properties(TestReadMask PROPERTIES LABELS "pdb")
if(SILO_ENABLE_HDF5 AND HDF5_FOUND)
    add_test(NAME testall-small-hdf5 COMMAND $<TARGET_FILE:testall> -small DB_HDF5)
    set_tests_properties(testall-small-hdf5 PROPERTIES DEPENDS "testall-hdf5")
    set_tests_properties(TestReadMask-hdf5 PROPERTIES DEPENDS "testall-small-hdf5;point-hdf5;extface-hdf5"
        REQUIRED_FILES "rect2d.h5;point2d.h5;globe.h5")
    set_tests_properties(memfile_simple-hdf5 PROPERTIES DEPENDS "testall-small-hdf5" REQUIRED_FILES "rect2d.h5")
    set_tests_properties(TestReadMask-hdf5 PROPERTIES LABELS "hdf5")
endif()

#
# multi_test is a work-horse test that tests multi-block for 2 and 3 dimensions,
# several different mesh types, materials, etc as well as a number of different
# scenarios involving interrupted writes, early closes on a file, etc. It also
# can test readback of what was written in a separate invokation.
#
set(MULTI_BLOCK_TESTS)
add_test(NAME multi_test-read COMMAND $<TARGET_FILE:multi_test> testread)
set_tests_properties(multi_test-read PROPERTIES DEPENDS multi_test)
add_test(NAME multi_test-badread COMMAND $<TARGET_FILE:multi_test> testbadread)
set_tests_properties(multi_test-badread PROPERTIES DEPENDS multi_test)
add_test(NAME multi_test-earlyclose COMMAND $<TARGET_FILE:multi_test> earlyclose)
set_tests_properties(multi_test-earlyclose PROPERTIES DEPENDS multi_test)
list(APPEND MULTI_BLOCK_TESTS multi_test multi_test-read multi_test-badread multi_test-earlyclose)
if(SILO_ENABLE_HDF5 AND HDF5_FOUND)
    add_test(NAME dir-hdf5-xcopy-input COMMAND "${CMAKE_COMMAND}" -E copy
        ${CMAKE_CURRENT_BINARY_DIR}/multi_ucd3d.pdb ${CMAKE_CURRENT_BINARY_DIR}/multi_ucd3d_copy.pdb)
    set_tests_properties(dir-hdf5-xcopy-input PROPERTIES DEPENDS multi_test)
    add_test(NAME dir-xcopy-input COMMAND "${CMAKE_COMMAND}" -E copy
        ${CMAKE_CURRENT_BINARY_DIR}/multi_ucd3d.h5 ${CMAKE_CURRENT_BINARY_DIR}/multi_ucd3d_copy.h5)
    set_tests_properties(dir-xcopy-input PROPERTIES DEPENDS multi_test-hdf5)
    set_tests_properties(multi_test-earlyclose PROPERTIES DEPENDS dir-xcopy-input)
    set_tests_properties(readstuff-hdf5 PROPERTIES DEPENDS "dir-xcopy-input;dir-hdf5-xcopy-input")
    add_test(NAME multi_test-read-hdf5 COMMAND $<TARGET_FILE:multi_test> testread DB_HDF5)
    set_tests_properties(multi_test-read-hdf5 PROPERTIES DEPENDS "dir-xcopy-input;dir-hdf5-xcopy-input")
    add_test(NAME multi_test-badread-hdf5 COMMAND $<TARGET_FILE:multi_test> testbadread DB_HDF5)
    set_tests_properties(multi_test-badread-hdf5 PROPERTIES DEPENDS "dir-xcopy-input;dir-hdf5-xcopy-input")
    add_test(NAME multi_test-earlyclose-hdf5 COMMAND $<TARGET_FILE:multi_test> earlyclose DB_HDF5)
    add_test(NAME multi_test-flush COMMAND $<TARGET_FILE:multi_test> testflush DB_HDF5)
    add_test(NAME multi_test-libver COMMAND $<TARGET_FILE:multi_test> libver DB_HDF5)
    set_tests_properties(multi_test-libver PROPERTIES DEPENDS "dir-xcopy-input;dir-hdf5-xcopy-input")
    add_test(NAME multi_test-libver-read COMMAND $<TARGET_FILE:multi_test> testread DB_HDF5)
    set_tests_properties(multi_test-libver-read PROPERTIES DEPENDS multi_test-libver)
    if(SILO_ENABLE_HZIP)
        add_test(NAME multi_test-hzip COMMAND $<TARGET_FILE:multi_test> hzip DB_HDF5)
        set_tests_properties(multi_test-hzip PROPERTIES DEPENDS "dir-xcopy-input;dir-hdf5-xcopy-input")
        add_test(NAME multi_test-hzip-read COMMAND $<TARGET_FILE:multi_test> testread DB_HDF5)
        set_tests_properties(multi_test-hzip-read PROPERTIES DEPENDS multi_test-hzip)
        list(APPEND MULTI_BLOCK_TESTS multi_test-hzip multi_test-hzip-read)

        add_custom_target(testhzip ALL)
        add_custom_command(TARGET testhzip POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/testhzip
            ${silo_test_output_dir})
        add_test(NAME testhzip
            COMMAND ${CMAKE_COMMAND} -E env
            PATH=${_sh_path}
            ASAN_OPTIONS=detect_leaks=0
            "${silo_test_output_dir}/testhzip")

    endif()
    list(APPEND MULTI_BLOCK_TESTS multi_test-hdf5 readstuff-hdf5 multi_test-read-hdf5 multi_test-badread-hdf5
        multi_test-earlyclose-hdf5 multi_test-flush multi_test-libver multi_test-libver-read)
endif()
if(MULTI_BLOCK_TESTS)
    set(_multi_test_lock "multi_test_data_files")
    set_tests_properties(${MULTI_BLOCK_TESTS} PROPERTIES RESOURCE_LOCK ${_multi_test_lock})
    set_tests_properties(${MULTI_BLOCK_TESTS} PROPERTIES LABELS "multi")
endif()

#
# The dir.c test ues multi_ucd3d.[pdb,h5] to do some things but it corrupts the content.
# So, we create a dependency of dir test on a COPY of multi_ucd3d_copy.[pdb,h5]
#
if(SILO_ENABLE_HDF5 AND HDF5_FOUND)
    add_test(NAME dir-xcopy COMMAND $<TARGET_FILE:dir> xcopy)
    set_tests_properties(dir-xcopy PROPERTIES DEPENDS dir-xcopy-input)

    add_test(NAME dir-hdf5-xcopy COMMAND $<TARGET_FILE:dir> xcopy DB_HDF5)
    set_tests_properties(dir-hdf5-xcopy PROPERTIES DEPENDS dir-hdf5-xcopy-input)
endif()

#
# The multi_file test tests storing Silo database across multiple files
# using nameschemes or not, with some blocks empty or not and with blocks in subdirs or not.
# The multidir option for multi_file requires MPI.
#
set(MULTI_FILE_TESTS)
add_test(NAME multi_file-multidir-usens COMMAND $<TARGET_FILE:multi_file> multidir use-ns)
list(APPEND MULTI_FILE_TESTS multi_file-multidir-usens)
if(SILO_ENABLE_HDF5 AND HDF5_FOUND)
    add_test(NAME multi_file-multidir-usens-hdf5 COMMAND $<TARGET_FILE:multi_file> multidir use-ns DB_HDF5)
    list(APPEND MULTI_FILE_TESTS multi_file-multidir-usens-hdf5)
endif()
# These tests all take way too long under ASAN 
if(NOT SILO_BUILD_FOR_ASAN)
    add_test(NAME multi_file-usens COMMAND $<TARGET_FILE:multi_file> use-ns)
    add_test(NAME multi_file-evalns COMMAND $<TARGET_FILE:multi_file> eval-ns)
    set_tests_properties(multi_file-evalns PROPERTIES DEPENDS multi_file-usens)
    add_test(NAME multi_file-usens-empties COMMAND $<TARGET_FILE:multi_file> use-ns empties)
    add_test(NAME multi_file-multidir COMMAND $<TARGET_FILE:multi_file> multidir)
    list(APPEND MULTI_FILE_TESTS multi_file multi_file-usens multi_file-evalns multi_file-usens-empties multi_file-multidir)
    if(SILO_ENABLE_HDF5 AND HDF5_FOUND)
        add_test(NAME multi_file-usens-hdf5 COMMAND $<TARGET_FILE:multi_file> use-ns DB_HDF5)
        add_test(NAME multi_file-evalns-hdf5 COMMAND $<TARGET_FILE:multi_file> eval-ns DB_HDF5)
        set_tests_properties(multi_file-evalns-hdf5 PROPERTIES DEPENDS multi_file-usens-hdf5)
        add_test(NAME multi_file-usens-empties-hdf5 COMMAND $<TARGET_FILE:multi_file> use-ns empties DB_HDF5)
        add_test(NAME multi_file-multidir-hdf5 COMMAND $<TARGET_FILE:multi_file> multidir DB_HDF5)
        list(APPEND MULTI_FILE_TESTS multi_file-hdf5 multi_file-usens-hdf5 multi_file-evalns-hdf5 multi_file-usens-empties-hdf5 multi_file-multidir-hdf5)
    endif()
endif()
set_tests_properties(${MULTI_FILE_TESTS} PROPERTIES RESOURCE_LOCK "multi_file_tests")
set_tests_properties(${MULTI_FILE_TESTS} PROPERTIES LABELS "multi")

#
# Partial I/O tests...mainly for Silo functionality to tease a tiny bit of raw data efficiently
# and quickly from large datasets.
#
if(SILO_ENABLE_HDF5 AND HDF5_FOUND)
    add_test(NAME partial-io-setup COMMAND $<TARGET_FILE:testall> DB_HDF5)
    set_tests_properties(partial-io-setup PROPERTIES DEPENDS testall-hdf5)
    add_test(NAME partial-io-rect3d COMMAND $<TARGET_FILE:partial_io> DB_HDF5 point-query rect3d.h5 d 10 [0,0,0,1,1,1,2,2,2,3,3,3,4,4,4,5,5,5,6,6,6,7,7,7,8,8,8,9,9,9])
    set_tests_properties(partial-io-rect3d PROPERTIES DEPENDS "partial-io-setup" RESOURCE_LOCK "partial.h5;rect3d.h5")
    add_test(NAME partial-io-ucd3d COMMAND $<TARGET_FILE:partial_io> DB_HDF5 point-query ucd3d.h5 d 5 [0,26,234,1482,2574])
    set_tests_properties(partial-io-ucd3d PROPERTIES DEPENDS "partial-io-setup" RESOURCE_LOCK "partial.h5;ucd3d.h5")
endif()

if(NOT WIN32)
    add_custom_target(testdtypes ALL)
    add_custom_command(TARGET testdtypes POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/testdtypes
        ${silo_test_output_dir})
    add_test(NAME testdtypes
        COMMAND ${CMAKE_COMMAND} -E env ASAN_OPTIONS=detect_leaks=0
        "${silo_test_output_dir}/testdtypes")
    set_tests_properties(testdtypes PROPERTIES DEPENDS specmix)
    if(SILO_ENABLE_HDF5 AND HDF5_FOUND)
        add_test(NAME testdtypes-hdf5
            COMMAND ${CMAKE_COMMAND} -E env ASAN_OPTIONS=detect_leaks=0
            "${silo_test_output_dir}/testdtypes" DB_HDF5)
        set_tests_properties(testdtypes PROPERTIES DEPENDS specmix-hdf5)
    endif()
endif()

if(SILO_ENABLE_SILOCK AND NOT WIN32)
    add_custom_target(testsilock ALL)
    add_custom_command(TARGET testsilock POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/testsilock
        ${silo_test_output_dir})
    add_test(NAME testsilock
        COMMAND ${CMAKE_COMMAND} -E env
        "${silo_test_output_dir}/testsilock")
    set_tests_properties(onehex PROPERTIES DEPENDS testsilock RESOURCE_LOCK onehex.silo)
    if(SILO_ENABLE_HDF5 AND HDF5_FOUND)
        add_test(NAME testsilock-hdf5
            COMMAND ${CMAKE_COMMAND} -E env
            "${silo_test_output_dir}/testsilock" DB_HDF5)
        set_tests_properties(onehex-hdf5 PROPERTIES DEPENDS testsilock-hdf5)
        set_tests_properties(testsilock-hdf5 PROPERTIES DEPENDS testsilock RESOURCE_LOCK onehex.silo)
    endif()
endif()

if(SILO_ENABLE_HDF5 AND HDF5_FOUND)
    add_custom_target(checksums ALL)
    add_custom_command(TARGET checksums POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/checksums
        ${silo_test_output_dir})
    add_test(NAME checksums
        COMMAND ${CMAKE_COMMAND} -E env
        PATH=${_sh_path}
        ASAN_OPTIONS=detect_leaks=0
        "${silo_test_output_dir}/checksums")
    set_tests_properties(checksums PROPERTIES DEPENDS multi_test-hdf5)
    set_tests_properties(checksums PROPERTIES SKIP_RETURN_CODE ${_silo_test_skip_retval})
    # There is a bug in HDF5-1.14.6 that causes this test to fail
    if(HDF5_VERSION VERSION_EQUAL "1.14.6")
        set_tests_properties(checksums PROPERTIES DISABLED TRUE)
    endif()
endif()

#
# Python tests
#
if(SILO_ENABLE_PYTHON_MODULE AND Python_EXECUTABLE)
    add_test(NAME test-write-py COMMAND ${Python_EXECUTABLE} "${silo_test_output_dir}/test_write.py")
    set_tests_properties(test-write-py PROPERTIES ENVIRONMENT "PYTHONPATH=$<TARGET_FILE_DIR:SiloPy>${_envvar_path_sep}$ENV{PYTHONPATH};LD_PRELOAD=${SILO_LIBASAN_PATH}:${SILO_LIBUBSAN_PATH}")
    set_tests_properties(test-write-py PROPERTIES DEPENDS sami REQUIRED_FILES sami.silo)
    set_tests_properties(test-write-py PROPERTIES RESOURCE_LOCK "sami.silo")
    set_tests_properties(test-write-py PROPERTIES LABELS "python")

    if(SILO_ENABLE_HDF5 AND HDF5_FOUND)
        add_test(NAME test-write-py-hdf5 COMMAND ${Python_EXECUTABLE} "${silo_test_output_dir}/test_write.py")
        set_tests_properties(test-write-py-hdf5 PROPERTIES ENVIRONMENT "PYTHONPATH=$<TARGET_FILE_DIR:SiloPy>${_envvar_path_sep}$ENV{PYTHONPATH};LD_PRELOAD=${SILO_LIBASAN_PATH}:${SILO_LIBUBSAN_PATH}")
        set_tests_properties(test-write-py-hdf5 PROPERTIES DEPENDS sami-hdf5 REQUIRED_FILES sami.silo)
        set_tests_properties(test-write-py-hdf5 PROPERTIES RESOURCE_LOCK "sami.silo")
        set_tests_properties(test-write-py-hdf5 PROPERTIES LABELS "python;hdf5")
        add_test(NAME test-scale-quadvar-hdf5 COMMAND ${CMAKE_COMMAND} -E env "${silo_test_output_dir}/test_scale_quadvar")
        set_tests_properties(test-scale-quadvar-hdf5 PROPERTIES ENVIRONMENT "PYTHONPATH=$<TARGET_FILE_DIR:SiloPy>${_envvar_path_sep}$ENV{PYTHONPATH};LD_PRELOAD=${SILO_LIBASAN_PATH}:${SILO_LIBUBSAN_PATH};ASAN_OPTIONS=detect_leaks=0")
        set_tests_properties(test-scale-quadvar-hdf5 PROPERTIES LABELS "python;hdf5")
    endif()

    add_test(NAME multi_test-reset COMMAND $<TARGET_FILE:multi_test>)
    add_test(NAME test-read-py COMMAND ${Python_EXECUTABLE} "${silo_test_output_dir}/test_read.py")
    add_test(NAME test-error-py COMMAND ${Python_EXECUTABLE} "${silo_test_output_dir}/test_error.py")
    set_tests_properties(test-read-py PROPERTIES ENVIRONMENT "PYTHONPATH=$<TARGET_FILE_DIR:SiloPy>${_envvar_path_sep}$ENV{PYTHONPATH};LD_PRELOAD=${SILO_LIBASAN_PATH}:${SILO_LIBUBSAN_PATH}")
    set_tests_properties(test-error-py PROPERTIES ENVIRONMENT "PYTHONPATH=$<TARGET_FILE_DIR:SiloPy>${_envvar_path_sep}$ENV{PYTHONPATH};LD_PRELOAD=${SILO_LIBASAN_PATH}:${SILO_LIBUBSAN_PATH}")
    set_tests_properties(multi_test-reset PROPERTIES DEPENDS multi_test-earlyclose LABELS "python")
    set_tests_properties(test-read-py PROPERTIES DEPENDS multi_test-reset LABELS "python")
    set_tests_properties(test-error-py PROPERTIES DEPENDS multi_test-reset LABELS "python")
    set_tests_properties(multi_test-reset test-read-py PROPERTIES RESOURCE_LOCK ${_multi_test_lock})
    set_tests_properties(multi_test-reset test-read-py PROPERTIES REQUIRED_FILES "multi_ucd3d.pdb;multi_rect3d.pdb")

    if(SILO_ENABLE_HDF5 AND HDF5_FOUND)
       add_test(NAME multi_test-reset-hdf5 COMMAND $<TARGET_FILE:multi_test> hdf-friendly DB_HDF5)
       add_test(NAME test-read-py-hdf5 COMMAND ${Python_EXECUTABLE} "${silo_test_output_dir}/test_read.py" DB_HDF5)
       add_test(NAME test-error-py-hdf5 COMMAND ${Python_EXECUTABLE} "${silo_test_output_dir}/test_error.py" DB_HDF5)
       set_tests_properties(test-read-py-hdf5 PROPERTIES ENVIRONMENT "PYTHONPATH=$<TARGET_FILE_DIR:SiloPy>${_envvar_path_sep}$ENV{PYTHONPATH};LD_PRELOAD=${SILO_LIBASAN_PATH}:${SILO_LIBUBSAN_PATH}")
       set_tests_properties(test-error-py-hdf5 PROPERTIES ENVIRONMENT "PYTHONPATH=$<TARGET_FILE_DIR:SiloPy>${_envvar_path_sep}$ENV{PYTHONPATH};LD_PRELOAD=${SILO_LIBASAN_PATH}:${SILO_LIBUBSAN_PATH}")
       set_tests_properties(multi_test-reset-hdf5 PROPERTIES DEPENDS multi_test-earlyclose-hdf5 LABELS "python;hdf5")
       set_tests_properties(test-read-py-hdf5 PROPERTIES DEPENDS multi_test-reset-hdf5 LABELS "python;hdf5")
       set_tests_properties(test-error-py-hdf5 PROPERTIES DEPENDS multi_test-reset-hdf5 LABELS "python;hdf5")
       set_tests_properties(multi_test-reset-hdf5 test-read-py-hdf5 PROPERTIES RESOURCE_LOCK ${_multi_test_lock})
       set_tests_properties(multi_test-reset-hdf5 test-read-py-hdf5 PROPERTIES REQUIRED_FILES "multi_ucd3d.h5;multi_rect3d.h5")
    endif()

    add_test(NAME testonehex COMMAND ${CMAKE_COMMAND} -E env "${silo_test_output_dir}/testonehex" DB_PDB)
    set_tests_properties(testonehex PROPERTIES DEPENDS onehex)
    set_tests_properties(testonehex PROPERTIES ENVIRONMENT "PYTHONPATH=$<TARGET_FILE_DIR:SiloPy>${_envvar_path_sep}$ENV{PYTHONPATH};LD_PRELOAD=${SILO_LIBASAN_PATH}:${SILO_LIBUBSAN_PATH};ASAN_OPTIONS=detect_leaks=0")
    set_tests_properties(testonehex PROPERTIES LABELS "python")
    set_tests_properties(testonehex onehex PROPERTIES RESOURCE_LOCK "onehex.silo")

    if(SILO_ENABLE_HDF5 AND HDF5_FOUND)
        add_test(NAME testonehex-hdf5 COMMAND ${CMAKE_COMMAND} -E env "${silo_test_output_dir}/testonehex" DB_HDF5)
        set_tests_properties(testonehex-hdf5 PROPERTIES DEPENDS "onehex-hdf5;testonehex")
        set_tests_properties(testonehex-hdf5 PROPERTIES ENVIRONMENT "PYTHONPATH=$<TARGET_FILE_DIR:SiloPy>${_envvar_path_sep}$ENV{PYTHONPATH};LD_PRELOAD=${SILO_LIBASAN_PATH}:${SILO_LIBUBSAN_PATH};ASAN_OPTIONS=detect_leaks=0")
        set_tests_properties(testonehex-hdf5 PROPERTIES LABELS "python;hdf5")
        set_tests_properties(testonehex-hdf5 onehex-hdf5 PROPERTIES RESOURCE_LOCK "onehex.silo")
    endif()

endif()

#
# test_mat_compression tests
#
add_test(NAME test_mat_compression-basic COMMAND $<TARGET_FILE:test_mat_compression> in=specmix_quad.pdb:Material out=tmc-basic.pdb:Material)
set_tests_properties(test_mat_compression-basic PROPERTIES DEPENDS specmix RESOURCE_LOCK "specmix_quad.pdb" LABELS "pdb")
add_test(NAME test_mat_compression-dense COMMAND $<TARGET_FILE:test_mat_compression> in=specmix_quad.pdb:Material out=tmc-dense.pdb:Material dense=1)
set_tests_properties(test_mat_compression-dense PROPERTIES DEPENDS specmix RESOURCE_LOCK "specmix_quad.pdb" LABELS "pdb")
if(SILO_ENABLE_HDF5 AND HDF5_FOUND)
    add_test(NAME test_mat_compression-basic-hdf5 COMMAND $<TARGET_FILE:test_mat_compression> in=specmix_quad.h5:Material out=tmc-basic.h5:Material)
    set_tests_properties(test_mat_compression-basic-hdf5 PROPERTIES DEPENDS specmix RESOURCE_LOCK "specmix_quad.h5" LABELS "hdf5")
    add_test(NAME test_mat_compression-dense-hdf5 COMMAND $<TARGET_FILE:test_mat_compression> in=specmix_quad.h5:Material out=tmc-dense.h5:Material dense=1)
    set_tests_properties(test_mat_compression-dense-hdf5 PROPERTIES DEPENDS specmix RESOURCE_LOCK "specmix_quad.h5" LABELS "hdf5")
    add_test(NAME test_mat_compression-gzip-hdf5 COMMAND $<TARGET_FILE:test_mat_compression> in=specmix_quad.h5:Material out=tmc-gzip.h5:Material dense=1 compress="METHOD=GZIP LEVEL=3")
    set_tests_properties(test_mat_compression-gzip-hdf5 PROPERTIES DEPENDS specmix RESOURCE_LOCK "specmix_quad.h5" LABELS "hdf5;compression")
    if(SILO_ENABLE_ZFP)
        add_test(NAME test_mat_compression-zfp-hdf5 COMMAND $<TARGET_FILE:test_mat_compression> in=specmix_quad.h5:Material out=tmc-zfp.h5:Material dense=1 compress="METHOD=ZFP ACCURACY=0.001")
        set_tests_properties(test_mat_compression-zfp-hdf5 PROPERTIES DEPENDS specmix RESOURCE_LOCK "specmix_quad.h5" LABELS "hdf5;compression")
    endif()
endif()
