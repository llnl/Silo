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
e_callfail_code=$(grep E_CALLFAIL $silo_header | tr -s ' ' | cut -d' ' -f3)
[ $? -eq 0 ] || exit 1
db_short_val=$(grep DB_SHORT $silo_header | tr -s ' ' | cut -d'=' -f2 | tr -d ',')
[ $? -eq 0 ] || exit 1

#
# Find text executable to generate data and then resulting data file
#
if [ -z "$2" ]; then
    all_objs=$(find_file -x all_silo_objects tests/bin/all_silo_objects)
    [ $? -eq 0 ] || exit 1
    $all_objs $1
    [ $? -eq 0 ] || exit 1
    all_objs_file=$(find_file -r all_objects.$ext tests/all_objects.$ext)
    [ $? -eq 0 ] || exit 1
    old_silo=0
else
    all_objs_file=$(find_file -r $2.$ext tests/$2.$ext)
    [ $? -eq 0 ] || exit 1
    old_silo=1
fi

#
# Corrupt one object at a time with browser's low-level write mode, then
# confirm that the corresponding high-level DBGetXxx path reports
# E_MALFORMED.
#
malformed_filename=malformed_$$.$ext
check_malformed()
{
    dirname=$1
    objname=$2
    cname_assign=$3
    subarr=$4
    ecode=$5

    sep="."
    if [ $ext = "pdb" ] && [ -n "$subarr" ]; then
        sep="_"
    fi

    if [ -z "$ecode" ]; then
        ecode=$e_malformed_code
    fi    

    # Create temp file to corrupt
    cp $all_objs_file $malformed_filename || return 1

    echo ""
    echo ""
    echo ""
    echo "******************************************************************" >&2
    echo "Working on $dirname/$objname for $cname_assign" >&2
    echo "******************************************************************" >&2

    # Confirm we can read the object fine as is. Read it "normally" and with lowlevel 1 (-l 1)
    echo ""
    echo "Confirm read of valid object..." >&2
    $browser -q --proper-exit-code -e "cd $dirname" -e "$objname" $malformed_filename >&2
    [ $? -eq 0 ] || {
        if [ $old_silo -eq 0 ]; then
            echo "ERROR: expected valid high-level read for $dirname/$objname before corrupting with $cname_assign" >&2
        else
            echo "ERROR: expected valid high-level read for $dirname/$objname" >&2
        fi
        return 1
    }
    echo ""
    echo "Lowlevel read of valid object..." >&2
    $browser -q --proper-exit-code -l 1 -e "cd $dirname" -e "$objname" $malformed_filename >&2
    [ $? -eq 0 ] || {
        if [ $old_silo -eq 0 ]; then
            echo "ERROR: expected valid lowlevel read for $dirname/$objname before corrupting with $cname_assign" >&2
        else
            echo "ERROR: expected valid lowlevel read for $dirname/$objname" >&2
        fi
        return 1
    }
    [ $old_silo -eq 0 ] || return 0

    # Corrupt the specific object
    echo ""
    echo "Corrupt the object..." >&2
    $browser --proper-exit-code -q -W -l 1 -e "cd $dirname" -e "${objname}${sep}${cname_assign}" $malformed_filename
    [ $? -eq 0 ] || {
        echo "ERROR: Attempted corruption of $dirname/$objname with $cname_assign failed" >&2
        return 1
    }

    # Now, try to display that corrupted object in a new browser instance
    echo ""
    echo "Confirm malformed error triggered on corrupted object..." >&2
    $browser -q --proper-exit-code -e "cd $dirname" -e "$objname" $malformed_filename
    result=$?
    [ $result -eq $ecode ] || {
        echo "ERROR: expected E_MALFORMED ($e_malformed_code) but got $result for $dirname/$objname after $cname_assign" >&2
        return 1
    }
}

#
# Regarding malformed issues involving 'datatype' member of various objects...
# PDB and HDF5 drivers deal with datatype totally differently.
#
# In PDB driver, we store datatype information as metadata and return that stored
# metadata on read. So, it is important there during read to ensure the datatype
# metadata is consistent with the void* data of the object being read and issue
# E_MALFORMED if not.
#
# In HDF5 driver, although we do indeed store datatype information as metadata,
# we actually ignore that information during read. On write, we use datatype
# information to decode the type of void* pointers passed as args in the API.
# On read, we use datatype of HDF5 *stored* data to populate value of datatype
# member and not the stored metadata for that member. This means attempts to
# corrupt file objects by setting wrong datatype have no effect. It would have
# been better to simply not ever store datatype as metadata. But, its too late
# to fix that now.
#

