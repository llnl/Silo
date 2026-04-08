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
#include "stdlib.h"

#include "pydbfile.h"
#include "pydbtoc.h"
#include "pysilo.h"

#include <string>

using std::string;

#define FREE(M)         if(M){free(M);(M)=NULL;}

// ****************************************************************************
//  Method:  DBfile_DBGetToc
//
//  Purpose:
//    Encapsulates DBGetToc
//
//  Python Arguments:
//    none
//
//  Programmer:  Jeremy Meredith
//  Creation:    July 12, 2005
//
// ****************************************************************************
static PyObject *DBfile_DBGetToc(PyObject *self, PyObject *args)
{
    DBfileObject *obj = (DBfileObject*)self;

    if (!obj->db)
    {
        SiloErrorFunc("This file has been closed.");
        return NULL;
    }

    DBtoc *toc = DBGetToc(obj->db);

    DBtocObject *retval = PyObject_NEW(DBtocObject, &DBtocType);
    if (retval)
    {
        retval->toc = toc;
    }
    return (PyObject*)retval;
}

// ****************************************************************************
//  Method:  DBfile_DBGetVar
//
//  Purpose:
//    Encapsulates DBGetVar
//
//  Python Arguments:
//    form 1: varname
//
//  Programmer:  Jeremy Meredith
//  Creation:    July 12, 2005
//
//  Modifications:
//
//    Mark C. Miller, Tue Aug  5 11:04:14 PDT 2008
//    I modifed case where we're returning a string valued variable to strip
//    off the trailing null character. The PyString_FromStringAndSize method
//    was being given a length argument that included the trailing null and
//    the result was a bit if an odd string in python.
//
//    Mark C. Miller, Wed Nov 14 15:35:44 PST 2012
//    Removed error return case for 'only flat variables.' This also fixes a
//    problem with string object members on HDF5 driver where the funky "'<s>"
//    construct is used for *both* string valued members and variable
//    members whose value is also a string but is the name of another dataset
//    in the file.
//
//    Mark C. Miller, Thu Aug 22 00:34:52 PDT 2024
//    Skip possible empty first and last strings in a string list.
// ****************************************************************************
static PyObject *DBfile_DBGetVar(PyObject *self, PyObject *args)
{
    DBfile *db = ((DBfileObject*)self)->db;

    if (!db)
    {
        SiloErrorFunc("This file has been closed.");
        return NULL;
    }

    char *str, *iestr=0;
    int dontErrInSanityChecks= 0;
    if(!PyArg_ParseTuple(args, "ss", &str, &iestr))
    {
        PyErr_Clear();
        if(!PyArg_ParseTuple(args, "s", &str))
        {
            SiloErrorFunc("A string argument is required.");
            return NULL;
        }
    }
    if (iestr && !strcmp(iestr, "dont-throw-errors-in-sanity-checks"))
        dontErrInSanityChecks = 1;
    char msg[256];
    snprintf(msg, sizeof(msg), "Problem with DBInqVarType   for variable \"%s\"", str);

    int vartype = DBInqVarType(db, str);
    if (vartype != DB_VARIABLE)
    {
        if (!dontErrInSanityChecks)
            SiloErrorFunc(msg);
        return NULL;
    }

    int len = DBGetVarLength(db,str);
    if (len < 0)
    {
        snprintf(&msg[13], sizeof(msg)-13, "DBGetVarLength");
        if (!dontErrInSanityChecks)
            SiloErrorFunc(msg);
        return NULL;
    }
    int type = DBGetVarType(db,str);
    if (type < 0)
    {
        snprintf(&msg[13], sizeof(msg)-13, "DBGetVarType");
        if (!dontErrInSanityChecks)
            SiloErrorFunc(msg);
        return NULL;
    }
    void *var = DBGetVar(db,str);
    if (!var)
    {
        snprintf(&msg[13], sizeof(msg)-13, "DBGetVar");
        if (!dontErrInSanityChecks)
            SiloErrorFunc(msg);
        return NULL;
    }

    if (len == 1 || type == DB_CHAR)
    {
        PyObject *tmp;
        switch (type)
        {
          case DB_INT:
            tmp = PyInt_FromLong((long)*((int*)var)); break;
          case DB_SHORT:
            tmp = PyInt_FromLong((long)*((short*)var)); break;
          case DB_LONG:
            tmp = PyInt_FromLong(*((long*)var)); break;
          case DB_FLOAT:
            tmp = PyFloat_FromDouble((double)*((float*)var)); break;
          case DB_DOUBLE:
            tmp = PyFloat_FromDouble(*((double*)var)); break;
          case DB_CHAR:
            if (len == 1)
                tmp = PyInt_FromLong((long)*((char*)var));
            else
            {
                int narr = -1;
                char **strArr = DBStringListToStringArray((char*)var, &narr, 0);
                if (narr > 0 && strArr)
                {
                    PyObject *tmp2 = PyList_New(0);
                    for (int i = 0; i < narr; i++)
                    {
                        if (i == 0 && strArr[i] && strArr[i][0] == '\0')
                        {
                            FREE(strArr[i]);
                            continue; // skip an empty first entry
                        }
                        if (i == (narr-1) && strArr[i] && strArr[i][0] == '\0')
                        {
                            FREE(strArr[i]);
                            continue; // skip an empty last entry
                        }
                        PyList_Append(tmp2, PyString_FromString(strArr[i]));
                        FREE(strArr[i]);
                    }
                    FREE(strArr);
                    tmp = PyList_AsTuple(tmp2);
                    Py_DECREF(tmp2);
                }
                else
                {
                    // strip trailing null if one exists
                    char *p = (char *) var;
                    if (p[len-1] == '\0') len--;
                    tmp = PyString_FromStringAndSize((char*)var, len);
                }
            }
            break;
          default:
            SiloErrorFunc("Unknown variable type.");
            tmp = NULL;
            break;
        }
        if (var) free(var);
        return tmp;
    }
    else
    {
        PyObject *retval = len>0?PyTuple_New(len):NULL;
        for (int i=0; i<len; i++)
        {    
            PyObject *tmp;
            switch (type)
            {
              case DB_INT:    tmp = PyInt_FromLong((long)((int*)var)[i]); break;
              case DB_SHORT:  tmp = PyInt_FromLong((long)((short*)var)[i]); break;
              case DB_LONG:   tmp = PyInt_FromLong(((long*)var)[i]); break;
              case DB_FLOAT:  tmp = PyFloat_FromDouble((double)((float*)var)[i]); break;
              case DB_DOUBLE: tmp = PyFloat_FromDouble(((double*)var)[i]); break;
              case DB_CHAR:   tmp = PyInt_FromLong((long)((char*)var)[i]); break;
              default:
                SiloErrorFunc("Unknown variable type.");
                return NULL;
            }
            PyTuple_SET_ITEM(retval, i, tmp);
        }
        if (var) free(var);
        return retval;
    }
    SiloErrorFunc("An unknown Silo error occurred.");
    return NULL;
}

