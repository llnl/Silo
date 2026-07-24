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
#ifndef _WIN32
#include <unistd.h>
#else
#include <direct.h>
#include <stdlib.h>
#include <string.h>
#define access _access
#endif
#include <sys/stat.h>
#include <sys/types.h>

#include "silo.h"
#include <std.c>
extern int build_quad(DBfile *dbfile, char *name);
extern int build_ucd(DBfile *dbfile, char *name);
extern int build_ucd_tri(DBfile *dbfile, char *name, int flags);



/*-------------------------------------------------------------------------
 * Function:    main
 *
 * Purpose:     
 *
 * Return:      0
 *
 * Programmer:  
 *
 * Modifications:
 *      Robb Matzke, 1999-04-09
 *      Added argument parsing to control the driver which is used.
 *
 *-------------------------------------------------------------------------
 */
int main(int argc, char *argv[])
{
    
    int            meshtypes[3] = {DB_QUAD_RECT,
                                   DB_UCDMESH,
                                   DB_UCDMESH};
    char const    *meshnames[3] = {"/quad_dir/quadmesh",
                                   "/ucd_dir/ucdmesh",
                                   "/tri_dir/trimesh"};
    int            nmesh = 3;
    char           original_dir[1024];
    DBfile        *dbfile, *dbfile2, *dbfile3, *dbfile4, *dbfile5;
    char          *filename = "dir.pdb";
    char          *filename2 = "dir2.pdb";
    int            i, driver = DB_PDB, driver2 = DB_PDB;
    int            show_all_errors = FALSE;
    int            ndirs = 0;
    int            ntocs = 0;
    int            compat = 0;

    for (i=1; i<argc; i++) {
        if (!strncmp(argv[i], "DB_PDB",6)) {
            driver = StringToDriver(argv[i]);
            driver2 = driver;
            filename = "dir.pdb";
            filename2 = "dir2.pdb";
        } else if (!strncmp(argv[i], "DB_HDF5", 7)) {
            driver = StringToDriver(argv[i]);
            driver2 = driver;
            filename = "dir.h5";
            filename2 = "dir2.h5";
        } else if (!strcmp(argv[i], "mix-driver-copy")) {
            driver2 = driver == DB_HDF5 ? DB_PDB : DB_HDF5;
            filename2 = driver == DB_HDF5 ? "dir2.pdb" : "dir2.h5";
        } else if (!strcmp(argv[i], "show-all-errors")) {
            show_all_errors = 1;
        } else if (!strncmp(argv[i], "ndirs=", 6)) {
            ndirs = (int) strtol(argv[i]+6,0,10);
        } else if (!strncmp(argv[i], "ntocs=", 6)) {
            ntocs = (int) strtol(argv[i]+6,0,10);
        } else if (!strcmp(argv[i], "compat-over-perf")) {
            compat = DB_COMPAT_OVER_PERF;
        } else if (!strcmp(argv[i], "perf-over-compat")) {
            compat = DB_PERF_OVER_COMPAT;
	} else {
            fprintf(stderr, "%s: ignored argument `%s'\n", argv[0], argv[i]);
        }
    }
    
    DBSetFriendlyHDF5Names(1);
    DBSetCompatibilityMode(compat);
    DBShowErrors(show_all_errors?DB_ALL_AND_DRVR:DB_ALL, NULL);

    dbfile = DBCreate(filename, 0, DB_LOCAL, "dir test file", driver);
    printf("Creating file: '%s'...\n", filename);

    for (i = 0; i < ndirs; i++)
    {
        char dirname[64];
        sprintf(dirname, "domain_%08d", i);
        DBMkDir(dbfile, dirname);
    }

    DBMkdir(dbfile, "quad_dir");
    DBMkdir(dbfile, "ucd_dir");
    DBMkdir(dbfile, "tri_dir");

    /* Test MkDirP */ 
    DBMkDirP(dbfile, "gorfo/foo/bar/bin/baz");
    DBSetDir(dbfile, "/gorfo/foo/bar");
    DBMkDirP(dbfile, "../baz/bin");
    DBMkDirP(dbfile, "../bah/bin");
    DBSetDir(dbfile, "/");
    DBMkDirP(dbfile, "/");
    DBMkDirP(dbfile, "gorfo/foo/bar");

    DBGetDir(dbfile, original_dir);

    DBSetDir(dbfile, "/quad_dir");
    build_quad(dbfile, "quadmesh");
    DBMkdir(dbfile, "quad_subdir1");
    DBSetDir(dbfile, "quad_subdir1");
    build_quad(dbfile, "quadmesh");
    DBMkdir(dbfile, "quad_subdir2");
    DBSetDir(dbfile, "quad_subdir2");
    DBMkSymlink(dbfile, "/quad_dir/quadmesh", "qmlink");
    DBMkSymlink(dbfile, "../../quadmesh", "qm2link");
    build_quad(dbfile, "quadmesh");
    DBSetDir(dbfile, "../..");
    DBMkdir(dbfile, "quad_subdir3");
    DBSetDir(dbfile, "quad_subdir3");
    DBMkSymlink(dbfile, "/quad_dir/quad_subdir1", "dirlink");
    DBMkSymlink(dbfile, "dir2.h5:/gorfo", "extlink");
    db_errno = 0;

    build_quad(dbfile, "quadmesh");

    DBSetDir(dbfile, "/ucd_dir");
    build_ucd(dbfile, "ucdmesh");

    DBSetDir(dbfile, "/tri_dir");
    build_ucd_tri(dbfile, "trimesh", 0x0); /* make a smaller trimesh here */

    DBSetDir(dbfile, original_dir);
    DBPutMultimesh(dbfile, "mmesh", nmesh, (DBCAS_t) meshnames, meshtypes, NULL);

    DBClose(dbfile);

    dbfile = DBOpen(filename, driver, DB_READ);
    dbfile2 = DBCreate(filename2, 0, DB_LOCAL, "dir test file", driver2);

    DBSetDir(dbfile, "/tri_dir");
    DBMkDirP(dbfile2, "gorfo/foo/bar");
    DBSetDir(dbfile2, "gorfo/foo");

    /* Just copy a mesh and its sub-objects */
    DBCp(0, dbfile, dbfile2, "trimesh", "trimesh_copy", DB_EOA);

    DBSetDir(dbfile2, "/");

    /* Try a recursive copy */
    DBCp("-r", dbfile, dbfile2, "/quad_dir/quad_subdir1", "quad_dir_copy", DB_EOA);

    /* Test a complicated whole file copy ACROSS drivers */
    if (driver2 != DB_PDB && access("multi_ucd3d_copy.pdb", 0) == 0)
    {
        int result;
        DBfile *fsrc = DBOpen("multi_ucd3d_copy.pdb", DB_PDB, DB_READ);
        DBfile *fdst = DBCreate("foo.silo", 0, DB_LOCAL, "cross-driver whole file copy test", DB_HDF5);
        result = DBCp("-r", fsrc, fdst, "/", "/", DB_EOA);
        DBClose(fsrc);
        DBClose(fdst);
        if (result != 0)
        {
            fprintf(stderr, "Complex whole file, cross driver copy from PDB to HDF5 failed\n");
            fprintf(stderr, "Errno = %d, \"%s\"\n", DBErrno(), DBErrString());
            return 1;
        }
    }
    else if (driver2 == DB_PDB && access("multi_ucd3d_copy.h5", 0) == 0)
    {
        int result;
        DBfile *fsrc = DBOpen("multi_ucd3d_copy.h5", DB_HDF5, DB_READ);
        DBfile *fdst = DBCreate("bar.silo", 0, DB_LOCAL, "cross-driver whole file copy test", DB_PDB);
        result = DBCp("-r", fsrc, fdst, "/", "/", DB_EOA);
        DBClose(fsrc);
        DBClose(fdst);
        if (result != 0)
        {
            fprintf(stderr, "Complex whole file, cross driver copy from HDF5 to PDB failed\n");
            fprintf(stderr, "Errno = %d, \"%s\"\n", DBErrno(), DBErrString());
            return 1;
        }
        fsrc = DBOpen("multi_ucd3d_copy.h5", DB_HDF5, DB_APPEND);
        result = DBCp("-rs", fsrc, fsrc, "/block5", "block5_copy", DB_EOA);
        DBClose(fsrc);
        if (result != 0)
        {
            fprintf(stderr, "Copying via symlinks failed\n");
            fprintf(stderr, "Errno = %d, \"%s\"\n", DBErrno(), DBErrString());
            return 1;
        }
    }
    else
    {
        fprintf(stderr, "\n\nComplex whole file, cross driver copy requires \"multi_ucd3d_copy.[h5,pdb]\"\n\n\n");
        return 1;
    }

    build_ucd_tri(dbfile2, "trimesh", 0x2); /* make a larger tri mesh here */

    /* try to copy the smaller trimesh on top of the larger one */
    DBCp(0, dbfile, dbfile2, "trimesh", "trimesh", DB_EOA);

    /* Test DBLs */
    {
        int nlist  = ndirs + 5;
        char **list = malloc((nlist) * sizeof(char*));
        DBSetDir(dbfile, "/");
        DBLs(dbfile, 0, 0, &nlist);
        DBLs(dbfile, 0, list, &nlist);
        for (i = 0; i < nlist; i++)
        {
            printf("\"%s\"\n", list[i]);
            free(list[i]);
        }

        nlist = ndirs + 5;
        DBSetDir(dbfile, "/tri_dir");
        DBLs(dbfile, 0, list, &nlist);
        for (i = 0; i < nlist; i++)
        {
            printf("\"%s\"\n", list[i]);
            free(list[i]);
        }
        free(list);
    }

    /* make some hellaciously long directory names which are almost the same */
    if (ntocs)
    {
        char tmp[] = "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        char tmp2[sizeof(tmp)+10];
        DBClose(dbfile);
        dbfile = DBOpen(filename, driver, DB_APPEND);
        DBSetDir(dbfile, "/");
        DBMkDir(dbfile, "testtoc");
        DBSetDir(dbfile, "/testtoc");
        for (i = 0; i < ndirs; i++)
        {
            snprintf(tmp2, sizeof(tmp2), "%s%08d", tmp, i);
            DBMkDir(dbfile, tmp2);
        }
        for (i = 0; i < ntocs/2; i++)
        {
            DBSetDir(dbfile, "/");
            DBGetToc(dbfile);
            DBSetDir(dbfile, "/testtoc");
            DBGetToc(dbfile);
        }
    }

    DBClose(dbfile);
    DBClose(dbfile2);

    dbfile = DBOpen(filename, driver, DB_READ);
    dbfile2 = DBOpen(filename2, driver2, DB_APPEND);

    if ((driver&0xF) == DB_HDF5)
        DBCpDir(dbfile, "ucd_dir", dbfile2, "gorfo/foobar");

    /* this should be ok becase we're opening for read */
    dbfile3 = DBOpen(filename, driver, DB_READ);
    DBClose(dbfile3);

    /* this should fail because file is open for write */
    dbfile4 = DBOpen(filename2, driver, DB_READ);
    if (dbfile4 != 0 || db_errno != E_CONCURRENT)
        exit(EXIT_FAILURE);

    /* this should fail because filename is already open */
    dbfile5 = DBOpen(filename, driver, DB_APPEND);
    if (dbfile5 != 0 || db_errno != E_CONCURRENT)
        exit(EXIT_FAILURE);

    DBClose(dbfile);
    DBClose(dbfile2);

    /* test attempt to DBCreate a file without clobbering it and
       for which the path is really a dir in the host filesystem */
    unlink("dir-test-foo");
#ifndef _WIN32
    mkdir("dir-test-foo", 0777);
#else
    mkdir("dir-test-foo");
#endif
    dbfile2 = DBCreate("dir-test-foo", DB_NOCLOBBER, DB_LOCAL, "dir test file", driver2);
    unlink("dir-test-foo");
    if (dbfile2 != 0)
        exit(EXIT_FAILURE);
#ifndef _WIN32
    mkdir("dir-test-foo", 0777);
#else
    mkdir("dir-test-foo");
#endif
    dbfile2 = DBCreate("dir-test-foo", DB_CLOBBER, DB_LOCAL, "dir test file", driver2);
    unlink("dir-test-foo");
    if (dbfile2 != 0)
        exit(1);

    CleanupDriverStuff();
    return 0;
}