# Various corruptions of material objects
check_malformed mat_objs material ndims=5 || exit 1
if [ "$ext" = "pdb" ]; then
    check_malformed mat_objs material dims='"/arrs/intsA"' || exit 1
else
    check_malformed mat_objs material dims=\{3,4,5\} || exit 1
fi
check_malformed mat_objs material nmat=7 || exit 1
check_malformed mat_objs material matnos='"/arrs/intsB"' || exit 1
check_malformed mat_objs material matlist='"/arrs/intsB"' || exit 1
check_malformed mat_objs material_names_colors matnames='"/arrs/strsB"' || exit 1
check_malformed mat_objs material_names_colors matcolors='"/arrs/strsB"' || exit 1
check_malformed mat_objs material_mix mixlen=4495 || exit 1
check_malformed mat_objs material_mix mix_next='"/arrs/intsB"' || exit 1
check_malformed mat_objs material_mix mix_vf='"/arrs/fltsA"' || exit 1
check_malformed mat_objs material_mix mix_vf='"/arrs/dblsA"' || exit 1
check_malformed mat_objs material_mix mix_mat='"/arrs/intsA"' || exit 1
check_malformed mat_objs material_mix mix_zone='"/arrs/intsB"' || exit 1
if [ "$ext" = "pdb" ]; then
    check_malformed mat_objs material_mix datatype=$db_short_val || exit 1
    check_malformed mat_objs material_mix mix_vf='"/arrs/svals4"' || exit 1
fi

# Various corruptions of Matspecies objects
check_malformed mat_objs matspecies ndims=5 || exit 1
check_malformed mat_objs matspecies2 ndims=5 || exit 1
check_malformed mat_objs matspecies nmat=-1 || exit 1
check_malformed mat_objs matspecies2 nmat=-1 || exit 1
check_malformed mat_objs matspecies nspecies_mf=99 || exit 1
check_malformed mat_objs matspecies nmatspec='"/arrs/intsA"' || exit 1
check_malformed mat_objs matspecies speccolors='"/arrs/strsA"' || exit 1
check_malformed mat_objs matspecies species_mf='"/arrs/fltsA"' || exit 1
# diff in component name between pdb and hdf5
if [ "$ext" = "pdb" ]; then
    check_malformed mat_objs matspecies datatype=$db_short_val || exit 1
    check_malformed mat_objs matspecies species_names='"/arrs/strsA"' || exit 1
    check_malformed mat_objs matspecies dims='"/arrs/intsA"' || exit 1
    check_malformed mat_objs matspecies species_mf='"/arrs/svals3"' || exit 1
else
    check_malformed mat_objs matspecies specnames='"/arrs/strsA"' || exit 1
    check_malformed mat_objs matspecies dims=\{3,4,5\} || exit 1
fi

# Various corruptions of Curve objects
check_malformed misc_objs curve npts=-1 || exit 1
check_malformed misc_objs curve npts=6 || exit 1
if [ "$ext" = "pdb" ]; then
    check_malformed misc_objs curve datatype=$db_short_val || exit 1
    check_malformed misc_objs curve xvals='"/arrs/svals5"' || exit 1
    check_malformed misc_objs curve yvals='"/arrs/svals5"' || exit 1
else
    check_malformed misc_objs curve xvarname='"/arrs/fltsB"' || exit 1
    check_malformed misc_objs curve yvarname='"/arrs/fltsA"' || exit 1
fi

# Various corruptions of Compoundarray objects
check_malformed misc_objs compound nelems=-1 || exit 1
check_malformed misc_objs compound nelems=99 || exit 1
check_malformed misc_objs compound nvalues=-1 || exit 1
check_malformed misc_objs compound nvalues=99 || exit 1
check_malformed misc_objs compound elemnames='"/arrs/strsA"' || exit 1
check_malformed misc_objs compound elemlengths='"/arrs/intsB"' || exit 1
check_malformed misc_objs compound values='"/arrs/intsB"' || exit 1
if [ "$ext" = "pdb" ]; then
    check_malformed misc_objs compound datatype=$db_short_val || exit 1
    check_malformed misc_objs compound values='"/arrs/svals6"' || exit 1
fi

