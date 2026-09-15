#!/bin/sh

# Copyright (C) 1994-2016 Lawrence Livermore National Security, LLC.
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
# Contract  No.   DE-AC52-07NA27344 with  the  DOE.  Neither the  United
# States Government  nor Lawrence  Livermore National Security,  LLC nor
# any of  their employees,  makes any warranty,  express or  implied, or
# assumes   any   liability   or   responsibility  for   the   accuracy,
# completeness, or usefulness of any information, apparatus, product, or
# process  disclosed, or  represents  that its  use  would not  infringe
# privately-owned   rights.  Any  reference   herein  to   any  specific
# commercial products,  process, or  services by trade  name, trademark,
# manufacturer or otherwise does not necessarily constitute or imply its
# endorsement,  recommendation,   or  favoring  by   the  United  States
# Government or Lawrence Livermore National Security, LLC. The views and
# opinions  of authors  expressed  herein do  not  necessarily state  or
# reflect those  of the United  States Government or  Lawrence Livermore
# National  Security, LLC,  and shall  not  be used  for advertising  or
# product endorsement purposes.

# -----------------------------------------------------------------------------
# Test Silo's ability to detect malformed objects it reads from files and
# preventing bad things to happen.
#
# Mark C. Miller, Wed Sep  2 14:37:44 PDT 2026
# -----------------------------------------------------------------------------
#
# Find dir where this script lives and source the shell utils script there.
#
# dirname -- "$0" gets the script directory even if this script is run via a
#     relative path like ../../foo/bar/gorfo.sh.
# The CDPATH= nulls that env. variable and prevents cd from printing anything
#     if CDPATH is set in the environment.
# pwd gives the absolute path after the cd has occurred. This all happens in
#     a subshell so the cwd of the current script is unchanged.
#
script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. $script_dir/silo_sh_utils.sh

#
# Decide which file extesion to look for
#
ext="pdb"
[ "$1" = "DB_HDF5" ] && ext="h5"

#
# Ensure the multi_test executable is available
#
multi_test=$(find_file -x tests/bin/multi_test tests/multi_test ./multi_test ../../multi_test)
[ $? -eq 0 ] || exit 1

#
# Ensure we have Silo's 'browser' tool available
#
browser=$(find_file -x bin/browser tools/browser/browser tools/browser/.libs/browser ../tools/browser/browser ../../../tools/browser/browser)
[ $? -eq 0 ] || exit 1

#
# Get error code for E_MALFORMED from silo header
#
silo_header=$(find_file -r ./src/silo/silo.h.in ../src/silo/silo.h.in ../../src/silo/silo.h.in)
e_malformed_code=$(grep E_MALFORMED $silo_header | tr -s ' ' | cut -d' ' -f3)
[ $? -eq 0 ] || exit 1

#
# Find text executable to generate data
#
all_objs=$(find_file -x all_silo_objects tests/bin/all_silo_objects)
[ $? -eq 0 ] || exit 1
$all_objs $1
[ $? -eq 0 ] || exit 1
all_objs_file=$(find_file -r all_objects.$ext tests/all_objects.$ext)
[ $? -eq 0 ] || exit 1

#
# Corrupt one object at a time with browser's low-level write mode, then
# confirm that the corresponding high-level DBGetXxx path reports
# E_MALFORMED.
#
check_malformed()
{
    dirname=$1
    objname=$2
    cname_assign=$3

    cp $all_objs_file malformed.$ext || return 1
    $browser -q -W -l 1 -e "cd $dirname" -e "$objname.$cname_assign" malformed.$ext
    [ $? -eq 0 ] || return 1

    $browser -q --proper-exit-code -e "cd $dirname" -e "$objname" malformed.$ext
    [ $? -eq $e_malformed_code ] || {
        echo "expected E_MALFORMED for $dirname/$objname after $cname_assign" >&2
        return 1
    }
}

# Existing material coverage.
check_malformed material_objects material 'ndims=5' || exit 1
check_malformed material_objects material 'nmat=7' || exit 1
check_malformed material_objects material_mix 'mixlen=4495' || exit 1
check_malformed material_objects material 'matnos="ed"' || exit 1
check_malformed material_objects material 'matlist="ed"' || exit 1
check_malformed material_objects material_mix 'mix_next="ed"' || exit 1

# Material species and simple objects.
check_malformed material_objects matspecies 'ndims=5' || exit 1
check_malformed material_objects matspecies 'nmat=-1' || exit 1
check_malformed simple_objects curve 'npts=-1' || exit 1
check_malformed simple_objects compound 'nelems=-1' || exit 1
check_malformed simple_objects compound 'nvalues=-1' || exit 1
check_malformed simple_objects defvars 'ndefs=-1' || exit 1

# Multi-block objects.
check_malformed multi_objects multimesh 'nblocks=-1' || exit 1
check_malformed multi_objects multimeshadj 'nblocks=-1' || exit 1
check_malformed multi_objects multimeshadj 'lneighbors=-1' || exit 1
check_malformed multi_objects multivar 'nvars=-1' || exit 1
check_malformed multi_objects multimat 'nmats=-1' || exit 1
check_malformed multi_objects multimatspecies 'nspec=-1' || exit 1

# Point, quad, UCD and list objects.
check_malformed point_objects pointmesh 'ndims=5' || exit 1
check_malformed point_objects pointvar 'nvals=99' || exit 1
check_malformed quad_objects quadmesh 'ndims=5' || exit 1
check_malformed quad_objects quadvar 'nvals=99' || exit 1
check_malformed ucd_objects ucdmesh 'ndims=5' || exit 1
check_malformed ucd_objects ucdvar 'nvals=99' || exit 1
check_malformed ucd_objects zl2 'nzones=-1' || exit 1
check_malformed flphzl_objects facelist 'nfaces=-1' || exit 1
check_malformed flphzl_objects phzl 'nfaces=-1' || exit 1

# CSG and MRG objects.
check_malformed csg_objects csgmesh 'ndims=5' || exit 1
check_malformed csg_objects csgzl 'nregs=-1' || exit 1
check_malformed csg_objects csgvar 'nvals=99' || exit 1
check_malformed mrg_objects groupelmap 'num_segments=-1' || exit 1
check_malformed mrg_objects mrgtree 'num_nodes=-1' || exit 1
check_malformed mrg_objects mrgvar 'ncomps=99' || exit 1

#
# Cleanup
#
rm -rf malformed.$ext

exit 0