// ****************************************************************************
//  Method:  DBfile_DBGetVarInfo
//
//  Purpose: Get metadata for a variable
//
//  Creation Mark C. Miller, Mon Nov 12 11:12:03 PST 2012
//  Plagerized liberally from Silex' SiloObjectView
//  
//  Mark C. Miller, Tue Nov 13 17:29:29 PST 2012
//  Added optional 1/0 flag to descend into variable components and read their
//  data as a tuple member of the returned python dict.
// ****************************************************************************
static PyObject *DBfile_DBGetVarInfo(PyObject *self, PyObject *args)
{
    DBfile *db = ((DBfileObject*)self)->db;

    if (!db)
    {
        SiloErrorFunc("This file has been closed.");
        return NULL;
    }

    char *str;
    int get_data_flag = 0;
    if (!PyArg_ParseTuple(args, "si", &str, &get_data_flag))
    {
        if (!PyArg_ParseTuple(args, "s", &str))
            return NULL;
        else
            PyErr_Clear();
    }

    int vtype = DBInqVarType(db, str);
    if (vtype == DB_INVALID_OBJECT)
    {
        SiloErrorFunc("Silo object not found.");
        return NULL;
    }

    if (vtype == DB_VARIABLE)
    {
        int dims[50];
        int const maxdims = (int) sizeof(dims)/sizeof(dims[0]);
        int ndims = DBGetVarDims(db, str, maxdims, dims);
        int dtype = DBGetVarType(db, str);

        PyObject *dimsTuple = PyTuple_New(ndims);
        for (int i = 0; i < ndims; i++)
            PyTuple_SET_ITEM(dimsTuple, i, PyInt_FromLong((long)dims[i]));

        PyObject *retval = PyDict_New();
        PyDict_SetItemString(retval, "ndims", PyInt_FromLong((long)ndims));
        PyDict_SetItemString(retval, "datatype", PyInt_FromLong((long)dtype));
        PyDict_SetItemString(retval, "dims", dimsTuple);
        Py_DECREF(dimsTuple);
        if (get_data_flag)
        {
            PyObject *argTuple = PyTuple_New(2);
            PyTuple_SET_ITEM(argTuple, 0, PyString_FromString(str));
            PyTuple_SET_ITEM(argTuple, 1, PyString_FromString("dont-throw-errors-in-sanity-checks"));
            PyObject *dobj = DBfile_DBGetVar(self, argTuple);
            Py_DECREF(argTuple);
            if (dobj)
            {
                PyDict_SetItemString(retval, "data", dobj);
                Py_DECREF(dobj);
            }
        }
        return retval;
    }

    //
    // Note that because we read the object through Silo's generic object
    // interface, the Silo library will not be able to correctly apply
    // object-level de-compression algorithms. We could add logic to the
    // implementation of the GetObject method to detect the kind of object
    // being read and, if it is a compressed mesh/var object, do the work
    // necessary to prepare for its decompression. Too much work for now.
    //
    DBobject *silo_obj = DBGetObject(db, str);

    PyObject *retval = PyDict_New();
    PyDict_SetItemString(retval, "name", PyString_FromString(silo_obj->name));
    PyDict_SetItemString(retval, "type", PyString_FromString(silo_obj->type));
    for (int i=0; i<silo_obj->ncomponents; i++)
    {
        PyObject *tmpTuple;
        string compname = silo_obj->comp_names[i];
        string pdbname  = silo_obj->pdb_names[i];
        int type = DBGetComponentType(db, str, compname.c_str());
        void *comp = 0;

        if (type != DB_NOTYPE)
        {
            comp  = DBGetComponent(db, str, compname.c_str());
            if (!comp)
            {
                char msg[256];
                snprintf(msg, sizeof(msg), "Unable to get component \"%s\" for object \%s\"", compname.c_str(), str);
                SiloErrorFunc(msg);
                continue;
            }
        }
        int ival = -1;
        switch (type)
        {
          case DB_INT:
            ival = *((int*)comp);
            PyDict_SetItemString(retval, compname.c_str(), PyInt_FromLong((long)ival));
            break;
          case DB_INTA:
            tmpTuple = PyTuple_New(3);
            PyTuple_SET_ITEM(tmpTuple, 0, PyInt_FromLong((long)((int*)comp)[0]));
            PyTuple_SET_ITEM(tmpTuple, 1, PyInt_FromLong((long)((int*)comp)[1]));
            PyTuple_SET_ITEM(tmpTuple, 2, PyInt_FromLong((long)((int*)comp)[2]));
            PyDict_SetItemString(retval, compname.c_str(), tmpTuple);
            break;
          case DB_SHORT:
            ival = *((short*)comp);
            PyDict_SetItemString(retval, compname.c_str(), PyInt_FromLong((long)ival));
            break;
          case DB_LONG:
            ival = (int) *((long*)comp);
            PyDict_SetItemString(retval, compname.c_str(), PyInt_FromLong((long)ival));
            break;
          case DB_LONG_LONG:
            ival = (int) *((long long*)comp);
            PyDict_SetItemString(retval, compname.c_str(), PyInt_FromLong((long)ival));
            break;
          case DB_FLOAT:
            PyDict_SetItemString(retval, compname.c_str(), PyFloat_FromDouble((double)*((float*)comp)));
            break;
          case DB_FLOATA:
            tmpTuple = PyTuple_New(3);
            PyTuple_SET_ITEM(tmpTuple, 0, PyFloat_FromDouble((double)((float*)comp)[0]));
            PyTuple_SET_ITEM(tmpTuple, 1, PyFloat_FromDouble((double)((float*)comp)[1]));
            PyTuple_SET_ITEM(tmpTuple, 2, PyFloat_FromDouble((double)((float*)comp)[2]));
            PyDict_SetItemString(retval, compname.c_str(), tmpTuple);
            break;
          case DB_DOUBLE:
            PyDict_SetItemString(retval, compname.c_str(), PyFloat_FromDouble(*((double*)comp)));
            break;
          case DB_DOUBLEA:
            tmpTuple = PyTuple_New(3);
            PyTuple_SET_ITEM(tmpTuple, 0, PyFloat_FromDouble((double)((double*)comp)[0]));
            PyTuple_SET_ITEM(tmpTuple, 1, PyFloat_FromDouble((double)((double*)comp)[1]));
            PyTuple_SET_ITEM(tmpTuple, 2, PyFloat_FromDouble((double)((double*)comp)[2]));
            PyDict_SetItemString(retval, compname.c_str(), tmpTuple);
            break;
          case DB_CHAR:
            if (*((char*)comp)== 0)
                PyDict_SetItemString(retval, compname.c_str(), PyString_FromString(""));
            else
                PyDict_SetItemString(retval, compname.c_str(), PyString_FromString((char*)comp));
            break;
          case DB_NOTYPE:
            break;
          default:
            string valStr = std::string(pdbname.c_str());
            if (pdbname.find("'<s>") == 0)
            {
                int len = pdbname.length();
                valStr = string((const char*)(pdbname.c_str()),4,len-5);
            }
            bool is_meta_data = false;
            if ((compname == "align") || (compname == "baseindex") || (compname == "dims") ||
                (compname == "dtime") || (compname == "max_extents") || (compname == "max_index") ||
                (compname == "min_extents") || (compname == "min_index") || (compname == "time"))
                is_meta_data = true;
            if (get_data_flag || is_meta_data)
            {
                PyObject *argTuple = PyTuple_New(2);
                PyTuple_SET_ITEM(argTuple, 0, PyString_FromString(valStr.c_str()));
                PyTuple_SET_ITEM(argTuple, 1, PyString_FromString("dont-throw-errors-in-sanity-checks"));
                PyObject *dobj = DBfile_DBGetVar(self, argTuple);
                Py_DECREF(argTuple);
                if (dobj)
                {
                    PyDict_SetItemString(retval, compname.c_str(), dobj);
                    Py_DECREF(dobj);
                }
                else
                {
                    PyDict_SetItemString(retval, compname.c_str(), PyString_FromString(valStr.c_str()));
                }
            }
            else
            {
                PyDict_SetItemString(retval, compname.c_str(), PyString_FromString(valStr.c_str()));
            }
            break;
        }

        // No such call as "DBFreeComponent".  Maybe there should be one!
        if (comp) free(comp);
        comp = NULL;

    }
    DBFreeObject(silo_obj);

    return retval;
}

