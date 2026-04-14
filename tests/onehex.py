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
import sys, Silo

if len(sys.argv) == 1:
    db = Silo.Create("onehexpy.silo", "This is a test to see how well this string displays in Qt widget in Silex\nThis is a test to see how well this string displays in Qt widget in Silex\nThis is a test to see how well this string displays in Qt widget in Silex\nThis is a test to see how well this string displays in Qt widget in Silex\n")
elif 'DB_HDF5' in sys.argv[1]:
    db = Silo.Create("onehexpy.silo", "onehex from python to DB_HDF5", Silo.DB_HDF5, Silo.DB_CLOBBER);
else:
    db = Silo.Create("onehexpy.silo", "onehex from python to DB_PDB", Silo.DB_PDB, Silo.DB_CLOBBER);

# coordinate arrays for the mesh
coord0 = (0, 0.707107, 0, -0.707107, 0, 0.707107, 0, -0.707107)
coord1 = (0, 0.5, 1, 0.5, 0.707107, 1.20711, 1.70711, 1.20711)
coord2 = (0, 0.5, 1, 0.5, -0.707107, -0.207107, 0.292893, -0.207107)
min_extents = (-0.707107, 0, -0.707107)
max_extents = (0.707107, 1.70711, 1)

# the ucd mesh object
mesh = {"name":"hex",\
        "type":"DBucdmesh",\
        "coord0":coord0,\
        "coord1":coord1,\
        "coord2":coord2,\
        "min_extents":min_extents,\
        "max_extents":max_extents,\
        "zonelist":"zonelist",\
        "ndims":3,\
        "nnodes":8,\
        "nzones":1,\
        "facetype":100,\
        "cycle":0,\
        "coord_sys":Silo.DB_OTHER,\
        "datatype":Silo.DB_DOUBLE,\
        "origin":0}
db.WriteObject("hex", mesh)

# nodelist and shape arrays for the zonelist
nodelist = (0, 1, 2, 3, 4, 5, 6, 7)
shapecnt = (1,)
shapesize = (8,)

# zonelist object
zonelist = {"name":"zonelist",\
            "type":"DBzonelist",\
            "ndims":3,\
            "nzones":1,\
            "nshapes":1,\
            "shapecnt":shapecnt,\
            "shapesize":shapesize,\
            "lnodelist":len(nodelist),\
            "nodelist":nodelist}
db.WriteObject("zonelist", zonelist)

# variable array
value0 = (0, 0.957107, 1, -0.457107, -0.5, 0.457107, 0.5, -0.957107)

# variable object
v = {"name":"v",\
     "type":"DBucdvar",\
     "meshid":"hex",\
     "datatype":Silo.DB_DOUBLE,\
     "nels":len(value0),\
     "nvals":1,\
     "ndims":3,\
     "centering":Silo.DB_NODECENT,\
     "value0":value0}
db.WriteObject("v", v)

db.Close()
