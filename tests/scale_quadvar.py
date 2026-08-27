#!/usr/bin/env python3

import os, sys
import Silo

# Ensure we can run standalone too
try:
    SKIP_RETURN_VALUE = int(os.environ["SILO_TEST_SKIP_RETURN_CODE"])
except:
    SKIP_RETURN_VALUE = 17

# read box-in-a-box data
try:
    f = Silo.Open("box_in_a_box.silo", Silo.DB_READ)
except:
    f = Silo.Open("bin/box_in_a_box.silo", Silo.DB_READ)

#
# The test file used here was produced with a Silo library using
# deflate compression. If the HDF lib in current use is not set
# up for that, reads will fail with compression errors. If that
# happens, just skip this test. If any other read failure occurs
# that is a real problem.
#
try:
    msName = f.GetToc().qmesh_names[0]
    msInfo = f.GetVarInfo(msName,1)
except Silo.SiloException as e:
    if e.args[0] == Silo.E_COMPRESSION:
        print("HDF5 lib not configured with deflate filter")
        sys.exit(SKIP_RETURN_VALUE)
    raise

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