/* Returns 1 (true) if all items have the same type.
   If allow_subclasses != 0, accepts subtypes of the first element's type.
   On non-iterable, sets TypeError and returns -1.
   If out_type != NULL and seq non-empty, *out_type is set (borrowed). */
static int
all_same_type(PyObject *seq, int allow_subclasses, PyTypeObject **out_type)
{
    PyObject *fast = PySequence_Fast(seq, "expected a sequence or iterable");
    if (!fast)
    {
        PyErr_Clear();
        return -1;
    }

    Py_ssize_t n = PySequence_Fast_GET_SIZE(fast);
    if (n == 0) {
        if (out_type) *out_type = NULL;
        Py_DECREF(fast);
        return 1;  /* define empty as "homogeneous" */
    }

    PyObject *first = PySequence_Fast_GET_ITEM(fast, 0);  // borrowed
    PyTypeObject *t0 = Py_TYPE(first);

    int ok = 1;
    for (Py_ssize_t i = 1; i < n; i++) {
        PyObject *it = PySequence_Fast_GET_ITEM(fast, i);  // borrowed
        if (allow_subclasses) {
            if (!PyObject_TypeCheck(it, t0)) { ok = 0; break; }
        } else {
            if (Py_TYPE(it) != t0) { ok = 0; break; }
        }
    }

    Py_DECREF(fast);
    if (ok && out_type) *out_type = t0;
    return ok;
}

