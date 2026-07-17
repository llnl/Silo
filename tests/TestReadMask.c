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
#include <stdio.h>
#include <silo.h>
#include <string.h>
#ifndef _WIN32
#include <sys/time.h>
#endif
#include <sys/timeb.h>
#include <time.h>
#include <std.c>

/* To compile this program on hyper, here is the command:
 *
 *   cc -o ts -I/usr/local/silo/4.2/irix64/n32/include 
 *      -L/usr/local/silo/4.2/irix64/n32/lib -DNEW_SILO TestSilo.c -lsilo -lm
 * 
 * Programmer: Brad Whitlock
 * Date:       Thu May 17 13:00:50 PST 2001
 *
 */
void    printMask(FILE * fp, unsigned long long mask);
int     test_readmat(DBfile * dbfile, const char *testName, unsigned long long mask);
int     test_readpointmesh(DBfile * dbfile, unsigned long long mask);
int     test_readpointvar(DBfile * dbfile, unsigned long long mask);
int     test_readquadmesh(DBfile * dbfile, unsigned long long mask);
int     test_readquadvar(DBfile * dbfile, unsigned long long mask);
int     test_readucdmesh(DBfile * dbfile, const char *testName, unsigned long long mask);
int     test_readucdvar(DBfile * dbfile, unsigned long long mask);
int     test_readfacelist(DBfile * dbfile, unsigned long long mask);
int     test_readzonelist(DBfile * dbfile, unsigned long long mask);

void    ResetTime(void);
int     ElapsedTime(void);
void    printMaterial(const char *routine, DBmaterial * mat);
void    printTimes(int *ms);

/* Variables to hold time data. */
#ifndef _WIN3W
struct timeval start_time;
struct timeval end_time;
#endif

/*
 * 
 * Mark C. Miller, Mon Jan 11 16:24:33 PST 2010
 * Added missing test reading of zonelist for last mask setting.
 */
