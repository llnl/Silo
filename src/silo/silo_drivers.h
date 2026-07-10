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
Contract No.  DE-AC52-07NA27344 with the DOE.

Neither the  United States Government nor  Lawrence Livermore National
Security, LLC nor any of  their employees, makes any warranty, express
or  implied,  or  assumes  any  liability or  responsibility  for  the
accuracy, completeness,  or usefulness of  any information, apparatus,
product, or  process disclosed, or  represents that its use  would not
infringe privately-owned rights.

Any reference herein to  any specific commercial products, process, or
services by trade name,  trademark, manufacturer or otherwise does not
necessarily  constitute or imply  its endorsement,  recommendation, or
favoring  by  the  United  States  Government  or  Lawrence  Livermore
National Security,  LLC. The views  and opinions of  authors expressed
herein do not necessarily state  or reflect those of the United States
Government or Lawrence Livermore National Security, LLC, and shall not
be used for advertising or product endorsement purposes.
*/
/*
 * Programmer:  Robb Matzke <robb@arborea.spizella.com>
 *              Tuesday, February  9, 1999
 */
#ifndef SILO_DRIVERS_H
#define SILO_DRIVERS_H


/*
 * SILO drivers header file.
 *
 * This file contains the definitions for SILO drivers and should be included
 * by every SILO source file that defines DB_MAIN.
 *
 * Normally, programs cannot tell if a particular driver is defined by just
 * including silo.h.  To determine if a driver is defined, include this file.
 */
#include "config.h" /* Included for driver tests */
#include "silo.h"

/*
 * Get rid of the driver definitions from silo.h so that we can define the
 * actual drivers.
 */
#undef DB_NETCDF
#undef DB_PDB
#undef DB_PDBP
#undef DB_TAURUS
#undef DB_UNKNOWN
#undef DB_DEBUG
#undef DB_HDF5X

/* Now set up the real driver definitions. */
#ifdef HAVE_NETCDF_DRIVER
#  define DB_NETCDF
#endif
#ifdef HAVE_PDB_DRIVER
#  define DB_PDB
#endif
#ifdef HAVE_PDBP_DRIVER
#  define DB_PDBP
#endif
#ifdef HAVE_TAURUS_DRIVER
#  define DB_TAURUS
#endif
#define DB_UNKNOWN
#define DB_DEBUG
#ifdef HAVE_HDF5_DRIVER
#  define DB_HDF5X
#endif

/*-------------------------------------------------------------------------
 * Conditionally include definitions for the file formats that the application
 * is about to use.  As we include the format header files, we should make
 * sure that the file format constant is properly defined.  Each file format
 * should be given a unique small integer that will be used as an index into
 * an array.  If a new file format is added, be sure to increment the value
 * of DB_NFORMATS.
 *
 * Don't use slot `1' of these arrays as that reduces the error checking if
 * the user doesn't spell the format name correctly.
 *
 * Anyone can change this info as long as the DB_NFORMATS constant doesn't
 * change.  If anything else changes, the silo library must be recompiled.
 *
 * Opening a database file of type DB_UNKNOWN will try to open the database
 * file with each driver that is defined beginning with the lowest numbered
 * driver.  Therefore, the most specific drivers should be listed first.
 *-------------------------------------------------------------------------
 */
#ifdef DB_NETCDF
#undef  DB_NETCDF
#define DB_NETCDF            0
#define DB_NETCDF_OPEN     db_cdf_Open
#define DB_NETCDF_CREATE   NULL
#define DB_NETCDF_FSINGLE  db_cdf_ForceSingle
#define DB_NETCDF_VERSION  NULL

extern DBfile *db_cdf_Open(char const *, int, int);
extern int db_cdf_ForceSingle(int);
extern int db_cdf_Version(int *, int *, int *);

#else
#define DB_NETCDF_OPEN     NULL
#define DB_NETCDF_CREATE   NULL
#define DB_NETCDF_FSINGLE  NULL
#define DB_NETCDF_VERSION  NULL
#endif

/* Slot 1 is for PDB Proper */
#ifdef DB_PDBP
#undef  DB_PDBP
#define DB_PDBP       1
#define DB_PDBP_OPEN        db_pdbp_Open
#define DB_PDBP_CREATE      db_pdbp_Create
#define DB_PDBP_FSINGLE     db_pdbp_ForceSingle
#define DB_PDBP_VERSION     db_pdbp_Version

extern DBfile *db_pdbp_Open(char const *, int, int);
extern DBfile *db_pdbp_Create(char const *, int, int, int, char const *);
extern int db_pdbp_ForceSingle(int);
extern int db_pdbp_Version(int *, int *, int *);

#else
#define DB_PDBP_OPEN        NULL
#define DB_PDBP_CREATE      NULL
#define DB_PDBP_FSINGLE     NULL
#define DB_PDBP_VERSION     NULL
#endif

/* Slot 2 is PDB Lite */
#ifdef DB_PDB
#undef  DB_PDB
#define DB_PDB       2
#define DB_PDB_OPEN        db_pdb_Open
#define DB_PDB_CREATE      db_pdb_Create
#define DB_PDB_FSINGLE     db_pdb_ForceSingle
#define DB_PDB_VERSION     db_pdb_Version

extern DBfile *db_pdb_Open(char const *, int, int);
extern DBfile *db_pdb_Create(char const *, int, int, int, char const *);
extern int db_pdb_ForceSingle(int);
extern int db_pdb_Version(int *, int *, int *);

#else
#define DB_PDB_OPEN        NULL
#define DB_PDB_CREATE      NULL
#define DB_PDB_FSINGLE     NULL
#define DB_PDB_VERSION     NULL
#endif

