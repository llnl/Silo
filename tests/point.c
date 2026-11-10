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

#include "silo.h"
#include <math.h>
#include <stdlib.h>
#ifdef _WIN32
#include <string.h>
#endif
#include <std.c>

#ifndef PI
#  define PI    3.141592653589793238
#endif
#define TwoPI (2*PI)

#define ALLOC(t,n)  (t*)calloc(n,sizeof(t))
#define FREE(x) free((void*)(x)); (x)=NULL
#define assert(x)   if (!(x)) \
                    { \
                        fprintf(stderr,"point: assertion failed at line %d.\n",__LINE__); \
                        exit(EXIT_SUCCESS); \
                    }
#define assert_mem(x)   if ((x) == NULL) \
                        { \
                            fprintf(stderr,"point: out of memory at line %d.\n",__LINE__); \
                            exit(EXIT_SUCCESS); \
                        }


/* Local static functions */
static void build_point(DBfile *dbfile, char *name, int N, int dims);
static void build_defvars(DBfile *dbfile, int dims);

/* Other functions */
#ifndef randf
/*-------------------------------------------------------------------------
 * Function:    randf
 * 
 * Purpose:     Generates random numbers between RMIN (inclusive) and
 *              RMAX (exclusive).  RMIN should be smaller than RMAX.
 * 
 * Return:      A pseudo-random number
 * 
 * Programmer:  Robb Matzke
 *              robb@callisto.nuance.mdn.com
 *              Jul  9, 1996
 * 
 * Modifications:
 *       Thomas R. Treadway, Wed Nov 28 15:25:53 PST 2007
 *       Moved from src/swat/randf.c
 *
 *       Mark C. Miller, Wed Jun 30 09:43:10 PDT 2010
 *       Moved here from src/silo/silo.c
 *-------------------------------------------------------------------------
 */
double
randf(double rmin, double rmax)
{   
    unsigned long   acc;
    static double   divisor = 0;

    if (divisor < 1)
        divisor = pow(2.0, 30);

    rmax -= rmin;
    acc = ((rand() & 0x7fff) << 15) | (rand() & 0x7fff);

    return (rmax * (acc / divisor) + rmin);
}
#endif


/*-------------------------------------------------------------------------
 * Function:	main
 *
 * Purpose:	
 *
 * Return:	0
 *
 * Programmer:	
 *
 * Modifications:
 * 	Robb Matzke, 1999-04-09
 *	Added argument parsing to control the driver which is used.
 *
 *-------------------------------------------------------------------------
 */
int
main(int argc, char **argv)
{
    
    DBfile         *dbfile = NULL;
    DBpointmesh    *pm = NULL;
    DBmeshvar      *pv = NULL;
    static char     meshname[] = {"pmesh"};
    int		    i, driver=DB_PDB;
    char	    filename[256], *file_ext=".pdb";
    int             show_all_errors = FALSE;

    for (i=1; i<argc; i++) {
	if (!strncmp(argv[i], "DB_PDB", 6)) {
	    driver = StringToDriver(argv[i]);
	    file_ext = ".pdb";
	} else if (!strncmp(argv[i], "DB_HDF5", 7)) {
            driver = StringToDriver(argv[i]);
	    file_ext = ".h5";
        } else if (!strcmp(argv[i], "show-all-errors")) {
            show_all_errors = 1;
	} else if (argv[i][0] != '\0') {
	    fprintf(stderr, "%s: ignored argument `%s'\n", argv[0], argv[i]);
	}
    }
    
    DBShowErrors(show_all_errors?DB_ALL_AND_DRVR:DB_ALL, NULL);

    /* Check 2D point meshes */
    sprintf(filename, "point2d%s", file_ext);
    printf("Creating file: '%s'...\n", filename);
    dbfile = DBCreate(filename, 0, DB_LOCAL, "point test file", driver);
    assert(dbfile != NULL);

    build_point(dbfile, meshname, 100, 2);
    DBClose(dbfile);

    printf("Opening file for reading.....\n");
    dbfile = DBOpen(filename, driver, DB_READ);
    assert(dbfile != NULL);
    DBSetDir(dbfile, "/dir1");

    pm = DBGetPointmesh(dbfile, meshname);
    pv = DBGetPointvar(dbfile, "d");
    DBClose(dbfile);
    DBFreePointmesh(pm);
    DBFreeMeshvar(pv);

    /* Check 3D point meshes */
    sprintf(filename, "point3d%s", file_ext);
    printf("Creating file: '%s'...\n", filename);
    dbfile = DBCreate(filename, 0, DB_LOCAL, "point test file", driver);
    assert(dbfile != NULL);

    build_point(dbfile, meshname, 100, 3);
    DBClose(dbfile);

    printf("Opening file for reading.....\n");
    dbfile = DBOpen(filename, DB_UNKNOWN, DB_READ);
    assert(dbfile != NULL);
    DBSetDir(dbfile, "/dir1");

    pm = DBGetPointmesh(dbfile, meshname);
    pv = DBGetPointvar(dbfile, "d");
    DBClose(dbfile);
    DBFreePointmesh(pm);
    DBFreeMeshvar(pv);

    CleanupDriverStuff();
    return(0);
}