// ****************************************************************************
//  Method:  DBfile_DBWrite
//
//  Purpose:
//    Encapsulates DBWrite
//
//  Python Arguments:
//    form 1: varname, integer
//    form 2: varname, real
//    form 3: varname, string
//    form 4: varname, tuple
//
//  Programmer:  Jeremy Meredith
//  Creation:    July 12, 2005
//
//  Modifications
//  Mark C. Miller, Thu Dec 20 00:05:41 PST 2012
//  Adjust parsing logic to avoid deprecation warning for parsing a float into
//  an integer variable.
// ****************************************************************************
static PyObject *DBfile_DBWrite(PyObject *self, PyObject *args)
{
    DBfile *db = ((DBfileObject*)self)->db;

    if (!db)
    {
        SiloErrorFunc("This file has been closed.");
        return NULL;
    }

    char *str;
    int len = 0, ndims, dtype = DB_NOTYPE, err = 1;
    int dims[10];
    int ivar;
    double dvar;
    char *svar;
    char *data = 0;
    int dsize[DB_NOTYPE+1];

    dsize[DB_INT] = sizeof(int);
    dsize[DB_SHORT] = sizeof(short);
    dsize[DB_LONG] = sizeof(long);
    dsize[DB_FLOAT] = sizeof(float);
    dsize[DB_DOUBLE] = sizeof(double);
    dsize[DB_CHAR] = sizeof(char);
    dsize[DB_LONG_LONG] = sizeof(long long);
    dsize[DB_NOTYPE] = 0;

    PyObject *pydata = 0, *pydims;
    if (PyArg_ParseTuple(args, "si", &str, &ivar))
    {
        ndims = 1;
        dims[0] = len = 1;
        data = (char*) &ivar;
        dtype = DB_INT;
    }
    else if (PyArg_ParseTuple(args, "sd", &str, &dvar) ||
             PyArg_ParseTuple(args, "sO!", &str, &PyFloat_Type, &dvar))
    {
        ndims = 1;
        dims[0] = len = 1;
        if (dvar == (int) dvar)
        {
            ivar = (int) dvar;
            data = (char*) &ivar;
            dtype = DB_INT;
        }
        else
        {
            data = (char*) &dvar;
            dtype = DB_DOUBLE;
        }
    }
    else if (PyArg_ParseTuple(args, "ss", &str, &svar))
    {
        data = svar;
        ndims = 1;
        dims[0] = len = strlen(svar);
        dtype = DB_CHAR;
    }
    else if (PyArg_ParseTuple(args, "sO", &str, &pydata))
    {
        if (!PyTuple_Check(pydata) && !PyList_Check(pydata))
        {
            PyErr_SetString(PyExc_TypeError, "For 2-arg: only tuple or list as second arg");
            return NULL;
        }

        ndims = 1;
        dims[0] = len = (int) PySequence_Length(pydata);
        if (dims[0] < 1)
        {
            PyErr_SetString(PyExc_TypeError, "Tuple or list must have size > 0");
            return NULL;
        }

        PyObject *item = PySequence_GetItem(pydata, 0);
        if (PyString_Check(item) && PyString_Size(item)==1)
            dtype = DB_CHAR;
        else if (PyInt_Check(item))
            dtype = DB_INT;
        else if (PyFloat_Check(item))
            dtype = DB_DOUBLE;
        else
        {
            PyErr_SetString(PyExc_TypeError, "For 2-arg: only int or double tuples are supported");
            return NULL;
        }
    }
    else if (PyArg_ParseTuple(args, "sOOi", &str, &pydata, &pydims, &dtype))
    {
        if (!PyTuple_Check(pydata) && !PyList_Check(pydata))
        {
            PyErr_SetString(PyExc_TypeError, "Only tuple or list for data");
            return NULL;
        }
        if (PySequence_Length(pydata) < 1)
        {
            PyErr_SetString(PyExc_TypeError, "Data tuple or list must have size > 0");
            return NULL;
        }

        if (!PyTuple_Check(pydims) && !PyList_Check(pydims))
        {
            PyErr_SetString(PyExc_TypeError, "Only tuple or list for dimensions");
            return NULL;
        }
        if (PySequence_Length(pydims) < 1)
        {
            PyErr_SetString(PyExc_TypeError, "Dimensions tuple or list must have size > 0");
            return NULL;
        }

        if (!(dtype == DB_CHAR || dtype == DB_SHORT || dtype == DB_INT || dtype == DB_LONG ||
            dtype == DB_LONG_LONG || dtype == DB_FLOAT || dtype == DB_DOUBLE))
        {
            PyErr_SetString(PyExc_TypeError, "Invalid Silo data type");
            return NULL;
        }

        ndims = len = (int) PySequence_Length(pydims);
        int nvals = 1;
        for (int i = 0; i < len; i++)
        {
            PyObject *item = PySequence_GetItem(pydims, i);
            if (!PyInt_Check(item))
            {
                PyErr_SetString(PyExc_TypeError, "Dimensions must all be integer");
                return NULL;
            }
            dims[i] = int(PyInt_AS_LONG(item));
            if (dims[i] <= 0)
            {
                PyErr_SetString(PyExc_TypeError, "Dimension data must all be positive");
                return NULL;
            }
            nvals *= dims[i];
        }

        len = (int) PySequence_Length(pydata);
        if (nvals != len)
        {
            {
                PyErr_SetString(PyExc_TypeError, "Data tuple or list size does not match dimensions");
                return NULL;
            }
        }
    }
    else
    {
        PyErr_SetString(PyExc_TypeError, "Problem parsing arguments");
        return NULL;
    }

    if (len && pydata)
    {
        data = new char[len * dsize[dtype]];
        char *p = (char*) data;
        for (int i = 0; i < len; i++)
        {
            PyObject *item = PySequence_GetItem(pydata, i);
            switch (dtype)
            {
                case DB_CHAR:
                {
                    if (!PyString_Check(item) || PyString_Size(item)>1)
                    {
                        PyErr_SetString(PyExc_TypeError, "Data is not a single DB_CHAR");
                        goto fail_exit;
                    }
                    *p = *(PyString_AsString(item));
                    break;
                }
                case DB_SHORT:
                {
                    if (!PyInt_Check(item))
                    {
                        PyErr_Format(PyExc_TypeError,
                            "For DB_SHORT data, hit non-Python integer at index %d of %d", i, len);
                        goto fail_exit;
                    }
                    short *ps = (short*)p;
                    *ps = (short) PyLong_AsLong(item);
                    break;
                }
                case DB_INT:
                {
                    if (!PyInt_Check(item))
                    {
                        PyErr_Format(PyExc_TypeError,
                            "For DB_INT data, hit non-Python integer at index %d of %d", i, len);
                        goto fail_exit;
                    }
                    int *pi = (int*)p;
                    *pi = (int) PyLong_AsLong(item);
                    break;
                }
                case DB_LONG:
                {
                    if (!PyLong_Check(item))
                    {
                        PyErr_Format(PyExc_TypeError,
                            "For DB_LONG data, hit non-Python integer at index %d of %d", i, len);
                        goto fail_exit;
                    }
                    long *pl = (long*)p;
                    *pl = PyLong_AsLong(item);
                    break;
                }
                case DB_LONG_LONG:
                {
                    if (!PyLong_Check(item))
                    {
                        PyErr_Format(PyExc_TypeError,
                            "For DB_LONG_LONG data, hit non-Python integer at index %d of %d", i, len);
                        goto fail_exit;
                    }
                    long long *pll = (long long*)p;
                    *pll = (long long) PyFloat_AsDouble(item);
                    break;
                }
                case DB_FLOAT:
                {
                    if (!PyFloat_Check(item))
                    {
                        PyErr_Format(PyExc_TypeError,
                            "For DB_FLOAT data, hit non-Python float at index %d of %d", i, len);
                        goto fail_exit;
                    }
                    float *pf = (float*)p;
                    *pf = (float) PyFloat_AsDouble(item);
                    break;
                }
                case DB_DOUBLE:
                {
                    if (!PyFloat_Check(item))
                    {
                        PyErr_Format(PyExc_TypeError,
                            "For DB_DOUBLE data, hit non-Python float at index %d of %d", i, len);
                        goto fail_exit;
                    }
                    double *pd = (double*)p;
                    *pd = PyFloat_AsDouble(item);
                    break;
                }
                default:
                    break;
            }
            p += dsize[dtype];
        }
    }

    err = DBWrite(db, str, data, dims, ndims, dtype);

fail_exit:

    if (data && data != (char*)&ivar && data != (char*)&dvar && data != svar)
        delete [] data;

    if (err == 1)
        return NULL;

    if (err == -1)
    {
        PyErr_SetString(PyExc_TypeError, "DBWrite failed");
        return NULL;
    }
    
    PyErr_Clear();
    Py_INCREF(Py_None);
    return Py_None;
}