#ifdef DB_TAURUS
#undef  DB_TAURUS
#define DB_TAURUS            3
#define DB_TAURUS_OPEN     db_taur_Open
#define DB_TAURUS_CREATE   NULL
#define DB_TAURUS_FSINGLE  NULL
#define DB_TAURUS_VERSION  NULL

extern DBfile *db_taur_Open(char const *, int, int);

#else
#define DB_TAURUS_OPEN     NULL
#define DB_TAURUS_CREATE   NULL
#define DB_TAURUS_FSINGLE  NULL
#define DB_TAURUS_VERSION  NULL
#endif

#ifdef DB_UNKNOWN               /*For opening files of unknown type */
#undef  DB_UNKNOWN
#define DB_UNKNOWN           5
#define DB_UNKNOWN_OPEN    db_unk_Open
#define DB_UNKNOWN_CREATE  NULL
#define DB_UNKNOWN_FSINGLE NULL
#define DB_UNKNOWN_VERSION NULL

extern DBfile *db_unk_Open(char const *, int, int);

#else
#define DB_UNKNOWN_OPEN    NULL
#define DB_UNKNOWN_CREATE  NULL
#define DB_UNKNOWN_FSINGLE NULL
#define DB_UNKNOWN_VERSION NULL
#endif

#ifdef DB_DEBUG                 /*A Demo */
#undef  DB_DEBUG
#define DB_DEBUG           6
#define DB_DEBUG_OPEN      db_debug_open
#define DB_DEBUG_CREATE    db_debug_create
#define DB_DEBUG_FSINGLE   NULL
#define DB_DEBUG_VERSION   NULL

extern DBfile *db_debug_open(char const *, int, int);
extern DBfile *db_debug_create(char const *, int, int, int, char const *);

#else
#define DB_DEBUG_OPEN      NULL
#define DB_DEBUG_CREATE    NULL
#define DB_DEBUG_FSINGLE   NULL
#define DB_DEBUG_VERSION   NULL
#endif

#ifdef DB_HDF5X
#undef DB_HDF5X
#define DB_HDF5X            7
#define DB_HDF5_OPEN       db_hdf5_Open
#define DB_HDF5_CREATE     db_hdf5_Create
#define DB_HDF5_FSINGLE    db_hdf5_ForceSingle
#define DB_HDF5_VERSION    db_hdf5_Version

extern DBfile *db_hdf5_Open(char const *, int, int);
extern DBfile *db_hdf5_Create(char const *, int, int, int, char const *);
extern int db_hdf5_ForceSingle(int);
extern int db_hdf5_Version(int *, int *, int *);

#else
#define DB_HDF5_OPEN       NULL
#define DB_HDF5_CREATE     NULL
#define DB_HDF5_FSINGLE    NULL
#define DB_HDF5_VERSION    NULL
#endif

/*
 * If DB_NFORMATS is changed, the silo library must be recompiled!
 */
#define DB_NFORMATS     10          /*Total number of file formats */

#ifdef DB_MAIN
/*-------------------------------------------------------------------------
 * The DBOpen and DBCreate functions/macros reference global variables
 * DBOpenCB and DBCreateCB which are arrays of pointers to open and
 * create functions for the various file types.
 *
 * The items in this array must be listed in order of file format type,
 * since the file format type will be used to index the array.
 *-------------------------------------------------------------------------
 */
#define DBOPENCB        {DB_NETCDF_OPEN,        \
                         DB_PDBP_OPEN,          \
                         DB_PDB_OPEN,           \
                         DB_TAURUS_OPEN,        \
                         NULL,                  /*unused*/\
                         DB_UNKNOWN_OPEN,       \
                         DB_DEBUG_OPEN,         \
                         DB_HDF5_OPEN,          \
                         NULL,                  /*unused*/\
                         NULL}                  /*unused*/

#define DBCREATECB      {DB_NETCDF_CREATE,      \
                         DB_PDBP_CREATE,        \
                         DB_PDB_CREATE,         \
                         DB_TAURUS_CREATE,      \
                         NULL,                  /*unused*/\
                         DB_UNKNOWN_CREATE,     \
                         DB_DEBUG_CREATE,       \
                         DB_HDF5_CREATE,        \
                         NULL,                  /*unused*/\
                         NULL}                  /*unused*/

#define DBFSINGLECB     {DB_NETCDF_FSINGLE,     \
                         DB_PDBP_FSINGLE,       \
                         DB_PDB_FSINGLE,        \
                         DB_TAURUS_FSINGLE,     \
                         NULL,                  /*unused*/\
                         DB_UNKNOWN_FSINGLE,    \
                         DB_DEBUG_FSINGLE,      \
                         DB_HDF5_FSINGLE,       \
                         NULL,                  /*unused*/\
                         NULL}                  /*unused*/

#define DBVERSIONCB     {DB_NETCDF_VERSION,     \
                         DB_PDBP_VERSION,       \
                         DB_PDB_VERSION,        \
                         DB_TAURUS_VERSION,     \
                         NULL,                  /*unused*/\
                         DB_UNKNOWN_VERSION,    \
                         DB_DEBUG_VERSION,      \
                         DB_HDF5_VERSION,       \
                         NULL,                  /*unused*/\
                         NULL}                  /*unused*/

DBfile *(*DBOpenCB[DB_NFORMATS]) (char const *, int, int) = DBOPENCB;
DBfile *(*DBCreateCB[DB_NFORMATS]) (char const *, int, int, int, char const *) = DBCREATECB;
int     (*DBFSingleCB[DB_NFORMATS]) (int) = DBFSINGLECB;
int     (*DBVersionCB[DB_NFORMATS]) (int*,int*,int*) = DBVERSIONCB;
#endif /* DB_MAIN */

#endif /* !SILO_DRIVERS_H */
