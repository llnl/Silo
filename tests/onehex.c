/*
Copyright (C) 1994-2016 Lawrence Livermore National Security, LLC.
LLNL-CODE-425250.
All rights reserved.

This file is part of Silo. For details, see silo.llnl.gov.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

   * Redistributions of source code must retain the above copyright
     notice, this list of conditions and the disclaimer below.
   * Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the disclaimer (as noted
     below) in the documentation and/or other materials provided with
     the distribution.
   * Neither the name of the LLNS/LLNL nor the names of its
     contributors may be used to endorse or promote products derived
     from this software without specific prior written permission.

THIS SOFTWARE  IS PROVIDED BY  THE COPYRIGHT HOLDERS  AND CONTRIBUTORS
"AS  IS" AND  ANY EXPRESS  OR IMPLIED  WARRANTIES, INCLUDING,  BUT NOT
LIMITED TO, THE IMPLIED  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A  PARTICULAR  PURPOSE ARE  DISCLAIMED.  IN  NO  EVENT SHALL  LAWRENCE
LIVERMORE  NATIONAL SECURITY, LLC,  THE U.S.  DEPARTMENT OF  ENERGY OR
CONTRIBUTORS BE LIABLE FOR  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR  CONSEQUENTIAL DAMAGES  (INCLUDING, BUT NOT  LIMITED TO,
PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS  OF USE,  DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER  IN CONTRACT, STRICT LIABILITY,  OR TORT (INCLUDING
NEGLIGENCE OR  OTHERWISE) ARISING IN  ANY WAY OUT  OF THE USE  OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

This work was produced at Lawrence Livermore National Laboratory under
Contract  No.   DE-AC52-07NA27344 with  the  DOE.  Neither the  United
States Government  nor Lawrence  Livermore National Security,  LLC nor
any of  their employees,  makes any warranty,  express or  implied, or
assumes   any   liability   or   responsibility  for   the   accuracy,
completeness, or usefulness of any information, apparatus, product, or
process  disclosed, or  represents  that its  use  would not  infringe
privately-owned   rights.  Any  reference   herein  to   any  specific
commercial products,  process, or  services by trade  name, trademark,
manufacturer or otherwise does not necessarily constitute or imply its
endorsement,  recommendation,   or  favoring  by   the  United  States
Government or Lawrence Livermore National Security, LLC. The views and
opinions  of authors  expressed  herein do  not  necessarily state  or
reflect those  of the United  States Government or  Lawrence Livermore
National  Security, LLC,  and shall  not  be used  for advertising  or
product endorsement purposes.
*/
#include <silo.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <float.h>

#include <config.h>
#ifdef HAVE_HDF5_H
/* Define this symbol BEFORE including hdf5.h to indicate the HDF5 code
   in this file uses version 1.6 of the HDF5 API. This is harmless for
   versions of HDF5 before 1.8 and ensures correct compilation with
   version 1.8 and thereafter. When, and if, the HDF5 code in this file
   is explicitly upgraded to the 1.8 API, this symbol should be removed. */
#define H5_USE_16_API
#include <hdf5.h> /* for testing custom fapl/fcpl options */
#endif

#define ASSERT(PRED) if(!(PRED)){fprintf(stderr,"Assertion \"%s\" at line %d failed\n",#PRED,__LINE__);abort();}

#define IND(i,j) i-1][j-1

#define matrix_assign(matrix,a11,a12,a13,a14,a21,a22,a23,a24,a31,a32,a33,a34,a41,a42,a43,a44)         \
   {                                                                          \
   matrix [IND(1,1)] = a11 ;                                              \
   matrix [IND(1,2)] = a12 ;                                              \
   matrix [IND(1,3)] = a13 ;                                              \
   matrix [IND(1,4)] = a14 ;                                              \
   matrix [IND(2,1)] = a21 ;                                              \
   matrix [IND(2,2)] = a22 ;                                              \
   matrix [IND(2,3)] = a23 ;                                              \
   matrix [IND(2,4)] = a24 ;                                              \
   matrix [IND(3,1)] = a31 ;                                              \
   matrix [IND(3,2)] = a32 ;                                              \
   matrix [IND(3,3)] = a33 ;                                              \
   matrix [IND(3,4)] = a34 ;                                              \
   matrix [IND(4,1)] = a41 ;                                              \
   matrix [IND(4,2)] = a42 ;                                              \
   matrix [IND(4,3)] = a43 ;                                              \
   matrix [IND(4,4)] = a44 ;                                              \
   }