# Various corruptions of defvars objects
check_malformed misc_objs defvars ndefs=-1 || exit 1
check_malformed misc_objs defvars types='"/arrs/intsB"' || exit 1
check_malformed misc_objs defvars names='"/arrs/strsA"' || exit 1
check_malformed misc_objs defvars defns='"/arrs/strsB"' || exit 1

# Various corruptions of multimesh objects
check_malformed mult_objs multimesh nblocks=-1 || exit 1
check_malformed mult_objs multimesh2 nblocks=-1 || exit 1
check_malformed mult_objs multimesh empty_cnt=99 || exit 1
check_malformed mult_objs multimesh empty_list='"/arrs/intsB"' || exit 1
check_malformed mult_objs multimesh has_external_zones='"/arrs/intsB"' || exit 1
check_malformed mult_objs multimesh zonecounts='"/arrs/intsB"' || exit 1
check_malformed mult_objs multimesh extentssize=99 || exit 1
check_malformed mult_objs multimesh extents='"/arrs/dblsB"' || exit 1
check_malformed mult_objs multimesh meshnames='"/arrs/strsB"' || exit 1
check_malformed mult_objs multimesh meshtypes='"/arrs/intsA"' || exit 1
check_malformed mult_objs multimesh2 meshtypes='"/arrs/intsA"' || exit 1

# Various corruptions of multimeshdj objects
check_malformed mult_objs multimeshadj nblocks=-1 || exit 1
check_malformed mult_objs multimeshadj nblocks=99 || exit 1
check_malformed mult_objs multimeshadj lneighbors=-1 || exit 1
check_malformed mult_objs multimeshadj totlnodelists=3 || exit 1
check_malformed mult_objs multimeshadj totlzonelists=3 || exit 1
check_malformed mult_objs multimeshadj meshtypes='"/arrs/intsB"' || exit 1
check_malformed mult_objs multimeshadj nneighbors='"/arrs/intsA"' || exit 1
check_malformed mult_objs multimeshadj neighbors='"/arrs/intsB"' || exit 1
check_malformed mult_objs multimeshadj back='"/arrs/intsB"' || exit 1
check_malformed mult_objs multimeshadj lnodelists='"/arrs/intsA"' || exit 1
# We cannot reasonably test corruption of the actual nodelists or zonelists
# content because that requies the read methods to do problem sized work.
# But, testing corruption of "content" of those arrays is also not necessary.
#check_malformed mult_objs multimeshadj nodelists='"/arrs/intsB"' || exit 1
check_malformed mult_objs multimeshadj lzonelists='"/arrs/intsA"' || exit 1
#check_malformed mult_objs multimeshadj zonelists='"/arrs/intsB"' || exit 1

# Various corruptions of multivar objects
check_malformed mult_objs multivar nvars=-1 || exit 1
check_malformed mult_objs multivar2 nvars=-1 || exit 1
check_malformed mult_objs multivar empty_cnt=99 || exit 1
check_malformed mult_objs multivar empty_list='"/arrs/intsB"' || exit 1
check_malformed mult_objs multivar extentssize=99 || exit 1
check_malformed mult_objs multivar extents='"/arrs/dblsB"' || exit 1
check_malformed mult_objs multivar varnames='"/arrs/strsB"' || exit 1
check_malformed mult_objs multivar vartypes='"/arrs/intsA"' || exit 1
check_malformed mult_objs multivar2 vartypes='"/arrs/intsA"' || exit 1

# Various corruptions of multimat objects
check_malformed mult_objs multimat nmats=-1 || exit 1
check_malformed mult_objs multimat2 nmats=-1 || exit 1
check_malformed mult_objs multimat nmats=99 || exit 1
check_malformed mult_objs multimat2 nmats=99 || exit 1
check_malformed mult_objs multimat nmatnos=99 || exit 1
check_malformed mult_objs multimat empty_cnt=99 || exit 1
check_malformed mult_objs multimat matnos='"/arrs/intsB"' || exit 1
check_malformed mult_objs multimat mixlens='"/arrs/intsB"' || exit 1
check_malformed mult_objs multimat matcounts='"/arrs/intsB"' || exit 1
check_malformed mult_objs multimat matlists='"/arrs/intsB"' || exit 1
check_malformed mult_objs multimat empty_list='"/arrs/intsB"' || exit 1
check_malformed mult_objs multimat matnames='"/arrs/strsB"' || exit 1
# NOTE: mat_colors/matcolors confusion in DBmultimats
if [ "$ext" = "pdb" ]; then
    check_malformed mult_objs multimat matcolors='"/arrs/strsB"' || exit 1
