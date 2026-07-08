// Copyright (C) 1994-2016 Lawrence Livermore National Security, LLC.
// LLNL-CODE-425250.
// All rights reserved.
// 
// This file is part of Silo. For details, see silo.llnl.gov.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 
//    * Redistributions of source code must retain the above copyright
//      notice, this list of conditions and the disclaimer below.
//    * Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the disclaimer (as noted
//      below) in the documentation and/or other materials provided with
//      the distribution.
//    * Neither the name of the LLNS/LLNL nor the names of its
//      contributors may be used to endorse or promote products derived
//      from this software without specific prior written permission.
// 
// THIS SOFTWARE  IS PROVIDED BY  THE COPYRIGHT HOLDERS  AND CONTRIBUTORS
// "AS  IS" AND  ANY EXPRESS  OR IMPLIED  WARRANTIES, INCLUDING,  BUT NOT
// LIMITED TO, THE IMPLIED  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A  PARTICULAR  PURPOSE ARE  DISCLAIMED.  IN  NO  EVENT SHALL  LAWRENCE
// LIVERMORE  NATIONAL SECURITY, LLC,  THE U.S.  DEPARTMENT OF  ENERGY OR
// CONTRIBUTORS BE LIABLE FOR  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR  CONSEQUENTIAL DAMAGES  (INCLUDING, BUT NOT  LIMITED TO,
// PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS  OF USE,  DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER  IN CONTRACT, STRICT LIABILITY,  OR TORT (INCLUDING
// NEGLIGENCE OR  OTHERWISE) ARISING IN  ANY WAY OUT  OF THE USE  OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
// This work was produced at Lawrence Livermore National Laboratory under
// Contract  No.   DE-AC52-07NA27344 with  the  DOE.  Neither the  United
// States Government  nor Lawrence  Livermore National Security,  LLC nor
// any of  their employees,  makes any warranty,  express or  implied, or
// assumes   any   liability   or   responsibility  for   the   accuracy,
// completeness, or usefulness of any information, apparatus, product, or
// process  disclosed, or  represents  that its  use  would not  infringe
// privately-owned   rights.  Any  reference   herein  to   any  specific
// commercial products,  process, or  services by trade  name, trademark,
// manufacturer or otherwise does not necessarily constitute or imply its
// endorsement,  recommendation,   or  favoring  by   the  United  States
// Government or Lawrence Livermore National Security, LLC. The views and
// opinions  of authors  expressed  herein do  not  necessarily state  or
// reflect those  of the United  States Government or  Lawrence Livermore
// National  Security, LLC,  and shall  not  be used  for advertising  or
// product endorsement purposes.

#include <Python.h>
#include <silo.h>
#include "pydbfile.h"
#include "pysilo.h"

#include <vector>
#include <string>
#include <iostream>
using namespace std;

static PyObject             *SiloError;
static PyObject             *siloModule = 0;

//
// Method table
//

std::vector<PyMethodDef> SiloMethods;

#if defined(_WIN32)
# if defined(SiloPy_EXPORTS) || defined(Silo_EXPORTS)
#  define SILOMODULE_API __declspec(dllexport)
# else
#  define SILOMODULE_API __declspec(dllimport)
# endif
#else
# if __GNUC__ >= 4
#   define SILOMODULE_API __attribute__ ((visibility("default")))
# else
#   define SILOMODULE_API /* not affected */
# endif
#endif

// ****************************************************************************
// Function: SiloErrorFunc
//
// Purpose: 
//   This function sets the Python error string if we're allowing Python
//   exceptions to be thrown from the Silo module.
//
// Arguments:
//   errString : The error string that is "thrown".
//
// Programmer: Brad Whitlock
// Creation:   Mon Sep 17 11:44:43 PDT 2001
//
// Modifications:
//   
// ****************************************************************************

void
SiloErrorFunc(const char *errString)
{
    PyErr_SetString(SiloError, errString);
}

// ****************************************************************************
// Function: AddMethod
//
// Purpose:
//   This function adds a method to the Silo module's Python method table.
//
// Arguments:
//   methodName : The name of the method.
//   cb         : The Python callback function.
//   doc        : The documentation string for the method.
//
// Programmer: Brad Whitlock
// Creation:   Tue Sep 4 15:36:47 PST 2001
//
// Modifications:
//   
// ****************************************************************************
static void
AddMethod(const char *methodName, PyObject *(cb)(PyObject *, PyObject *),
          const char *doc = NULL)
{
    PyMethodDef newMethod;
    newMethod.ml_name = (char *)methodName;
    newMethod.ml_meth = cb;
    newMethod.ml_flags = METH_VARARGS;
    newMethod.ml_doc = (char *)doc;
    SiloMethods.push_back(newMethod);
}