int
main(int argc, char *argv[])
{
    int i, driver = DB_PDB;
    DBfile *dbfile = NULL;
    int     ms[33];
    char   *pdbfiles[] = { "./rect2d.pdb",
        "./point2d.pdb",
        "./globe.pdb"
    };
    char   *h5files[] = { "./rect2d.h5",
        "./point2d.h5",
        "./globe.h5"
    };
    char **files = pdbfiles;
    int maskindex = 0;
    int show_all_errors = FALSE;

    /* Set the masks used for the tests. */
    unsigned long long mask[] = {
        DBAll,
        DBMatMatnos,
        DBMatMatlist,
        DBMatMixList,
        DBNone,
        /* ----- */
        DBAll,
        DBQMCoords,
        DBNone,
        /* ----- */
        DBAll,
        DBQVData,
        DBNone,
        /* ----- */
        DBAll,
        DBPMCoords,
        DBNone,
        /* ----- */
        DBAll,
        DBPVData,
        DBNone,
        /* ----- */
        DBAll,
        DBUMCoords,
        DBUMFacelist,
        DBUMZonelist,
        DBNone,
        /* ----- */
        DBAll,
        DBUVData,
        DBNone,
        /* ----- */
        DBAll,
        DBFacelistInfo,
        DBNone,
        /* ----- */
        DBAll,
        DBZonelistInfo,
        DBNone,
    };

    for (i=1; i<argc; i++) {
        if (!strncmp(argv[i], "DB_PDB", 6)) {
            driver = StringToDriver(argv[i]);
            files = pdbfiles;
        } else if (!strncmp(argv[i], "DB_HDF5", 7)) {
            driver = StringToDriver(argv[i]);
            files = h5files;
        } else if (!strcmp(argv[i], "show-all-errors")) {
            show_all_errors = 1;
	} else if (argv[i][0] != '\0') {
            fprintf(stderr, "%s: ignored argument `%s'\n", argv[0], argv[i]);
        }
    }

    if (show_all_errors) DBShowErrors(DB_ALL_AND_DRVR, 0);

    printf("NOTE: The times listed here do not take any caching into\n");
    printf("account.  Thus, the first time listed in each section may be\n");
    printf("extraordinarily large.  For an accurate timing test, this cache\n");
    printf("would have to be disabled.\n\n");

    printMask(stdout, DBGetDataReadMask2());
    printf("\n\n");

    /**************************************************************************
     *
     * Test the material read flags.
     *
     *************************************************************************/
    dbfile = DBOpen(files[0], DB_UNKNOWN, DB_READ);
    if (dbfile == NULL)
    {
        fprintf(stderr, "The file %s could not be opened!\n", files[0]);
        return 1;
    }

    /* Try reading material. */
    ms[maskindex] = test_readmat(dbfile, "test_readmatnos", mask[maskindex]);
    printf("Reading material for %s took %d ms.\n\n", files[0],
           ms[maskindex]);
    maskindex++;
    ms[maskindex] = test_readmat(dbfile, "test_readmatnos", mask[maskindex]);
    printf("Reading material numbers for %s took %d ms.\n\n", files[0],
           ms[maskindex]);
    maskindex++;
    ms[maskindex] = test_readmat(dbfile, "test_readmatlist", mask[maskindex]);
    printf("Reading material list for %s took %d ms.\n\n", files[0], ms[maskindex]);
    maskindex++;
    ms[maskindex] = test_readmat(dbfile, "test_readmatmixlist", mask[maskindex]);
    printf("Reading material mix list for %s took %d ms.\n\n", files[0],
           ms[maskindex]);
    maskindex++;
    ms[maskindex] = test_readmat(dbfile, "test_readmatlist", mask[maskindex]);
    printf("Reading material list for %s took %d ms.\n\n", files[0], ms[maskindex]);
    maskindex++;
    printf("-----------------------------\n");


    /**************************************************************************
     *
     * Test the quad mesh read flags.
     *
     *************************************************************************/
    /* Try reading the quad mesh. */
    ms[maskindex] = test_readquadmesh(dbfile, mask[maskindex]);
    printf("Reading quad mesh for %s took %d ms.\n\n", files[0], ms[maskindex]);
    maskindex++;
    ms[maskindex] = test_readquadmesh(dbfile, mask[maskindex]);
    printf("Reading quad mesh for %s took %d ms.\n\n", files[0], ms[maskindex]);
    maskindex++;
    ms[maskindex] = test_readquadmesh(dbfile, mask[maskindex]);
    printf("Reading quad mesh for %s took %d ms.\n\n", files[0], ms[maskindex]);
    maskindex++;
    printf("-----------------------------\n");

    /* Try reading a quad var. */
    ms[maskindex] = test_readquadvar(dbfile, mask[maskindex]);
    printf("Reading quad var for %s took %d ms.\n\n", files[0], ms[maskindex]);
    maskindex++;
    ms[maskindex] = test_readquadvar(dbfile, mask[maskindex]);
    printf("Reading quad var for %s took %d ms.\n\n", files[0], ms[maskindex]);
    maskindex++;
    ms[maskindex] = test_readquadvar(dbfile, mask[maskindex]);
    printf("Reading quad var for %s took %d ms.\n\n", files[0], ms[maskindex]);
    maskindex++;
    printf("-----------------------------\n");

    DBClose(dbfile);

    /**************************************************************************
     *
     * Test the point mesh read flags.
     *
     *************************************************************************/
    dbfile = DBOpen(files[1], DB_UNKNOWN, DB_READ);
    if (dbfile == NULL)
    {
        fprintf(stderr, "The file %s could not be opened!\n", files[1]);
        return 1;
    }

    /* Try reading the point mesh. */
    ms[maskindex] = test_readpointmesh(dbfile, mask[maskindex]);
    printf("Reading point mesh for %s took %d ms.\n\n", files[1], ms[maskindex]);
    maskindex++;
    ms[maskindex] = test_readpointmesh(dbfile, mask[maskindex]);
    printf("Reading point mesh for %s took %d ms.\n\n", files[1], ms[maskindex]);
    maskindex++;
    ms[maskindex] = test_readpointmesh(dbfile, mask[maskindex]);
    printf("Reading point mesh for %s took %d ms.\n\n", files[1], ms[maskindex]);
    maskindex++;
    printf("-----------------------------\n");

    /* Try reading a point var. */
    ms[maskindex] = test_readpointvar(dbfile, mask[maskindex]);
    printf("Reading point var for %s took %d ms.\n\n", files[1], ms[maskindex]);
    maskindex++;
    ms[maskindex] = test_readpointvar(dbfile, mask[maskindex]);
    printf("Reading point var for %s took %d ms.\n\n", files[1], ms[maskindex]);
    maskindex++;
    ms[maskindex] = test_readpointvar(dbfile, mask[maskindex]);
    printf("Reading point var for %s took %d ms.\n\n", files[1], ms[maskindex]);
    maskindex++;
    printf("-----------------------------\n");
    DBClose(dbfile);

    /**************************************************************************
     *
     * Test the ucd mesh read flags.
     *
     *************************************************************************/
    dbfile = DBOpen(files[2], DB_UNKNOWN, DB_READ);
    if (dbfile == NULL)
    {
        fprintf(stderr, "The file %s could not be opened!\n", files[2]);
        return 1;
    }

    /* Try reading the ucd mesh. */
    ms[maskindex] = test_readucdmesh(dbfile, "test_readucdmeshnone", mask[maskindex]);
    printf("Reading ucd mesh for %s took %d ms.\n\n", files[2], ms[maskindex]);
    maskindex++;
    ms[maskindex] = test_readucdmesh(dbfile, "test_readucdmeshcoords", mask[maskindex]);
    printf("Reading ucd mesh for %s took %d ms.\n\n", files[2], ms[maskindex]);
    maskindex++;
    ms[maskindex] = test_readucdmesh(dbfile, "test_readucdmeshfaces", mask[maskindex]);
    printf("Reading ucd mesh for %s took %d ms.\n\n", files[2], ms[maskindex]);
    maskindex++;
    ms[maskindex] = test_readucdmesh(dbfile, "test_readucdmeshzones", mask[maskindex]);
    printf("Reading ucd mesh for %s took %d ms.\n\n", files[2], ms[maskindex]);
    maskindex++;
    ms[maskindex] = test_readucdmesh(dbfile, "test_readucdmeshzones", mask[maskindex]);
    printf("Reading ucd mesh for %s took %d ms.\n\n", files[2], ms[maskindex]);
    maskindex++;
    printf("-----------------------------\n");

    /* Try reading a ucd var. */
    ms[maskindex] = test_readucdvar(dbfile, mask[maskindex]);
    printf("Reading ucd var for %s took %d ms.\n\n", files[2], ms[maskindex]);
    maskindex++;
    ms[maskindex] = test_readucdvar(dbfile, mask[maskindex]);
    printf("Reading ucd var for %s took %d ms.\n\n", files[2], ms[maskindex]);
    maskindex++;
    ms[maskindex] = test_readucdvar(dbfile, mask[maskindex]);
    printf("Reading ucd var for %s took %d ms.\n\n", files[2], ms[maskindex]);
    maskindex++;
    printf("-----------------------------\n");

    /* Try reading a facelist. */
    ms[maskindex] = test_readfacelist(dbfile, mask[maskindex]);
    printf("Reading face list for %s took %d ms.\n\n", files[2], ms[maskindex]);
    maskindex++;
    ms[maskindex] = test_readfacelist(dbfile, mask[maskindex]);
    printf("Reading face list for %s took %d ms.\n\n", files[2], ms[maskindex]);
    maskindex++;
    ms[maskindex] = test_readfacelist(dbfile, mask[maskindex]);
    printf("Reading face list for %s took %d ms.\n\n", files[2], ms[maskindex]);
    maskindex++;
    printf("-----------------------------\n");

    /* Try reading a zonelist. */
    ms[maskindex] = test_readzonelist(dbfile, mask[maskindex]);
    printf("Reading zone list for %s took %d ms.\n\n", files[2], ms[maskindex]);
    maskindex++;
    ms[maskindex] = test_readzonelist(dbfile, mask[maskindex]);
    printf("Reading zone list for %s took %d ms.\n\n", files[2], ms[maskindex]);
    maskindex++;
    ms[maskindex] = test_readzonelist(dbfile, mask[maskindex]);
    printf("Reading zone list for %s took %d ms.\n\n", files[2], ms[maskindex]);
    maskindex++;
    DBClose(dbfile);

    /* Print the times. */
    printTimes(ms);

    CleanupDriverStuff();
    return 0;
}