else
    check_malformed mult_objs multimat mat_colors='"/arrs/strsB"' || exit 1
fi

# Various corruptions of multimatspecies objects
check_malformed mult_objs multimatspecies nspec=-1 || exit 1
check_malformed mult_objs multimatspecies2 nspec=-1 || exit 1
check_malformed mult_objs multimatspecies nspec=99 || exit 1
check_malformed mult_objs multimatspecies2 nspec=99 || exit 1
check_malformed mult_objs multimatspecies nmat=99 || exit 1
check_malformed mult_objs multimatspecies nmatspec='"/arrs/intsB"' || exit 1
check_malformed mult_objs multimatspecies specnames='"/arrs/strsB"' || exit 1
check_malformed mult_objs multimatspecies speccolors='"/arrs/strsB"' || exit 1
check_malformed mult_objs multimatspecies empty_list='"/arrs/intsB"' || exit 1
check_malformed mult_objs multimatspecies empty_cnt=99 || exit 1

# Various corruptions of pointmesh objects
check_malformed point_objs pointmesh ndims=5 || exit 1
check_malformed point_objs pointmesh2 ndims=5 || exit 1
check_malformed point_objs pointmesh nels=2 || exit 1
check_malformed point_objs pointmesh2 nels=2 || exit 1
check_malformed point_objs pointmesh gnodeno='"/arrs/intsA"' || exit 1
check_malformed point_objs pointmesh ghost_node_labels='"/arrs/chrsA"' || exit 1
check_malformed point_objs pointmesh coord0='"/arrs/fltsA"' || exit 1
check_malformed point_objs pointmesh coord1='"/arrs/fltsB"' || exit 1
check_malformed point_objs pointmesh2 coord1='"/arrs/fltsB"' || exit 1
check_malformed point_objs pointmesh gnodeno='"/arrs/intsA"' || exit 1
check_malformed point_objs pointmesh ghost_node_labels='"/arrs/intsA"' || exit 1
if [ "$ext" = "pdb" ]; then
    check_malformed point_objs pointmesh datatype=$db_short_val || exit 1
    check_malformed point_objs pointmesh coord0='"/arrs/svals5"' || exit 1
    check_malformed point_objs pointmesh coord1='"/arrs/svals5"' || exit 1
fi

# Various corruptions of pointvar objects
check_malformed point_objs pointvar nvals=99 || exit 1
check_malformed point_objs pointvar nels=3 || exit 1
if [ "$ext" = "pdb" ]; then
    check_malformed point_objs pointvar datatype=$db_short_val || exit 1
    # Funky browser syntax issue parsing component names beginning with number
    # requires us to quote args slightly differently for PDB driver.
    check_malformed point_objs pointvar "'0_data'='/arrs/svals5'" || exit 1
    check_malformed point_objs pointvar "'0_data'='/arrs/fltsB'" || exit 1
    check_malformed point_objs pointvar "'1_data'='/arrs/svals5'" || exit 1
    check_malformed point_objs pointvar "'1_data'='/arrs/intsA'" || exit 1
else
    check_malformed point_objs pointvar data0='"/arrs/fltsB"' || exit 1
    check_malformed point_objs pointvar data1='"/arrs/fltsA"' || exit 1
fi

# Various corruptions of quadmesh objects
check_malformed quad_objs quadmesh ndims=5 || exit 1
check_malformed quad_objs quadmesh2 ndims=5 || exit 1
check_malformed quad_objs quadmesh3 ndims=5 || exit 1
check_malformed quad_objs quadmesh ghost_node_labels='"/arrs/chrsA"' || exit 1
check_malformed quad_objs quadmesh ghost_zone_labels='"/arrs/chrsB"' || exit 1
# odd issue in PDB driver with max index names
if [ "$ext" = "pdb" ]; then
    check_malformed quad_objs quadmesh datatype=$db_short_val || exit 1
    check_malformed quad_objs quadmesh coord0='"/arrs/svals3"' || exit 1
    check_malformed quad_objs quadmesh coord1='"/arrs/svals3"' || exit 1
    check_malformed quad_objs quadmesh3 coord1='"/arrs/svals3"' || exit 1
    check_malformed quad_objs quadmesh min_index=\{-1,-1\} subarr || exit 1
    check_malformed quad_objs quadmesh max_index_n=\{99,99\} subarr || exit 1
    check_malformed quad_objs quadmesh dims=\{2,3\} subarr || exit 1