// ****************************************************************************
//  Method:  silo_Open
//
//  Purpose:
//    Encapsulate DBOpen
//
//  Python Arguments:
//    form 1: filename, mode
//    form 2: filename
//
//  Programmer:  Jeremy Meredith
//  Creation:    July 12, 2005
//
// ****************************************************************************
PyObject *silo_Open(PyObject *self, PyObject *args)
{
    char *filename;
    int mode;
    if (! PyArg_ParseTuple(args, "si", &filename, &mode))
    {
        mode = DB_READ;
        if (! PyArg_ParseTuple(args, "s", &filename))
        {
            PyErr_SetString(PyExc_TypeError,
                            "Open takes either 1 or 2 arguments");
            return NULL;
        }
    }

    PyErr_Clear();
    DBfile *db = DBOpen(filename, DB_UNKNOWN, mode);
    if (!db)
    {
        PyErr_SetString(PyExc_TypeError,
                        "File could not be opened");
        return NULL;
    }
    return DBfile_NEW(db);
}

// ****************************************************************************
//  Method:  silo_Create
//
//  Purpose:
//    Encapsulate DBCreate
//
//  Python Arguments:
//    form 1: filename, fileinfo, driver, mode
//    form 2: filename, fileinfo, driver
//    form 3: filename, fileinfo
//
//  Programmer:  Jeremy Meredith
//  Creation:    July 12, 2005
//
// ****************************************************************************
PyObject *silo_Create(PyObject *self, PyObject *args)
{
    char *filename;
    char *info;
    int driver;
    int mode;
    if (!PyArg_ParseTuple(args, "ssii", &filename, &info, &driver, &mode))
    {
        mode = DB_CLOBBER;
        if (!PyArg_ParseTuple(args, "ssi", &filename, &info, &driver))
        {
            driver = DB_PDB;
            if (!PyArg_ParseTuple(args, "ss", &filename, &info))
            {
                PyErr_SetString(PyExc_TypeError,
                                "Create takes 2, 3, or 4 arguments");
                return NULL;
            }
        }
    }

    PyErr_Clear();
    DBfile *db = DBCreate(filename, mode, DB_LOCAL, info, driver);
    if (!db)
    {
        PyErr_SetString(PyExc_TypeError, "File creation failed");
        return NULL;
    }
    DBSetAllowOverwrites(1);
    return DBfile_NEW(db);
}