#define matrix_mult(matrixa, matrixb, matrixc)                                \
   {                                                                          \
   for (i = 1; i < 5; i++) {                                                  \
      for (j = 1; j < 5; j++) {                                               \
         matrixc [IND(i,j)] = matrixa [IND(i,1)] * matrixb [IND(1,j)] + \
                                  matrixa [IND(i,2)] * matrixb [IND(2,j)] + \
                                  matrixa [IND(i,3)] * matrixb [IND(3,j)] + \
                                  matrixa [IND(i,4)] * matrixb [IND(4,j)] ; \
         }                                                                    \
      }                                                                       \
   }

#ifndef M_PI        /* yea, Solaris 5 */
#define M_PI        3.14159265358979323846264338327950288   /* pi */
#endif
#define RAD(deg)    M_PI*(deg/180.0)

#include <std.c>

/******************************************************************************
 * Modifications:
 *   Mark C. Miller, Tue Mar  9 17:16:17 PST 2010
 *   Added explicit logic to set split vfd as an example of how to do it.
 *   Added missing DBFreeFacelist() so valgrind reports no leaks.
 *
 *   Mark C. Miller, Wed Jul 14 15:43:10 PDT 2010
 *   Changed name of 'example' option to 'split'. Added code to Unregister
 *   option sets.
 * 
 *****************************************************************************/