else
    check_malformed quad_objs quadmesh min_index=\{-1,-1,-1\} || exit 1
    check_malformed quad_objs quadmesh max_index=\{99,99,99\} || exit 1
    check_malformed quad_objs quadmesh dims=\{2,3,0\} || exit 1
    check_malformed quad_objs quadmesh3 dims=\{2,3,0\} || exit 1
fi

# Various corruptions of quadvar objects
check_malformed quad_objs quadvar nvals=99 || exit 1
check_malformed quad_objs quadvar nels=3 || exit 1
check_malformed quad_objs quadvar ndims=5 || exit 1
check_malformed quad_objs quadvar mixlen=5 || exit 1
# On PDB, this fails in PJ_ReadVariable 
if [ "$ext" = "pdb" ]; then
    check_malformed quad_objs quadvar datatype=$db_short_val || exit 1
    check_malformed quad_objs quadvar value1='"/arrs/svals4"' || exit 1
    check_malformed quad_objs quadvar mixed_value0='"/arrs/svals8"' || exit 1
    check_malformed quad_objs quadvar dims='"/arrs/intsB"' "" $e_callfail_code || exit 1
    check_malformed quad_objs quadvar align='"/arrs/fltsB"' "" $e_callfail_code || exit 1
else
    check_malformed quad_objs quadvar dims=\{5,7,3\} || exit 1
fi
check_malformed quad_objs quadvar value0='"/arrs/fltsA"' || exit 1
check_malformed quad_objs quadvar value1='"/arrs/fltsA"' || exit 1
check_malformed quad_objs quadvar mixed_value0='"/arrs/fltsA"' || exit 1
check_malformed quad_objs quadvar mixed_value1='"/arrs/fltsA"' || exit 1

# Various corruptions of ucdmesh objects
check_malformed ucd_objs ucdmesh ndims=5 || exit 1
check_malformed ucd_objs ucdmesh2 ndims=5 || exit 1
check_malformed ucd_objs ucdmesh nnodes=8 || exit 1
check_malformed ucd_objs ucdmesh2 nnodes=8 || exit 1
check_malformed ucd_objs ucdmesh coord0='"/arrs/dblsA"' || exit 1
check_malformed ucd_objs ucdmesh coord1='"/arrs/dblsA"' || exit 1
check_malformed ucd_objs ucdmesh ghost_node_labels='"/arrs/chrsA"' || exit 1
check_malformed ucd_objs ucdmesh gnodeno='"/arrs/intsA"' || exit 1
if [ "$ext" = "pdb" ]; then
    check_malformed ucd_objs ucdmesh datatype=$db_short_val || exit 1
    check_malformed ucd_objs ucdmesh coord0='"/arrs/svals9"' || exit 1
fi
# Extents arrays are clamped in size in both drivers in different ways
#check_malformed ucd_objs ucdmesh min_extents='"/arrs/dblsA"' || exit 1
#check_malformed ucd_objs ucdmesh max_extents='"/arrs/dblsA"' || exit 1

# Various corruptions of ucdvar objects
check_malformed ucd_objs ucdvar ndims=-1 || exit 1
check_malformed ucd_objs ucdvar nvals=99 || exit 1
check_malformed ucd_objs ucdvar nels=9999 || exit 1
check_malformed ucd_objs ucdvar_mix mixlen=2 || exit 1
check_malformed ucd_objs ucdvar_mix value0='"/arrs/fltsA"' || exit 1
check_malformed ucd_objs ucdvar_mix mixed_value0='"/arrs/fltsA"' || exit 1
if [ "$ext" = "pdb" ]; then
    check_malformed ucd_objs ucdvar_mix datatype=$db_short_val || exit 1
    check_malformed ucd_objs ucdvar_mix value0='"/arrs/svals4"' || exit 1
    check_malformed ucd_objs ucdvar_mix mixed_value0='"/arrs/svals3"' || exit 1
fi