int
test_readmat(DBfile * dbfile, const char *testName, unsigned long long mask)
{
    int     ms;
    DBmaterial *mat = NULL;

    /* Reset the timer. */
    ResetTime();

    DBSetDataReadMask2(mask);
    printf("%s: ", testName);
    printMask(stdout, DBGetDataReadMask2());
    printf("\n");
    /* Read the material list and print the material struct. */
    mat = DBGetMaterial(dbfile, "mat1");
    ms = ElapsedTime();
    printMaterial(testName, mat);
    DBFreeMaterial(mat);

    /* Return how many milliseconds since the call to ResetTime. */
    return ms;
}

void
printMaterial(const char *routine, DBmaterial * mat)
{
    if (mat != NULL)
    {
        int     i;

        printf("%s: mat {\n", routine);
        printf("    nmat=%d\n", mat->nmat);
        if (mat->matnos != NULL)
        {
            printf("    matnos = 0x%p = {", mat->matnos);
            for (i = 0; i < mat->nmat; ++i)
                printf("%d, ", mat->matnos[i]);
            printf("}\n");
        } else
        {
            printf("    matnos = 0x%p\n", mat->matnos);
        }
        printf("    matlist = 0x%p\n", mat->matlist);
        printf("    mix_vf = 0x%p\n", mat->mix_vf);
        printf("    mix_next = 0x%p\n", mat->mix_next);
        printf("    mix_mat = 0x%p\n", mat->mix_mat);
        printf("    mix_zone = 0x%p\n", mat->mix_zone);
        printf("};\n");
    } else
    {
        fprintf(stderr, "%s: mat = NULL!\n", routine);
    }
}