/*----------------------------------------------------------------------
 *  Routine                                                  build_point
 *
 *  Purpose
 *
 *      Build point-mesh and point-vars and return the mesh ID.
 *
 * Modifications
 *
 *    Lisa J. Roberts, Fri Apr  7 10:30:29 PDT 2000
 *    Removed radius and theta, which were unused.
 *
 *    Mark C. Miller, Wed Sep 23 11:54:56 PDT 2009
 *    Added logic to test long long global node numbers option.
 *--------------------------------------------------------------------*/
static void
build_point(DBfile *dbfile, char *name, int N, int dims)
{
    float          *x = NULL, *y = NULL, *z = NULL;
    float          *u = NULL, *v = NULL, *w = NULL;
    float          *d = NULL, *t = NULL;
    float          *vm1 = NULL, *vm2 = NULL;
    int            *itype = NULL;
    long long      *litype = NULL;
    int             i;
    float          *coords[3], *vars[3];
    DBoptlist      *optlist = NULL;
    DBoptlist      *optlist1 = NULL;
    char           *ghost_labels;
    char            name2[256];

    int             one = 1;
    int             cycle = 44;
    float           time = 4.4;
    double          dtime = 4.4;

    double          missing_value1 = (float) 345.678;
    double          missing_value2 = 0;

    x = ALLOC(float,N); assert_mem(x);
    y = ALLOC(float,N); assert_mem(y);
    z = ALLOC(float,N); assert_mem(y);
    u = ALLOC(float,N); assert_mem(u);
    v = ALLOC(float,N); assert_mem(v);
    vm1 = ALLOC(float,N); assert_mem(vm1);
    vm2 = ALLOC(float,N); assert_mem(vm2);
    w = ALLOC(float,N); assert_mem(w);
    d = ALLOC(float,N); assert_mem(d);
    t = ALLOC(float,N); assert_mem(t);
    itype = ALLOC(int,N); assert_mem(itype);
    litype = ALLOC(long long,N); assert_mem(litype);
    ghost_labels = ALLOC(char,N); assert_mem(ghost_labels);

    optlist = DBMakeOptlist(10);
    optlist1 = DBMakeOptlist(10);

    DBAddOption(optlist, DBOPT_CYCLE, &cycle);
    DBAddOption(optlist, DBOPT_TIME, &time);
    DBAddOption(optlist, DBOPT_DTIME, &dtime);
    DBAddOption(optlist, DBOPT_NODENUM, itype);
    DBAddOption(optlist1, DBOPT_CYCLE, &cycle);
    DBAddOption(optlist1, DBOPT_TIME, &time);
    DBAddOption(optlist1, DBOPT_DTIME, &dtime);
    DBAddOption(optlist1, DBOPT_HIDE_FROM_GUI, &one);
    DBAddOption(optlist1, DBOPT_MISSING_VALUE, &missing_value1);

    DBMkDir(dbfile, "dir1");
    DBMkDir(dbfile, "dir2");

    /*
     * Do some random data
     */
    DBSetDir(dbfile, "/dir1");
    for (i = 0; i < N; ++i)
    {
        double          f;

        f = i / (double)(N - 1);
        x[i] = randf(-1.0,1.0);
        y[i] = randf(-1.0,1.0);
        z[i] = randf(-1.0,1.0);
        if (dims == 2)
            d[i] = sqrt(x[i]*x[i] + y[i]*y[i]);
        else
            d[i] = sqrt(x[i]*x[i] + y[i]*y[i] + z[i]*z[i]);
        u[i] = x[i] / TwoPI;
        v[i] = y[i] / TwoPI;
        vm1[i] = v[i];
        vm2[i] = v[i];
        if ((i % 7) == 0)
        {
            vm1[i] = missing_value1;
            vm2[i] = missing_value2;
        }
        w[i] = w[i] / TwoPI;
        t[i] = pow(10., 5. * f * f);
        itype[i] = i;
        if (i > 5 && i < 17)
            ghost_labels[i] = 1;
    }

    coords[0] = x;
    coords[1] = y;
    coords[2] = z;


    DBPutPointmesh(dbfile, name, dims, coords, N, DB_FLOAT, optlist);

    DBAddOption(optlist, DBOPT_GHOST_NODE_LABELS, ghost_labels);
    sprintf(name2, "%s_wghost", name);
    DBPutPointmesh(dbfile, name2, dims, coords, N, DB_FLOAT, optlist);
    DBClearOption(optlist, DBOPT_GHOST_NODE_LABELS);

    vars[0] = d;
    DBPutPointvar(dbfile, "d", name, 1, vars, N, DB_FLOAT, optlist);

    vars[0] = u;
    DBPutPointvar(dbfile, "u", name, 1, vars, N, DB_FLOAT, optlist);

    vars[0] = v;
    DBPutPointvar(dbfile, "v", name, 1, vars, N, DB_FLOAT, optlist);

    vars[0] = vm1;
    DBClearOption(optlist1, DBOPT_HIDE_FROM_GUI);
    DBPutPointvar(dbfile, "vm1", name, 1, vars, N, DB_FLOAT, optlist1);

    DBClearOption(optlist1, DBOPT_MISSING_VALUE);
    DBAddOption(optlist1, DBOPT_MISSING_VALUE, &missing_value2);
    vars[0] = vm2;
    DBPutPointvar(dbfile, "vm2", name, 1, vars, N, DB_FLOAT, optlist1);
    DBClearOption(optlist1, DBOPT_MISSING_VALUE);
    DBAddOption(optlist1, DBOPT_HIDE_FROM_GUI, &one);

    if(dims == 3)
    {
        vars[0] = w;
        DBPutPointvar(dbfile, "w", name, 1, vars, N, DB_FLOAT, optlist);
    }

    vars[0] = (float *)itype;
    DBPutPointvar(dbfile, "itype", name, 1, vars, N, DB_INT, optlist);
    
    vars[0] = t;
    DBPutPointvar(dbfile, "t", name, 1, vars, N, DB_FLOAT, optlist1);

    build_defvars(dbfile, dims);

    /*
     * Do something a little prettier
     */
    DBSetDir(dbfile, "/dir2");
    for (i = 0; i < N; ++i)
    {
        double          f;
        double          radius;
        double          theta;

        f = i / (double)(N - 1);
        theta = TwoPI * f;
        radius = theta;
        x[i] = (float) radius * cos(theta);
        y[i] = (float) radius * sin(theta);
        z[i] = randf(-1.0,1.0);
        if (dims == 2)
            d[i] = sqrt(x[i]*x[i] + y[i]*y[i]);
        else
            d[i] = sqrt(x[i]*x[i] + y[i]*y[i] + z[i]*z[i]);
        u[i] = x[i] / TwoPI;
        v[i] = y[i] / TwoPI;
        w[i] = w[i] / TwoPI;
        t[i] = pow(10., 5. * f * f);
        itype[i] = i;
        litype[i] = ((long long)1<<35) + i;
    }

    coords[0] = x;
    coords[1] = y;
    coords[2] = z;

   
    DBClearOption(optlist, DBOPT_NODENUM);
    DBAddOption(optlist, DBOPT_NODENUM, litype);
    DBAddOption(optlist, DBOPT_LLONGNZNUM, &one);
    DBPutPointmesh(dbfile, name, dims, coords, N, DB_FLOAT, optlist);

    vars[0] = d;
    DBPutPointvar(dbfile, "d", name, 1, vars, N, DB_FLOAT, optlist);

    vars[0] = u;
    DBPutPointvar(dbfile, "u", name, 1, vars, N, DB_FLOAT, optlist);

    vars[0] = v;
    DBPutPointvar(dbfile, "v", name, 1, vars, N, DB_FLOAT, optlist);

    if(dims == 3)
    {
        vars[0] = w;
        DBPutPointvar(dbfile, "w", name, 1, vars, N, DB_FLOAT, optlist);
    }

    vars[0] = (float *)itype;
    DBPutPointvar(dbfile, "itype", name, 1, vars, N, DB_INT, optlist);
    
    vars[0] = t;
    DBPutPointvar(dbfile, "t", name, 1, vars, N, DB_FLOAT, optlist);

    build_defvars(dbfile, dims);

    /*
     * The defvar's have to be printed out in every directory.
     */
    DBSetDir(dbfile, "/");
    build_defvars(dbfile, dims);

    DBFreeOptlist(optlist);
    DBFreeOptlist(optlist1);
    FREE(x);
    FREE(y);
    FREE(z);
    FREE(u);
    FREE(v);
    FREE(w);
    FREE(d);
    FREE(t);
    FREE(itype);
    FREE(litype);
    FREE(vm1);
    FREE(vm2);
    FREE(ghost_labels);
}

static void
build_defvars(DBfile *dbfile, int dims)
{
    int i;

    if (dims == 2)
    {
        /*
         * Put out the vector variables.
         */
         i = 44;
         DBWrite (dbfile, "_meshtv_defvars",
                  "vel vector {u,v};speed scalar sqrt(u*u+v*v)",
                  &i, 1, DB_CHAR);
    }
    else
    {
        /*
         * Put out the vector variables.
         */
        i = 50;
        DBWrite (dbfile, "_meshtv_defvars",
                 "vel vector {u,v,w};speed scalar sqrt(u*u+v*v+w*w)",
                 &i, 1, DB_CHAR);
    }
}