# Various corruptions of facelist objects
check_malformed list_objs facelist nfaces=-1 || exit 1
check_malformed list_objs facelist nfaces=99 || exit 1
check_malformed list_objs facelist nshapes=99 || exit 1
check_malformed list_objs facelist lnodelist=99 || exit 1
check_malformed list_objs facelist nodelist='"/arrs/intsA"' || exit 1
check_malformed list_objs facelist shapecnt='"/arrs/intsA"' || exit 1
check_malformed list_objs facelist shapesize='"/arrs/intsA"' || exit 1
check_malformed list_objs facelist zoneno='"/arrs/intsA"' || exit 1

# Various corruptions of zonelist objects
check_malformed ucd_objs zl2 nzones=-1 || exit 1
check_malformed ucd_objs zl2 nzones=99 || exit 1
check_malformed ucd_objs zl2 nshapes=99 || exit 1
check_malformed ucd_objs zl2 lo_offset=99 || exit 1
check_malformed ucd_objs zl2 hi_offset=-1 || exit 1
check_malformed ucd_objs zl2 lnodelist=99 || exit 1
check_malformed ucd_objs zl2 ghost_zone_labels='"/arrs/chrsB"' || exit 1
check_malformed ucd_objs zl2 gzoneno='"/arrs/intsB"' || exit 1
check_malformed ucd_objs zl2 nodelist='"/arrs/intsA"' || exit 1
check_malformed ucd_objs zl2 shapecnt='"/arrs/intsB"' || exit 1
check_malformed ucd_objs zl2 shapesize='"/arrs/intsB"' || exit 1
check_malformed ucd_objs zl2 shapetype='"/arrs/intsB"' || exit 1

# Various corruptions of polyhedral zonelist objects
check_malformed list_objs phzl nfaces=-1 || exit 1
check_malformed list_objs phzl nfaces=1 || exit 1
check_malformed list_objs phzl nzones=1 || exit 1
check_malformed list_objs phzl lnodelist=2 || exit 1
check_malformed list_objs phzl lfacelist=3 || exit 1
check_malformed list_objs phzl lo_offset=5 || exit 1
check_malformed list_objs phzl hi_offset=-1 || exit 1
check_malformed list_objs phzl nodecnt='"/arrs/intsB"' || exit 1
check_malformed list_objs phzl nodelist='"/arrs/intsA"' || exit 1
check_malformed list_objs phzl facecnt='"/arrs/intsB"' || exit 1
check_malformed list_objs phzl facelist='"/arrs/intsA"' || exit 1
check_malformed list_objs phzl ghost_zone_labels='"/arrs/chrsB"' || exit 1
check_malformed list_objs phzl gzoneno='"/arrs/intsB"' || exit 1

# Various corruptions of csgmesh objects
check_malformed csg_objs csgmesh ndims=5 || exit 1
check_malformed csg_objs csgmesh2 ndims=5 || exit 1
check_malformed csg_objs csgmesh nbounds=2 || exit 1
check_malformed csg_objs csgmesh2 nbounds=2 || exit 1
check_malformed csg_objs csgmesh lcoeffs=2 || exit 1
check_malformed csg_objs csgmesh coeffs='"/arrs/intsA"' || exit 1
check_malformed csg_objs csgmesh typeflags='"/arrs/intsA"' || exit 1
check_malformed csg_objs csgmesh bndnames='"/arrs/strsA"' || exit 1
#check_malformed csg_objs csgmesh min_extents='"/arrs/dblsB"' || exit 1
#check_malformed csg_objs csgmesh max_extents='"/arrs/dblsB"' || exit 1
if [ "$ext" = "pdb" ]; then
    check_malformed csg_objs csgmesh datatype=$db_short_val || exit 1
    check_malformed csg_objs csgmesh coeffs='"/arrs/svals4"' || exit 1
fi

# Various corruptions of csg zonelist objects
check_malformed csg_objs csgzl nregs=-1 || exit 1
check_malformed csg_objs csgzl2 nregs=-1 || exit 1
check_malformed csg_objs csgzl nregs=99 || exit 1
check_malformed csg_objs csgzl2 nregs=99 || exit 1
check_malformed csg_objs csgzl nzones=-1 || exit 1
check_malformed csg_objs csgzl nzones=99 || exit 1
check_malformed csg_objs csgzl regnames='"/arrs/strsB"' || exit 1
check_malformed csg_objs csgzl zonenames='"/arrs/strsB"' || exit 1
check_malformed csg_objs csgzl typeflags='"/arrs/intsA"' || exit 1
check_malformed csg_objs csgzl leftids='"/arrs/intsA"' || exit 1
check_malformed csg_objs csgzl2 leftids='"/arrs/intsA"' || exit 1
check_malformed csg_objs csgzl rightids='"/arrs/intsA"' || exit 1
check_malformed csg_objs csgzl zonelist='"/arrs/intsA"' || exit 1