int
test_readpointmesh(DBfile * dbfile, unsigned long long mask)
{
    int     ms;
    DBpointmesh *pmesh = NULL;

    /* Reset the timer. */
    ResetTime();

    DBSetDataReadMask2(mask);
    printf("test_readpointmesh: ");
    printMask(stdout, DBGetDataReadMask2());
    printf("\n");
    /* Read the material list and print the material struct. */
    pmesh = DBGetPointmesh(dbfile, "dir1/pmesh");
    ms = ElapsedTime();
    if (pmesh != NULL)
    {
        printf("test_readpointmesh: pmesh = {\n");
        printf("  ...\n");
        printf("    name = %s\n", pmesh->name);
        printf("  ...\n");
        printf("    coords[0] = 0x%p\n", pmesh->coords[0]);
        printf("    coords[1] = 0x%p\n", pmesh->coords[1]);
        printf("    coords[2] = 0x%p\n", pmesh->coords[2]);
        printf("  ...\n");
        printf("    nels = %d\n", pmesh->nels);
        printf("  ...\n");
        printf("};\n");
    } else
    {
        fprintf(stderr, "test_readpointmesh: pmesh = NULL!\n");
    }
    DBFreePointmesh(pmesh);

    /* Return how many milliseconds since the call to ResetTime. */
    return ms;
}

/*
 * Mark C. Miller, Mon Jan 11 16:25:45 PST 2010
 * Removed condition on call to DBFreeMeshvar
 */
int
test_readpointvar(DBfile * dbfile, unsigned long long mask)
{
    int     ms;
    DBmeshvar *mvar = NULL;

    /* Reset the timer. */
    ResetTime();

    DBSetDataReadMask2(mask);
    printf("test_readpointvar: ");
    printMask(stdout, DBGetDataReadMask2());
    printf("\n");
    /* Read the material list and print the material struct. */
    mvar = DBGetPointvar(dbfile, "dir1/d");
    ms = ElapsedTime();
    if (mvar != NULL)
    {
        printf("test_readpointvar: mvar = {\n");
        printf("  ...\n");
        printf("    vals = 0x%p\n", mvar->vals);
        printf("  ...\n");
        printf("};\n");
    } else
    {
        fprintf(stderr, "test_readpointvar: mvar = NULL!\n");
    }

    DBFreeMeshvar(mvar);

    /* Return how many milliseconds since the call to ResetTime. */
    return ms;
}