// ****************************************************************************
//  Method:  DBfile_DBWriteObject
//
//  Purpose: Generalized method for writing silo objects.
//
//  Python Arguments:
//    form 1: object name, python dictionary (with problem sized arrays as
//    members)
// ****************************************************************************
static PyObject *DBfile_DBWriteObject(PyObject *self, PyObject *args)
{
    DBfile *db = ((DBfileObject*)self)->db;

    if (!db)
    {
        SiloErrorFunc("This file has been closed.");
        return NULL;
    }

    char *objname;
    PyDictObject *dictobj;
    if (!PyArg_ParseTuple(args, "sO!", &objname, &PyDict_Type, &dictobj)) return NULL;

    int ncomps = PyDict_Size((PyObject*)dictobj);
    if (!ncomps) return NULL;
    int objtype = DBGetObjtypeTag(PyString_AsString(PyDict_GetItemString((PyObject*)dictobj, "type")));
    DBobject *siloobj = DBMakeObject(objname, objtype, ncomps);
    PyObject *key, *value;
#if PY_VERSION_GE(2,5,0)
    Py_ssize_t pos = 0;
#else
    int pos = 0;
#endif
    while (PyDict_Next((PyObject*)dictobj, &pos, &key, &value))
    {
        // skip name and type values
        if (!strncmp(PyString_AsString(key), "type", 4))
            continue;
        if (!strncmp(PyString_AsString(key), "name", 4))
            continue;

        // handle some special cases Silo treats as either float or double explicitly
        if (!strncmp(PyString_AsString(key), "time", 4) ||
            !strncmp(PyString_AsString(key), "align", 5))
        {
            DBAddFltComponent(siloobj, PyString_AsString(key), (float) PyFloat_AS_DOUBLE(value));
            continue;
        }
        if (!strncmp(PyString_AsString(key), "missing_value", 13) ||
            !strncmp(PyString_AsString(key), "extents", 7) ||
            !strncmp(PyString_AsString(key), "dtime", 5))
        {
            DBAddDblComponent(siloobj, PyString_AsString(key), PyFloat_AS_DOUBLE(value));
            continue;
        }

        if (PyInt_Check(value)) // scalar integer
            DBAddIntComponent(siloobj, PyString_AsString(key), PyInt_AS_LONG(value));
        else if (PyFloat_Check(value)) // scalar floating point which we treat as double
            DBAddDblComponent(siloobj, PyString_AsString(key), PyFloat_AS_DOUBLE(value));
        else if (PyString_Check(value)) // scalar string
            DBAddStrComponent(siloobj, PyString_AsString(key), PyString_AsString(value));
        else if (PySequence_Check(value)) // tuple or list (e.g. a variable)
        {
            long len = PySequence_Size(value);
            PyObject *itemZero = PySequence_GetItem(value, 0);

            if (PyString_Check(itemZero))
            {
                int len2;
                long count[1];
                char *tmp = 0;
                char const **vals = new char const *[len];
                for (int i = 0; i < len; i++)
                {
                    PyObject *item = PySequence_GetItem(value, i);
                    vals[i] = PyString_AsString(item);
                    Py_DECREF(item);
                }
                DBStringArrayToStringList((char const * const *)vals, len, &tmp, &len2);
                count[0] = (long) len2;
                DBWriteComponent(db, siloobj, PyString_AsString(key), objname, "char", tmp, 1, count);
                delete [] vals;
                FREE(tmp);
            }
            else
            {
                bool allint = true;
                for (int i = 0; i < len && allint; i++)
                {
                    PyObject *item = PySequence_GetItem(value, i);
                    if (PyFloat_Check(item))
                        allint = false;
                    Py_DECREF(item);
                }
                if (allint)
                {
                    int *vals = new int[len];
                    for (int i = 0; i < len; i++)
                    {
                        PyObject *item = PySequence_GetItem(value, i);
                        if (PyFloat_Check(item))
                        {
                            double dval = PyFloat_AS_DOUBLE(item);
                            vals[i] = (int) dval;
                        }
                        else
                        {
                            vals[i] = PyInt_AS_LONG(item);
                        }
                        Py_DECREF(item);
                    }
                    DBWriteComponent(db, siloobj, PyString_AsString(key), objname, "integer", vals, 1, &len);
                    delete [] vals;
                }
                else
                {
                    double *vals = new double[len];
                    for (int i = 0; i < len; i++)
                    {
                        PyObject *item = PySequence_GetItem(value, i);
                        if (PyInt_Check(item))
                            vals[i] = (double) PyInt_AS_LONG(item);
                        else
                            vals[i] = PyFloat_AS_DOUBLE(item);
                        Py_DECREF(item);
                    }
                    DBWriteComponent(db, siloobj, PyString_AsString(key), objname, "double", vals, 1, &len);
                    delete [] vals;
                }
            }
        }
        // don't know what to do with this python type. So, we'll add it to the Silo
        // object as a *string* with value indicating it is a type we can't handle.
        else
        {
            DBAddStrComponent(siloobj, PyString_AsString(key), "unmappable python type");
        }
    }

    // Ok, we've built up the object, now write it
    DBWriteObject(db, siloobj, 1);
    DBFreeObject(siloobj);

    PyErr_Clear();
    Py_INCREF(Py_None);
    return Py_None;

}

