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

#ifndef PY_SILO_H
#define PY_SILO_H

// ****************************************************************************
//  Purpose:
//    Header file for all things in the Silo Python wrapper.
//
//  Programmer:  Jeremy Meredith
//  Creation:    July 12, 2005
//
// ****************************************************************************

#define PY_VERSION_GE(Maj,Min,Mic)  \
        (((PY_MAJOR_VERSION==Maj) && (PY_MINOR_VERSION==Min) && (PY_MICRO_VERSION>=Mic)) || \
         ((PY_MAJOR_VERSION==Maj) && (PY_MINOR_VERSION>Min)) || \
         (PY_MAJOR_VERSION>Maj))

void SiloErrorFunc(const char *errString);

// Python 2/3 compatability macros
// From http://python3porting.com/cextensions.html
#ifndef PyVarObject_HEAD_INIT
    #define PyVarObject_HEAD_INIT(type, size) \
        PyObject_HEAD_INIT(type) size,
#endif

#ifndef Py_TYPE
    #define Py_TYPE(ob) (((PyObject*)(ob))->ob_type)
#endif

#if PY_VERSION_GE(3,0,0)
    #define PY_SILO_MOD_DEF(ob, name, methods)  \
        static struct PyModuleDef moduledef = { \
            .m_base = PyModuleDef_HEAD_INIT,    \
            .m_name = name,                     \
            .m_methods = methods };             \
        ob = PyModule_Create(&moduledef);
#else
    #define PY_SILO_MOD_DEF(ob, name, methods) \
        ob = Py_InitModule(name, methods);
#endif

#if PY_VERSION_GE(3,0,0)
    #define PyInt_FromLong PyLong_FromLong
    #define PyInt_AS_LONG PyLong_AS_LONG
    #define PyInt_Check PyLong_Check
    #define PyString_FromString PyUnicode_FromString
    #define PyString_Check PyUnicode_Check
    #define PyString_FromStringAndSize PyUnicode_FromStringAndSize
    #define PyString_AsString PyUnicode_AsUTF8
    #define PyString_Size PyUnicode_GetLength
#endif


#endif
