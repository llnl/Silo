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
import os, sys
import Silo

# ---- file 1 ----
file = "multi_ucd3d.pdb"
if len(sys.argv) > 1 and sys.argv[1] == "DB_HDF5" and os.access("multi_ucd3d.h5",os.R_OK):
    file = "multi_ucd3d.h5"

db = Silo.Open(file)
print("db = ",db)
print("db.filename = '%s'"%db.filename)

toc = db.GetToc()
print("\n-- TOC --\n",toc)

print("cycle='%d'"%db.GetVar("cycle"))
print("dtime='%f'"%db.GetVar("dtime"))
print("_fileinfo='%s'"%db.GetVarInfo("_fileinfo"))
print("d['vartypes']=",db.GetVarInfo("d",1)['vartypes'])
print("d['varnames']=",db.GetVarInfo("d",1)['varnames'])

db.SetDir("block0")
toc = db.GetToc()
print("mesh1['coord0']=",db.GetVarInfo("mesh1",1)['coord0'][1:10])
print(db.GetVarInfo("v",1))
print("v['value0']=",db.GetVarInfo("v",1)['value0'][1:10])

db.SetDir("../block7")
toc = db.GetToc()
print("mesh1['coord0']=",db.GetVarInfo("mesh1",1)['coord0'][1:10])
print("v['value0']=",db.GetVarInfo("v",1)['value0'][1:10])

db.Close()

print("\n")

# ---- file 2 ----
file = "multi_rect3d.pdb"
if len(sys.argv) > 1 and sys.argv[1] == "DB_HDF5" and os.access("multi_rect3d.h5",os.R_OK):
    file = "multi_rect3d.h5"

db = Silo.Open(file)
print("db = ",db)

toc = db.GetToc()
print("\n-- TOC --\n",toc)

print("cycle='%d'"%db.GetVar("cycle"))
print("time='%f'"%db.GetVar("time"))
print("_fileinfo='%s'"%db.GetVar("_fileinfo"))
print("defvars['defns']=",db.GetVarInfo("defvars",1)['defns'])

db.SetDir("block0")
toc = db.GetToc()
print("mesh1['coord0']=",db.GetVarInfo("mesh1",1)['coord0'])

db.SetDir("/block7")
toc = db.GetToc()
print("mesh1['coord0']=",db.GetVarInfo("mesh1",1)['coord0'])

db.SetDir("/")
toc = db.GetToc()
db.Close()