PyObject *silo_ShowErrors(PyObject *self, PyObject *args)
{
    int emode;
    if (!PyArg_ParseTuple(args, "i", &emode))
    {
        PyErr_SetString(PyExc_TypeError, "ShowErrors takes 1 int argument");
        return NULL;
    }

    PyErr_Clear();
    if (emode == DB_TOP || 
        emode == DB_NONE ||
        emode == DB_ALL ||
        emode == DB_ABORT ||
        emode == DB_ALL_AND_DRVR)
        DBShowErrors(emode, NULL);
    else
    {
        PyErr_SetString(PyExc_TypeError, "Invalid error mode");
        return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

// ****************************************************************************
//  Method:  initSilo
//
//    Called by python to initialize the Silo module.
//
//  Arguments:
//    none
//
//  Programmer:  Jeremy Meredith
//  Creation:    July 12, 2005
//
//  Modifications:
//    Mark C. Miller, Tue Nov 13 10:56:57 PST 2012
//    Added a slew of constants so calllers can properly examine dict
//    contents returned by GetVarInfo method.
//
// ****************************************************************************
#define ADD_CONSTANT(C) { PyObject *sym = PyInt_FromLong(C); PyDict_SetItemString(d, #C, sym); Py_DECREF(sym); }
extern "C"
#if PY_VERSION_GE(3,0,0)
SILOMODULE_API PyObject * PyInit_Silo(void)
#else
void SILOMODULE_API initSilo(void)
#endif
{
    AddMethod("Open", silo_Open,
              "Usage: Open(filename [, DB_READ|DB_APPEND]])");
    AddMethod("Create", silo_Create,
              "Usage: Create(filename , info [, DB_PDB|DB_HDF5 [, DB_CLOBBER|DB_NOCLOBBER]])");
    AddMethod("ShowErrors", silo_ShowErrors,
              "Usage: ShowErrors([DB_TOP|DB_NONE|DB_ALL|DB_ABORT|DB_ALL_AND_DRVR])");
    AddMethod(NULL, NULL);


    PY_SILO_MOD_DEF(siloModule, "Silo", &SiloMethods[0]);

    PyObject *d;
    d = PyModule_GetDict(siloModule);
    SiloError = PyErr_NewException("Silo.SiloException", NULL, NULL);
    PyDict_SetItemString(d, "SiloException", SiloError);
    Py_DECREF(SiloError);

    // Error Modes
    ADD_CONSTANT(DB_TOP);
    ADD_CONSTANT(DB_NONE);
    ADD_CONSTANT(DB_ALL);
    ADD_CONSTANT(DB_ABORT);
    ADD_CONSTANT(DB_ALL_AND_DRVR);

    // File Drivers
    ADD_CONSTANT(DB_PDB);
    ADD_CONSTANT(DB_HDF5);
    ADD_CONSTANT(DB_HDF5_SEC2);
    ADD_CONSTANT(DB_HDF5_STDIO);
    ADD_CONSTANT(DB_HDF5_CORE);
    ADD_CONSTANT(DB_HDF5_LOG);
    ADD_CONSTANT(DB_HDF5_SPLIT);
    ADD_CONSTANT(DB_HDF5_DIRECT);
    ADD_CONSTANT(DB_HDF5_FAMILY);
    ADD_CONSTANT(DB_HDF5_MPIO);
    ADD_CONSTANT(DB_HDF5_MPIOP);
    ADD_CONSTANT(DB_HDF5_MPIP);
    ADD_CONSTANT(DB_HDF5_SILO);

    // File flags
    ADD_CONSTANT(DB_CLOBBER);
    ADD_CONSTANT(DB_NOCLOBBER);
    ADD_CONSTANT(DB_READ);
    ADD_CONSTANT(DB_APPEND);

    // Coordinate type flags
    ADD_CONSTANT(DB_COLLINEAR);
    ADD_CONSTANT(DB_NONCOLLINEAR);
    ADD_CONSTANT(DB_QUAD_RECT);
    ADD_CONSTANT(DB_QUAD_CURV);

    // Centering
    ADD_CONSTANT(DB_NOTCENT);
    ADD_CONSTANT(DB_NODECENT);
    ADD_CONSTANT(DB_ZONECENT);
    ADD_CONSTANT(DB_FACECENT);
    ADD_CONSTANT(DB_BNDCENT);
    ADD_CONSTANT(DB_EDGECENT);
    ADD_CONSTANT(DB_BLOCKCENT);

    // Major order
    ADD_CONSTANT(DB_ROWMAJOR);
    ADD_CONSTANT(DB_COLMAJOR);

    // Coordinate system
    ADD_CONSTANT(DB_CARTESIAN);
    ADD_CONSTANT(DB_CYLINDRICAL);
    ADD_CONSTANT(DB_SPHERICAL);
    ADD_CONSTANT(DB_NUMERICAL);
    ADD_CONSTANT(DB_OTHER);

    // Planar
    ADD_CONSTANT(DB_AREA);
    ADD_CONSTANT(DB_VOLUME);

    // Facetype
    ADD_CONSTANT(DB_RECTILINEAR);
    ADD_CONSTANT(DB_CURVILINEAR);

    // Datatype
    ADD_CONSTANT(DB_INT);
    ADD_CONSTANT(DB_SHORT);
    ADD_CONSTANT(DB_LONG);
    ADD_CONSTANT(DB_LONG_LONG);
    ADD_CONSTANT(DB_FLOAT);
    ADD_CONSTANT(DB_DOUBLE);
    ADD_CONSTANT(DB_CHAR);
    ADD_CONSTANT(DB_NOTYPE);

    ADD_CONSTANT(DB_ON);
    ADD_CONSTANT(DB_OFF);

    ADD_CONSTANT(DB_ABUTTING);
    ADD_CONSTANT(DB_FLOATING);

    ADD_CONSTANT(DB_VARTYPE_SCALAR);
    ADD_CONSTANT(DB_VARTYPE_VECTOR);
    ADD_CONSTANT(DB_VARTYPE_TENSOR);
    ADD_CONSTANT(DB_VARTYPE_SYMTENSOR);
    ADD_CONSTANT(DB_VARTYPE_ARRAY);
    ADD_CONSTANT(DB_VARTYPE_MATERIAL);
    ADD_CONSTANT(DB_VARTYPE_SPECIES);
    ADD_CONSTANT(DB_VARTYPE_LABEL);

    ADD_CONSTANT(DB_ZONETYPE_BEAM);
    ADD_CONSTANT(DB_ZONETYPE_POLYGON);
    ADD_CONSTANT(DB_ZONETYPE_TRIANGLE);
    ADD_CONSTANT(DB_ZONETYPE_QUAD);
    ADD_CONSTANT(DB_ZONETYPE_POLYHEDRON);
    ADD_CONSTANT(DB_ZONETYPE_TET);
    ADD_CONSTANT(DB_ZONETYPE_PYRAMID);
    ADD_CONSTANT(DB_ZONETYPE_PRISM);
    ADD_CONSTANT(DB_ZONETYPE_HEX);

    ADD_CONSTANT(DB_ZONETYPE_QUAD_BEAM);
    ADD_CONSTANT(DB_ZONETYPE_QUAD_TRIANGLE);
    ADD_CONSTANT(DB_ZONETYPE_QUAD_QUAD);
    ADD_CONSTANT(DB_ZONETYPE_QUAD_TET);
    ADD_CONSTANT(DB_ZONETYPE_QUAD_PYRAMID);
    ADD_CONSTANT(DB_ZONETYPE_QUAD_PRISM);
    ADD_CONSTANT(DB_ZONETYPE_QUAD_HEX);

#if PY_VERSION_GE(3,0,0)

    Py_INCREF(&DBfileType);
    if (PyModule_AddObject(siloModule, "DBfile", (PyObject *) &DBfileType) < 0) {
        Py_DECREF(&DBfileType);
        Py_DECREF(siloModule);
        return NULL;
    }


    return siloModule;
#endif
}