// ****************************************************************************
//  Method:  DBfile_DBMkDir
//
//  Purpose:
//    Encapsulates DBMkDir
//
//  Python Arguments:
//    form 1: dirname
//
//  Programmer:  Jeremy Meredith
//  Creation:    July 12, 2005
//
// ****************************************************************************
static PyObject *DBfile_DBMkDir(PyObject *self, PyObject *args)
{
    DBfile *db = ((DBfileObject*)self)->db;

    if (!db)
    {
        SiloErrorFunc("This file has been closed.");
        return NULL;
    }

    char *str;
    if(!PyArg_ParseTuple(args, "s", &str))
        return NULL;

    if (DBMkDir(db, str))
    {
        SiloErrorFunc("Could not make the directory.");
        return NULL;
    }
    else
    {
        Py_INCREF(Py_None);
        return Py_None;
    }
}

// ****************************************************************************
//  Method:  DBfile_DBSetDir
//
//  Purpose:
//    Encapsulates DBSetDir
//
//  Python Arguments:
//    form 1: dirname
//
//  Programmer:  Jeremy Meredith
//  Creation:    July 12, 2005
//
// ****************************************************************************
static PyObject *DBfile_DBSetDir(PyObject *self, PyObject *args)
{
    DBfile *db = ((DBfileObject*)self)->db;

    if (!db)
    {
        SiloErrorFunc("This file has been closed.");
        return NULL;
    }

    char *str;
    if(!PyArg_ParseTuple(args, "s", &str))
        return NULL;

    if (DBSetDir(db, str))
    {
        SiloErrorFunc("Could not change directories.");
        return NULL;
    }
    else
    {
        Py_INCREF(Py_None);
        return Py_None;
    }
}