int
test_readquadmesh(DBfile * dbfile, unsigned long long mask)
{
    int     ms;
    DBquadmesh *qmesh = NULL;

    /* Reset the timer. */
    ResetTime();

    DBSetDataReadMask2(mask);
    printf("test_readquadmesh: ");
    printMask(stdout, DBGetDataReadMask2());
    printf("\n");
    /* Read the quad mesh and print the quadmesh struct. */
    qmesh = DBGetQuadmesh(dbfile, "quadmesh2d");
    ms = ElapsedTime();
    if (qmesh != NULL)
    {
        printf("test_readquadmesh: qmesh = {\n");
        printf("  ...\n");
        printf("    name = %s\n", qmesh->name);
        printf("  ...\n");
        printf("    coords[0] = 0x%p\n", qmesh->coords[0]);
        printf("    coords[1] = 0x%p\n", qmesh->coords[1]);
        printf("    coords[2] = 0x%p\n", qmesh->coords[2]);
        printf("  ...\n");
        printf("    nnodes = %d\n", qmesh->nnodes);
        printf("  ...\n");
        printf("};\n");
    } else
    {
        fprintf(stderr, "test_readquadmesh: qmesh = NULL!\n");
    }
    DBFreeQuadmesh(qmesh);

    /* Return how many milliseconds since the call to ResetTime. */
    return ms;
}

/*
 * Mark C. Miller, Mon Jan 11 16:26:26 PST 2010
 * Removed condition on DBFreeQuadvar call.
 */
int
test_readquadvar(DBfile * dbfile, unsigned long long mask)
{
    int     ms;
    DBquadvar *qvar = NULL;

    /* Reset the timer. */
    ResetTime();

    DBSetDataReadMask2(mask);
    printf("test_readquadvar: ");
    printMask(stdout, DBGetDataReadMask2());
    printf("\n");
    /* Read the quad var and print the struct. */
    qvar = DBGetQuadvar(dbfile, "d");
    ms = ElapsedTime();
    if (qvar != NULL)
    {
        printf("test_readquadvar: qvar = {\n");
        printf("  ...\n");
        printf("    vals = 0x%p\n", qvar->vals);
        printf("  ...\n");
        printf("};\n");
    } else
    {
        fprintf(stderr, "test_readquadvar: qvar = NULL!\n");
    }

    /* This check gets us around a crash! */
    DBFreeQuadvar(qvar);

    /* Return how many milliseconds since the call to ResetTime. */
    return ms;
}

int
test_readucdmesh(DBfile * dbfile, const char *testName, unsigned long long mask)
{
    int     ms;
    DBucdmesh *umesh = NULL;

    /* Reset the timer. */
    ResetTime();

    DBSetDataReadMask2(mask);
    printf("%s: ", testName);
    printMask(stdout, DBGetDataReadMask2());
    printf("\n");
    /* Read the ucd mesh and print out the struct. */
    umesh = DBGetUcdmesh(dbfile, "mesh1");
    ms = ElapsedTime();
    if (umesh != NULL)
    {
        printf("%s: umesh = {\n", testName);
        printf("  ...\n");
        printf("    name = %s\n", umesh->name);
        printf("  ...\n");
        printf("    coords[0] = 0x%p\n", umesh->coords[0]);
        printf("    coords[1] = 0x%p\n", umesh->coords[1]);
        printf("    coords[2] = 0x%p\n", umesh->coords[2]);
        printf("  ...\n");
        printf("    faces = 0x%p\n", umesh->faces);
        printf("    zones = 0x%p\n", umesh->zones);
        if (umesh->zones != NULL)
            printf("    zones->nodelist = 0x%p\n", umesh->zones->nodelist);
        printf("    edges = 0x%p\n", umesh->edges);
        printf("  ...\n");
        printf("};\n");
    } else
    {
        fprintf(stderr, "%s: umesh = NULL!\n", testName);
    }
    DBFreeUcdmesh(umesh);

    /* Return how many milliseconds since the call to ResetTime. */
    return ms;
}

/*
 * Mark C. Miller, Mon Jan 11 16:26:51 PST 2010
 * Removed condition on DBFreeUcdvar call.
 */