# Various corruptions of csgvar objects
check_malformed csg_objs csgvar nvals=99 || exit 1
check_malformed csg_objs csgvar nels=2 || exit 1
# Component naming difference between drivers
if [ "$ext" = "pdb" ]; then
    check_malformed csg_objs csgvar datatype=$db_short_val || exit 1
    check_malformed csg_objs csgvar value0='"/arrs/fltsA"' || exit 1
    check_malformed csg_objs csgvar value0='"/arrs/svals1"' || exit 1
else
    check_malformed csg_objs csgvar vals0='"/arrs/fltsA"' || exit 1
fi

# Various corruptions of groupelmap objects
check_malformed mrg_objs groupelmap num_segments=-1 || exit 1
check_malformed mrg_objs groupelmap num_segments=2 || exit 1
check_malformed mrg_objs groupelmap groupel_types='"/arrs/intsA"' || exit 1
check_malformed mrg_objs groupelmap segment_lengths='"/arrs/intsA"' || exit 1
check_malformed mrg_objs groupelmap segment_ids='"/arrs/intsA"' || exit 1
check_malformed mrg_objs groupelmap segment_data='"/arrs/intsA"' || exit 1
check_malformed mrg_objs groupelmap frac_lengths='"/arrs/intsA"' || exit 1
check_malformed mrg_objs groupelmap segment_fracs='"/arrs/fltsA"' || exit 1
if [ "$ext" = "pdb" ]; then
    check_malformed mrg_objs groupelmap fracs_data_type=$db_short_val || exit 1
    check_malformed mrg_objs groupelmap segment_fracs='"/arrs/svals5"' || exit 1
fi

# Various corruptions of mrgtree objects
check_malformed mrg_objs mrgtree num_nodes=-1 || exit 1
check_malformed mrg_objs mrgtree num_nodes=99 || exit 1
check_malformed mrg_objs mrgtree root=-1 || exit 1
check_malformed mrg_objs mrgtree root=5 || exit 1
if [ "$ext" = "pdb" ]; then
    check_malformed mrg_objs mrgtree seg_types='"/arrs/intsB"'|| exit 1
    check_malformed mrg_objs mrgtree seg_lens='"/arrs/intsB"'|| exit 1
    check_malformed mrg_objs mrgtree seg_ids='"/arrs/intsB"'|| exit 1
    check_malformed mrg_objs mrgtree scalars='"/arrs/intsA"'|| exit 1
    check_malformed mrg_objs mrgtree children='"/arrs/intsA"'|| exit 1
    check_malformed mrg_objs mrgtree maps_name='"/arrs/strsB"'|| exit 1
else
    check_malformed mrg_objs mrgtree n_seg_types='"/arrs/intsB"'|| exit 1
    check_malformed mrg_objs mrgtree n_seg_lens='"/arrs/intsB"'|| exit 1
    check_malformed mrg_objs mrgtree n_seg_ids='"/arrs/intsB"'|| exit 1
    check_malformed mrg_objs mrgtree n_scalars='"/arrs/intsA"'|| exit 1
    check_malformed mrg_objs mrgtree n_children='"/arrs/intsA"'|| exit 1
    check_malformed mrg_objs mrgtree n_maps_name='"/arrs/strsB"'|| exit 1
fi

# Various corruptions of mrgvar objects
check_malformed mrg_objs mrgvar ncomps=-1 || exit 1
check_malformed mrg_objs mrgvar ncomps=99 || exit 1
check_malformed mrg_objs mrgvar nregns=2 || exit 1
if [ "$ext" = "pdb" ]; then
    check_malformed mrg_objs mrgvar datatype=$db_short_val || exit 1
    check_malformed mrg_objs mrgvar value0='"/arrs/fltsA"' || exit 1
    check_malformed mrg_objs mrgvar value0='"/arrs/svals3"' || exit 1
    check_malformed mrg_objs mrgvar value1='"/arrs/svals3"' || exit 1
else
    check_malformed mrg_objs mrgvar data0='"/arrs/fltsA"' || exit 1
fi
check_malformed mrg_objs mrgvar compnames='"/arrs/strsB"' || exit 1

#
# Cleanup
#
rm -rf $malformed_filename

exit 0