// ****************************************************************************
//  Method:  DBfile_DBClose
//
//  Purpose:
//    Encapsulates DBClose
//
//  Python Arguments:
//    none
//
//  Programmer:  Jeremy Meredith
//  Creation:    July 12, 2005
//
// ****************************************************************************
static PyObject *DBfile_DBClose(PyObject *self, PyObject *args)
{
    DBfile *db = ((DBfileObject*)self)->db;

    if (!db)
    {
        SiloErrorFunc("This file has been closed.");
        return NULL;
    }

    if(args && !PyArg_ParseTuple(args, ""))
        return NULL;

    if (DBClose(db))
    {
        SiloErrorFunc("Could not close the file.");
        return NULL;
    }
    else
    {
        ((DBfileObject*)self)->db = NULL;
        Py_INCREF(Py_None);
        return Py_None;
    }
}

// ****************************************************************************
//  DBfile method definitions  
//
//  Programmer:  Jeremy Meredith
//  Creation:    July 12, 2005
//
// ****************************************************************************
static struct PyMethodDef DBfile_methods[] = {
    {"GetToc", DBfile_DBGetToc, METH_VARARGS,
        "Return the table of contents for the current working directory of the "
        "Silo file as a struct-like object. For example...\n"
        ">>> db = Silo.Open('globe.silo')\n"
        ">>> x = db.GetToc()\n"
        ">>> x.nucdvar\n"
        "7\n"
        ">>> x.ucdvar_names\n"
        "('dx', 'dy', 'dz', 't', 'u', 'v', 'w')\n"},
    {"GetVar", DBfile_DBGetVar, METH_VARARGS,
        "Return a scalar Silo variable as a python scalar value. For example...\n"
        ">>> db = Silo.Open('globe.silo')\n"
        ">>> x = db.GetVar('cycle')\n"
        ">>> print(x)\n"
        "48\n"
        "If you want the data type and dimensions of the raw variable, use GetVarInfo().\n"},
    {"GetVarInfo", DBfile_DBGetVarInfo, METH_VARARGS,
        "Return either metadata or metadata+rawdata for any Silo object. For example...\n"
        ">>> db = Silo.Open('globe.silo')\n"
        ">>> dx_meta = db.GetVarInfo('dx', 0) # use 0 to get just meta data\n"
        ">>> print(dx_meta)\n"
        "{'name': 'dx', 'type': 'ucdvar', 'meshid': 'mesh1', 'value0': '/dx_data',\n"
        " 'ndims': 3, 'nvals': 1, 'nels': 1200, 'centering': 111, 'origin': 0, 'mixlen': 0,\n"
        " 'datatype': 20, 'time': '/time', 'cycle': 0, 'use_specmf': -1000}\n"
        ">>> dx_meta_and_raw = db.GetVarInfo('dx', 1) # use non-zero to also get raw data\n"
        ">>> print(dx_meta_and_raw)\n"
        "{'name': 'dx', 'type': 'ucdvar', 'meshid': 'mesh1',\n"
        " 'value0': (0.125606125, 0.113311, 0.089924125, 0.057734875, ...\n"
        "\n"
        "If you use GetVarInfo('v',0) on a raw variable, it will return a python dictionary\n"
        "object of the form {'datatype': 16, 'dims': (3,), 'ndims': 1} holding the\n"
        "variable's datatype and dimensions. If you pass a '1' for the second argument,\n"
        "it will return a python dictionary of the form\n"
        "{'datatype': 16, 'dims': (3,), 'ndims': 1, 'data': (...)}\n"},
    {"Write", DBfile_DBWrite, METH_VARARGS,
        "Write a miscellaneous scalar or array to a Silo file. For example...\n"
        ">>> db = Silo.Create('foo.silo', 'no comment', Silo.DB_PDB, Silo.DB_CLOBBER)\n"
        ">>> x=(1,2,3,4)\n"
        ">>> db.Write('x', x)\n"
        ">>> db.Close()\n"},
    {"WriteObject", DBfile_DBWriteObject, METH_VARARGS,
        "Write a Silo object to a Silo file. For example...\n"
        ">>> nodelist = (0, 1, 2, 3, 4, 5, 6, 7)\n"
        ">>> shapecnt = (1,)\n"
        ">>> shapesize = (8,)\n"
        ">>> zonelist = {'name':'zonelist','type':'DBzonelist','ndims':3,'nzones':1,'nshapes':1,\\\n"
        "'shapecnt':shapecnt,'shapesize':shapesize,'lnodelist':len(nodelist),'nodelist':nodelist}\n"
        ">>> db.WriteObject('zonelist', zonelist)\n"},
    {"MkDir", DBfile_DBMkDir, METH_VARARGS,
        "Create a directory in the Silo file. Works with absolute or relative path names."},
    {"SetDir", DBfile_DBSetDir, METH_VARARGS,
        "Set the current working directory of the Silo file. Works with absolute or relative path names"},
    {"Close", DBfile_DBClose, METH_VARARGS},
    {NULL, NULL}
};

// ****************************************************************************
//  Method:  DBfile_dealloc
//
//  Purpose:
//    Deallocate the object.
//
//  Arguments:
//    none
//
//  Programmer:  Jeremy Meredith
//  Creation:    July 12, 2005
//
// ****************************************************************************
static void DBfile_dealloc(PyObject *self)
{
    DBfile_DBClose(self, 0);
    PyObject_Del(self);
}

