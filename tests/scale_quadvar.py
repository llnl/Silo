#!/usr/bin/env python3

import Silo

# read box-in-a-box data
f = Silo.Open("box_in_a_box.silo", Silo.DB_READ)

msName = f.GetToc().qmesh_names[0]
msInfo = f.GetVarInfo(msName,1)

znName = f.GetToc().mat_names[0]
znInfo = f.GetVarInfo(znName, 1)

qvName = f.GetToc().qvar_names[0]
qvInfo = f.GetVarInfo(qvName, 1)

f.Close()

g = Silo.Create("scale_quadvar.silo", "scale data", Silo.DB_HDF5, Silo.DB_CLOBBER)

# write mesh data
g.WriteObject(msName, msInfo)

# write zone data
g.WriteObject(znName, znInfo)

# create scale data
scaleArr = tuple(val * 2 for val in qvInfo['value0'])

# override old quadvar with new data
qvInfo['name'] = "scaled_data"
qvInfo['value0'] = scaleArr

# write the new quadvar to the output file
g.WriteObject(qvInfo['name'], qvInfo)
# close the file
g.Close()