int
main(int argc, char *argv[])
{
    DBfile         *dbfile = NULL;
    char const * const coordnames[3] = {"xcoords", "ycoords", "zcoords"};
    double         *coords[3];
    int             nodelist[8];
    double          x[8], y[8], z[8];
    int             shapesize[1];
    int             shapecnt[1];
    DBfacelist     *facelist = NULL;
    int             matnos[1], matlist[1], dims[1];
    int             i, j, len;
    char            mesh_command[256];
    float           rot1[4][4], rot2[4][4], final[4][4];
    float           angle;
    double          var[8];
    int		    driver = DB_PDB;
    int             setinf = 0;
    int             setnan = 0;
    char            *default_filename = "onehex.silo";
    char 	    *filename = default_filename;
    int             show_all_errors = FALSE;
    int             append = FALSE;

    int alloc_inc, vfd, core_vfd;
    char *mext, *rext;
    int meta_opts_id, raw_opts_id, split_opts_id, custom_opts_id;
    DBoptlist *core_opts = 0, *split_opts = 0, *custom_opts = 0;
#ifdef HAVE_HDF5_H
    hid_t fapl, fcpl;
#endif

    /* Parse command-line */
    for (i=1; i<argc; i++) {
	if (!strncmp(argv[i], "DB_", 3)) {
	    driver = StringToDriver(argv[i]);
	} else if (!strcmp(argv[i], "split")) {

            /* set up the meta file options for core vfd with 1K alloc */
            core_opts = DBMakeOptlist(10);

            /* indicate the vfd is core */
            core_vfd = DB_H5VFD_CORE;
            DBAddOption(core_opts, DBOPT_H5_VFD, &core_vfd);

            /* indicate the allocation increment is 1K */
            alloc_inc = 1<<10;
            DBAddOption(core_opts, DBOPT_H5_CORE_ALLOC_INC, &alloc_inc);

            /* register the core file options set with the library */
            meta_opts_id = DBRegisterFileOptionsSet(core_opts); 
           
            /* set up the raw file options */
            /* We're using pre-defined default file options for the raw part */
            raw_opts_id = DB_FILE_OPTS_H5_DEFAULT_SEC2;

            /* now, set up the split file options */
            split_opts = DBMakeOptlist(10);

            /* indicate the vfd is split */
            vfd = DB_H5VFD_SPLIT;
            DBAddOption(split_opts, DBOPT_H5_VFD, &vfd);

            /* indicate the meta file options set */
            DBAddOption(split_opts, DBOPT_H5_META_FILE_OPTS, &meta_opts_id);

            /* indicate the meta file extension */
            mext = ".meta.h5";
            DBAddOption(split_opts, DBOPT_H5_META_EXTENSION, mext);

            /* indicate the raw file options set */
            DBAddOption(split_opts, DBOPT_H5_RAW_FILE_OPTS, &raw_opts_id);

            /* indicate the raw file extension */
            /* Note, this is NOT an extension but an sprintf name pattern */
            rext = "%s.raw.dat";
            DBAddOption(split_opts, DBOPT_H5_RAW_EXTENSION, rext);

            /* register the split file options set with the library */
            split_opts_id = DBRegisterFileOptionsSet(split_opts); 

            /* set the 'driver' */
            driver = DB_HDF5_OPTS(split_opts_id);

            /* DO NOT FREE THE ASSOCIATED OPTLIST UNTIL AFTER OPEN/CREATE */

	} else if (!strcmp(argv[i], "append")) {
            append = TRUE;
	} else if (!strcmp(argv[i], "inf")) {
            setinf = 1;
	} else if (!strcmp(argv[i], "nan")) {
            setnan = 1;
#ifdef HAVE_HDF5_H
	} else if (!strcmp(argv[i], "custom")) {

            /* Test passing custom FAPL/FCPL to HDF5 driver by 
               setting some ridiculous values. Note that this
               client (onehex) generates a small enough file
               that '2' for sizes won't cause failures. */
            fapl = H5Pcreate(H5P_FILE_ACCESS);
            H5Pset_alignment(fapl, 0, 1024);
            fcpl = H5Pcreate(H5P_FILE_CREATE);
            H5Pset_sizes(fcpl, 4, 4);

            custom_opts = DBMakeOptlist(2);
            DBAddOption(custom_opts, DBOPT_H5_FAPL_HID_T, &fapl);
            DBAddOption(custom_opts, DBOPT_H5_FCPL_HID_T, &fcpl);

            custom_opts_id = DBRegisterFileOptionsSet(custom_opts); 

            driver = DB_HDF5_OPTS(custom_opts_id);
#endif
        } else if (!strncmp(argv[i], "fname=", 6)) {
            filename = strdup(&argv[i][6]);
        } else if (!strcmp(argv[i], "show-all-errors")) {
            show_all_errors = 1;
	} else if (argv[i][0] != '\0') {
	    fprintf(stderr, "%s: ignored argument `%s'\n", argv[0], argv[i]);
	}
    }

    DBShowErrors(show_all_errors?DB_ALL_AND_DRVR:DB_ABORT, NULL);
    printf("%s test file \"%s\".\n", append?"Opening":"Creating", filename);
    if (append)
    {
        dbfile = DBOpen(filename, driver, DB_APPEND);
        DBMkDir(dbfile, "dir1");
        DBSetDir(dbfile, "dir1");
    }
    else
        dbfile = DBCreate(filename, DB_CLOBBER, DB_LOCAL,
            "This is a test to see how well this string displays in Qt widget in Silex\n"
            "This is a test to see how well this string displays in Qt widget in Silex\n"
            "This is a test to see how well this string displays in Qt widget in Silex\n"
            "This is a test to see how well this string displays in Qt widget in Silex\n",
            driver);

    /* Ok, now we can safely free the file options sets optlists */
    if (core_opts)
    {
        DBUnregisterFileOptionsSet(meta_opts_id);
        DBFreeOptlist(core_opts);
    }
    if (split_opts)
    {
        DBUnregisterFileOptionsSet(split_opts_id);
        DBFreeOptlist(split_opts);
    }
    if (custom_opts)
    {
        DBUnregisterFileOptionsSet(custom_opts_id);
        DBFreeOptlist(custom_opts);
    }

    x[0] = 0; y[0] = 0; z[0] = 0;
    x[1] = 1; y[1] = 0; z[1] = 0;
    x[2] = 1; y[2] = 0; z[2] = 1;
    x[3] = 0; y[3] = 0; z[3] = 1;
    x[4] = 0; y[4] = 1; z[4] = 0;
    x[5] = 1; y[5] = 1; z[5] = 0;
    x[6] = 1; y[6] = 1; z[6] = 1;
    x[7] = 0; y[7] = 1; z[7] = 1;

    coords[0] = x;
    coords[1] = y;
    coords[2] = z;

    angle = 45;
    angle = M_PI*(angle/180.0);
    matrix_assign(rot1,
                  1, 0, 0, 0,
                  0, cos(angle), sin(angle), 0,
                  0, -sin(angle), cos(angle), 0,
                  0, 0, 0, 1);
    matrix_assign(rot2,
                  cos(angle), 0, -sin(angle), 0,
                  0, 1, 0, 0,
                  sin(angle), 0, cos(angle), 0,
                  0, 0, 0, 1);
    matrix_mult(rot1, rot2, final);

    for (i = 0; i < 8; i++)
    {
        float           tx, ty,tz;

        tx = x[i]*final[IND(1,1)] + y[i]*final[IND(1,2)] + z[i]*final[IND(1,3)] + final[IND(1,4)];
        ty = x[i]*final[IND(2,1)] + y[i]*final[IND(2,2)] + z[i]*final[IND(2,3)] + final[IND(2,4)];
        tz = x[i]*final[IND(3,1)] + y[i]*final[IND(3,2)] + z[i]*final[IND(3,3)] + final[IND(3,4)];

        x[i] = tx;
        y[i] = ty;
        z[i] = tz;

        var[i] = x[i]+y[i]*z[i];
    }

    DBPutUcdmesh(dbfile, "hex", 3, coordnames, coords, 8, 1, "zonelist",
                 "facelist", DB_DOUBLE, NULL);

    matnos[0] = 1;
    matlist[0] = 1;
    dims[0] = 1;

    DBPutMaterial(dbfile, "mat", "hex", 1, matnos, matlist, dims,
                  1, NULL, NULL, NULL, NULL, 0, DB_FLOAT, NULL);

    if (setinf)
    {
        double d = 1.0;
        var[7] = d/(d-1.0);
    }
    if (setnan)
    {
#ifndef _WIN32
        var[0] = nan("");
        if (var[0] == 0.0)
#endif
            var[0] = sqrt((double)-1.0);
    }
    DBPutUcdvar1(dbfile, "v", "hex", var, 8, NULL, 0, DB_DOUBLE, DB_NODECENT, NULL);

    nodelist[0] = 0;
    nodelist[1] = 1;
    nodelist[2] = 2;
    nodelist[3] = 3;
    nodelist[4] = 4;
    nodelist[5] = 5;
    nodelist[6] = 6;
    nodelist[7] = 7;

    shapesize[0] = 8;
    shapecnt[0] = 1;

    DBSetDeprecateWarnings(0);
    DBPutZonelist(dbfile, "zonelist", 1, 3, nodelist, 8, 0, shapesize,
                  shapecnt, 1);
    DBSetDeprecateWarnings(3);

    facelist = DBCalcExternalFacelist(nodelist, 8, 0, shapesize, shapecnt, 1,
                                      NULL, 0);

    DBPutFacelist(dbfile, "facelist", facelist->nfaces, facelist->ndims,
               facelist->nodelist, facelist->lnodelist, facelist->origin,
               facelist->zoneno, facelist->shapesize, facelist->shapecnt,
                  facelist->nshapes, facelist->types, facelist->typelist,
                  facelist->ntypes);

    DBFreeFacelist(facelist);
    sprintf(mesh_command, "mesh hex; contour v");
    len = strlen(mesh_command) + 1;
    DBWrite(dbfile, "_meshtvinfo", mesh_command, &len, 1, DB_CHAR);

done:

    DBClose(dbfile);

    CleanupDriverStuff();

    if (filename != default_filename)
        free(filename);

    return 0;
}