// ****************************************************************************
//  Method:  DBfile_as_string
//
//  Purpose:
//    Convert the DBfileObject to a string representation.
//
//  Arguments:
//    s          the target string, with space already allocated
//
//  Programmer:  Jeremy Meredith
//  Creation:    July 12, 2005
//
// ****************************************************************************
#define FSTRSIZE 1000
static void DBfile_as_string(PyObject *self, char *s)
{
    DBfileObject *obj = (DBfileObject*)self;
    if (obj->db)
        snprintf(s, FSTRSIZE, "<DBfile object, filename='%s'>", obj->db->pub.name);
    else
        snprintf(s, FSTRSIZE, "<closed DBfile object>");
}

// ****************************************************************************
//  Method:  DBfile_str
//
//  Purpose:
//    Convert the DBfileObject to a PyString
//
//  Arguments:
//    none
//
//  Programmer:  Jeremy Meredith
//  Creation:    July 12, 2005
//
// ****************************************************************************
static PyObject *DBfile_str(PyObject *self)
{
    char str[FSTRSIZE];
    DBfile_as_string(self, str);
    return PyString_FromString(str);
}

// ****************************************************************************
//  Method:  DBfile_print
//
//  Purpose:
//    Print the DBfileObject into a file as text
//
//  Arguments:
//    fp         the file pointer
//    flags      (unused)
//
//  Programmer:  Jeremy Meredith
//  Creation:    July 12, 2005
//
// ****************************************************************************
static int DBfile_print(PyObject *self, FILE *fp, int flags)
{
    char str[FSTRSIZE];
    DBfile_as_string(self, str);
    fprintf(fp, "%s\n", str);
    return 0;
}

// ****************************************************************************
//  Method: DBfile_getattr 
//
//  Purpose:
//    Return an attribute by name.  There is only one attribute of a
//    DBfile, which is its filename.
//
//  Arguments:
//    name       the name of the attribute to return
//
//  Programmer:  Jeremy Meredith
//  Creation:    July 12, 2005
//
// ****************************************************************************
static PyObject *DBfile_getattr(PyObject *self, char *name)
{
    int i;
    DBfileObject *obj = (DBfileObject*)self;

    if (!obj->db)
    {
        SiloErrorFunc("This file has been closed.");
        return NULL;
    }

    if (!strcmp(name, "filename"))
        return PyString_FromString(obj->db->pub.name);

#if PY_VERSION_GE(3,0,0)
    if (!strcmp(name, "__dict__"))
    {
        PyObject *result= PyDict_New();
        for (i = 0; DBfile_methods[i].ml_meth; i++)
            PyDict_SetItem(result, PyString_FromString(DBfile_methods[i].ml_name), PyString_FromString(DBfile_methods[i].ml_name));
        Py_INCREF(result);
        return result;
    }

    for (i = 0; DBfile_methods[i].ml_name; i++)
    {
        if (!strcmp(name, DBfile_methods[i].ml_name))
        {
            PyObject *result = PyCFunction_New(&DBfile_methods[i], self);
            if (result == NULL)
                result = Py_None;
            Py_INCREF(result);
            return result;
        }
    }
    return PyObject_GenericGetAttr(self, PyString_FromString(name));
#else
    return Py_FindMethod(DBfile_methods, self, name);
#endif

}

// ****************************************************************************
//  DBfile Python Type Object
//
//  Programmer:  Jeremy Meredith
//  Creation:    July 12, 2005
//
// ****************************************************************************
PyTypeObject DBfileType =
{
    //
    // Type header
    //
    PyVarObject_HEAD_INIT(&PyType_Type,0)
    "DBfile",                            // tp_name
    sizeof(DBfileObject),                // tp_basicsize
    0,                                   // tp_itemsize

    //
    // Standard methods
    //
    (destructor)DBfile_dealloc,          // tp_dealloc
    (printfunc)DBfile_print,             // tp_print
    (getattrfunc)DBfile_getattr,         // tp_getattr
    0,                                   // tp_setattr -- this object is read-only
    0,                                   // tp_compare -- removed in python 3
    (reprfunc)0,                         // tp_repr

    //
    // Type categories
    //
    0,                                   // tp_as_number
    0,                                   // tp_as_sequence
    0,                                   // tp_as_mapping

    //
    // More methods
    //
    0,                                   // tp_hash
    0,                                   // tp_call
    (reprfunc)DBfile_str,                // tp_str
    0,                                   // tp_getattro
    0,                                   // tp_setattro
    0,                                   // tp_as_buffer
    0,                                   // tp_flags
    "Wrapper for a Silo DBfile object.", // tp_doc
    0,                                   // tp_traverse
    0,                                   // tp_clear
    0,                                   // tp_richcompare
    0                                    // tp_weaklistoffset
};

// ****************************************************************************
//  Method:  DBfile_NEW
//
//  Purpose:
//    Allocate and initialize a DBfileObject.
//
//  Arguments:
//    init       the initial value
//
//  Programmer:  Jeremy Meredith
//  Creation:    July 12, 2005
//
// ****************************************************************************
PyObject *DBfile_NEW(DBfile *init)
{
    DBfileObject *obj = PyObject_NEW(DBfileObject, &DBfileType);
    if (obj)
    {
        obj->db = init;
    }
    return (PyObject*)obj;
}

// ****************************************************************************
//  Method:  DBfile_NEW
//
//  Purpose:
//    Allocate and initialize a DBfileObject with default values.
//
//  Python Arguments:
//    none
//
//  Programmer:  Jeremy Meredith
//  Creation:    July 12, 2005
//
// ****************************************************************************
PyObject *DBfile_new(PyObject *self, PyObject *args)
{
    return DBfile_NEW(NULL);
}