int
test_readucdvar(DBfile * dbfile, unsigned long long mask)
{
    int     ms;
    DBucdvar *uvar = NULL;

    /* Reset the timer. */
    ResetTime();

    DBSetDataReadMask2(mask);
    printf("test_readucdvar: ");
    printMask(stdout, DBGetDataReadMask2());
    printf("\n");
    /* Read the quad var and print the struct. */
    uvar = DBGetUcdvar(dbfile, "u");
    ms = ElapsedTime();
    if (uvar != NULL)
    {
        printf("test_readucdvar: uvar = {\n");
        printf("  ...\n");
        printf("    vals = 0x%p\n", uvar->vals);
        printf("  ...\n");
        printf("};\n");
    } else
    {
        fprintf(stderr, "test_readucdvar: uvar = NULL!\n");
    }

    /* This check gets us around a crash! */
    DBFreeUcdvar(uvar);

    /* Return how many milliseconds since the call to ResetTime. */
    return ms;
}

int
test_readfacelist(DBfile * dbfile, unsigned long long mask)
{
    int     ms;
    DBfacelist *fl = NULL;

    /* Reset the timer. */
    ResetTime();

    DBSetDataReadMask2(mask);
    printf("test_readfacelist: ");
    printMask(stdout, DBGetDataReadMask2());
    printf("\n");
    /* Read the face list and print the struct. */
    fl = DBGetFacelist(dbfile, "fl");
    ms = ElapsedTime();
    if (fl != NULL)
    {
        printf("test_readfacelist: fl = {\n");
        printf("  ...\n");
        printf("    nodelist = 0x%p\n", fl->nodelist);
        printf("  ...\n");
        printf("    shapecnt = 0x%p\n", fl->shapecnt);
        printf("    shapesize = 0x%p\n", fl->shapesize);
        printf("    typelist = 0x%p\n", fl->typelist);
        printf("    types = 0x%p\n", fl->types);
        printf("    ntypes = %d\n", fl->ntypes);
        printf("    nodeno = 0x%p\n", fl->nodeno);
        printf("    zoneno = 0x%p\n", fl->zoneno);
        printf("};\n");
    } else
    {
        fprintf(stderr, "test_readfacelist: fl = NULL!\n");
    }
    DBFreeFacelist(fl);

    /* Return how many milliseconds since the call to ResetTime. */
    return ms;
}

int
test_readzonelist(DBfile * dbfile, unsigned long long mask)
{
    int     ms;
    DBzonelist *zl = NULL;

    /* Reset the timer. */
    ResetTime();

    DBSetDataReadMask2(mask);
    printf("test_readzonelist: ");
    printMask(stdout, DBGetDataReadMask2());
    printf("\n");
    /* Read the face list and print the struct. */
    zl = DBGetZonelist(dbfile, "zl");
    ms = ElapsedTime();
    if (zl != NULL)
    {
        printf("test_readzonelist: zl = {\n");
        printf("  ...\n");
        printf("    shapecnt = 0x%p\n", zl->shapecnt);
        printf("    shapesize = 0x%p\n", zl->shapesize);
        printf("    shapetype = 0x%p\n", zl->shapetype);
        printf("    nodelist = 0x%p\n", zl->nodelist);
        printf("  ...\n");
        printf("    zoneno = 0x%p\n", zl->zoneno);
        printf("    gzoneno = 0x%p\n", zl->gzoneno);
        printf("};\n");
    } else
    {
        fprintf(stderr, "test_readzonelist: zl = NULL!\n");
    }
    DBFreeZonelist(zl);

    /* Return how many milliseconds since the call to ResetTime. */
    return ms;
}

void
ResetTime(void)
{
#ifndef _WIN32
    /* Get the start time */
    gettimeofday(&start_time,0);
#endif
}

int
ElapsedTime(void)
{
#ifndef _WIN32
    int     ms;
    /* Get the end time */
    gettimeofday(&end_time,0);

    /* Figure out how many milliseconds the rendering took. */
    ms = (int)difftime(end_time.tv_sec, start_time.tv_sec);
    if (ms == 0)
        ms = end_time.tv_usec - start_time.tv_usec;
    else
        ms = ((ms - 1) * 1000000) + (1000000 - start_time.tv_usec) +
            end_time.tv_usec;

    /* Copy the end time into the start time. */
    memcpy((void *)&start_time, (void *)&end_time, sizeof(struct timeval));

    /* Return how many milliseconds it took to render. */
    return ms;
#else
    return 1000;
#endif
}

void
printTimes(int *ms)
{
    int i = 0;
    printf("Read material (all):   %d ms.\n", ms[i++]);
    printf("Read matnos:           %d ms.\n", ms[i++]);
    printf("Read matlist:          %d ms.\n", ms[i++]);
    printf("Read mixlist:          %d ms.\n", ms[i++]);
    printf("Read material (none):  %d ms.\n", ms[i++]);
    printf("-----------------------------\n");
    printf("Read quadmesh (all):   %d ms.\n", ms[i++]);
    printf("Read quadmesh:         %d ms.\n", ms[i++]);
    printf("Read quadmesh (none):  %d ms.\n", ms[i++]);
    printf("-----------------------------\n");
    printf("Read quadvar (all):    %d ms.\n", ms[i++]);
    printf("Read quadvar:          %d ms.\n", ms[i++]);
    printf("Read quadvar (none):   %d ms.\n", ms[i++]);
    printf("-----------------------------\n");
    printf("Read pointmesh (all):  %d ms.\n", ms[i++]);
    printf("Read pointmesh:        %d ms.\n", ms[i++]);
    printf("Read pointmesh (none): %d ms.\n", ms[i++]);
    printf("-----------------------------\n");
    printf("Read pointvar (all):   %d ms.\n", ms[i++]);
    printf("Read pointvar:         %d ms.\n", ms[i++]);
    printf("Read pointvar (none):  %d ms.\n", ms[i++]);
    printf("-----------------------------\n");
    printf("Read ucdmesh (all):    %d ms.\n", ms[i++]);
    printf("Read ucdmesh coords:   %d ms.\n", ms[i++]);
    printf("Read ucdmesh facelist: %d ms.\n", ms[i++]);
    printf("Read ucdmesh zonelist: %d ms.\n", ms[i++]);
    printf("Read ucdmesh (none):   %d ms.\n", ms[i++]);
    printf("-----------------------------\n");
    printf("Read ucdvar (all):     %d ms.\n", ms[i++]);
    printf("Read ucdvar:           %d ms.\n", ms[i++]);
    printf("Read ucdvar (none):    %d ms.\n", ms[i++]);
    printf("-----------------------------\n");
    printf("Read facelist (all):   %d ms.\n", ms[i++]);
    printf("Read facelist info:    %d ms.\n", ms[i++]);
    printf("Read facelist (none):  %d ms.\n", ms[i++]);
    printf("-----------------------------\n");
    printf("Read zonelist (all):   %d ms.\n", ms[i++]);
    printf("Read zonelist info:    %d ms.\n", ms[i++]);
    printf("Read zonelist (none):  %d ms.\n", ms[i++]);
}

void
printMask(FILE * fp, unsigned long long mask)
{
    fprintf(fp, "readMask={");
    if (mask == DBAll)
        fprintf(fp, "DBAll");
    else if (mask == DBNone)
        fprintf(fp, "DBNone");
    else
    {
        if (mask & DBCalc)              fprintf(fp, "DBCalc, ");
        if (mask & DBMatMatnos)         fprintf(fp, "DBMatMatnos, ");
        if (mask & DBMatMatlist)        fprintf(fp, "DBMatMatlist, ");
        if (mask & DBMatMixList)        fprintf(fp, "DBMatMixList, ");
        if (mask & DBCurveArrays)       fprintf(fp, "DBCurveArrays, ");
        if (mask & DBPMCoords)          fprintf(fp, "DBPMCoords, ");
        if (mask & DBPVData)            fprintf(fp, "DBPVData, ");
        if (mask & DBQMCoords)          fprintf(fp, "DBQMCoords, ");
        if (mask & DBQVData)            fprintf(fp, "DBQVData, ");
        if (mask & DBUMCoords)          fprintf(fp, "DBUMCoords, ");
        if (mask & DBUMFacelist)        fprintf(fp, "DBUMFacelist, ");
        if (mask & DBUMZonelist)        fprintf(fp, "DBUMZonelist, ");
        if (mask & DBUVData)            fprintf(fp, "DBUVData, ");
        if (mask & DBFacelistInfo)      fprintf(fp, "DBFacelistInfo, ");
        if (mask & DBZonelistInfo)      fprintf(fp, "DBZonelistInfo, ");
    }
    fprintf(fp, "};");
}
