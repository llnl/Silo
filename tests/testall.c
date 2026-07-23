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

/* File-wide modifications:
 *
 *     Sean Ahern, Wed Jan 22 15:11:09 PST 1997
 *     I reformated whitespace:  got rid of tabs and ending whitespace.  I
 *     added inclusion of standard include files to get the prototypes for
 *     standard functions.  I added prototypes for the functions defined here
 *     and made them static.  I changed all function definitions to be ANSI.
 *     I ran the file through "lint" and made appropriate changes.
 *
 *     Sean Ahern, Mon Oct 12 17:38:36 PDT 1998
 *     Removed use of the AIO library, since it's no longer supported.
 *
 *     Sean Ahern, Mon Nov  2 17:59:23 PST 1998
 *     Removed use of the coordnames in the DBPutQuadmesh and DBPutUcdmesh 
 *     calls since the parameter is ignored anyway.
 *
 *     Eric Brugger, Mon Feb 22 14:21:13 PST 1999
 *     I changed all floats used for storing intermediate values to doubles
 *     to reduce numeric errors.
 *
 */

#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <math.h>
#include <stdlib.h>
#ifndef _WIN32
#include <unistd.h>
#else
typedef ptrdiff_t ssize_t;
#include <stdio.h>
#include <io.h>
#endif
#include <string.h>
#include "silo.h"
#include <std.c>

#define false   0
#define true    1

#define Min(A,B) (((A)<(B))?(A):(B))
#define Max(A,B) (((A)<(B))?(B):(A))
#ifndef M_PI        /* yea, Solaris 5 */
#define M_PI        3.14159265358979323846264338327950288   /* pi */
#endif
#define  ALLOC_N(x,n)     (x *) calloc (n, sizeof (x))
#define  FREE(x)          if ( (x) != NULL) {free(x);(x)=NULL;}

/* Local function prototypes */
static void     fill_rect2d_bkgr(int matlist[], int nx, int ny, int matno);
static void     fill_rect2d_mat(float x[], float y[], int matlist[], int nx,
                                int ny, int mix_next[], int mix_mat[],
                                int mix_zone[], float mix_vf[], int *mixlen,
                                int matno, double radius);
static void     fill_rect3d_bkgr(int matlist[], int nx, int ny, int nz,
                                 int matno);
static void     fill_rect3d_mat(float x[], float y[], float z[], int matlist[],
                                int nx, int ny, int nz, int mix_next[],
                                int mix_mat[], int mix_zone[], float mix_vf[],
                                int *mixlen, int matno, double radius);
static void     build_rect2d(DBfile * dbfile, int size, int order);
static void     build_curv2d(DBfile * dbfile, int size, int order);
static void     build_ucd2d(DBfile * dbfile, int size, int order);
static void     build_rect3d(DBfile * dbfile, int size, int order);
static void     build_curv3d(DBfile * dbfile, int size, int order);
static void     build_ucd3d(DBfile * dbfile, int size, int order);
static void     build_poly3d(DBfile * dbfile, int size, int order);
static void     build_carray(DBfile * dbfile);
static void     build_curve(DBfile * dbfile, int driver);
static void     MakeFiles(char *suffix, int size, int order, int type);

static void
fill_rect2d_bkgr(int matlist[], int nx, int ny, int matno)
{
    int       i, j;

    for (i = 0; i < nx; i++) {
        for (j = 0; j < ny; j++) {
            matlist[j * nx + i] = matno;
        }
    }
}

static void
fill_rect2d_mat(float x[], float y[], int matlist[], int nx, int ny, int
                mix_next[], int mix_mat[], int mix_zone[], float mix_vf[], int
                *mixlen, int matno, double radius)
{
    int       i, j, k, l;
    double    xcenter, ycenter;
    double    dist;
    int       cnt;
    int       mixlen2;
    int       *itemp;
    double    dx, dy;
    double    xx[20], yy[20];

    mixlen2 = *mixlen;

    itemp = ALLOC_N (int, (nx + 1) * (ny + 1));

    xcenter = .5;
    ycenter = .5;
    for (i = 0; i < nx + 1; i++) {
        for (j = 0; j < ny + 1; j++) {
            dist = sqrt((x[i] - xcenter) * (x[i] - xcenter) +
                        (y[j] - ycenter) * (y[j] - ycenter));
            itemp[j * (nx + 1) + i] = (dist < radius) ? 1 : 0;
        }
    }
    for (i = 0; i < nx; i++) {
        for (j = 0; j < ny; j++) {
            cnt = itemp[(j) * (nx + 1) + (i)] +
                itemp[(j + 1) * (nx + 1) + (i)] +
                itemp[(j + 1) * (nx + 1) + (i + 1)] +
                itemp[(j) * (nx + 1) + (i + 1)];
            if (cnt == 0) {
                /* EMPTY */
            }
            else if (cnt == 4) {
                matlist[j * nx + i] = matno;
            }
            else {
                dx = (x[i + 1] - x[i]) / 21.;
                dy = (y[j + 1] - y[j]) / 21.;
                for (k = 0; k < 20; k++) {
                    xx[k] = x[i] + (dx / 2.) + (k * dx);
                    yy[k] = y[j] + (dy / 2.) + (k * dy);
                }
                cnt = 0;
                for (k = 0; k < 20; k++) {
                    for (l = 0; l < 20; l++) {
                        dist = sqrt((xx[k] - xcenter) *
                                    (xx[k] - xcenter) +
                                    (yy[l] - ycenter) *
                                    (yy[l] - ycenter));
                        cnt += (dist < radius) ? 1 : 0;
                    }
                }
                mix_mat[mixlen2] = matlist[j * nx + i];
                mix_mat[mixlen2 + 1] = matno;
                mix_next[mixlen2] = mixlen2 + 2;
                mix_next[mixlen2 + 1] = 0;
                mix_zone[mixlen2] = j * nx + i;
                mix_zone[mixlen2 + 1] = j * nx + i;
                mix_vf[mixlen2] = 1. - (((double)cnt) / 400.);
                mix_vf[mixlen2 + 1] = ((double)cnt) / 400.;
                matlist[j * nx + i] = - (mixlen2 + 1);
                mixlen2 += 2;
            }
        }
    }

    FREE (itemp);

    *mixlen = mixlen2;
}

static void
fill_rect3d_bkgr(int matlist[], int nx, int ny, int nz, int matno)
{
    int       i, j, k;

    for (i = 0; i < nx; i++) {
        for (j = 0; j < ny; j++) {
            for (k = 0; k < nz; k++) {
                matlist[k * nx * ny + j * nx + i] = matno;
            }
        }
    }
}

static void
fill_rect3d_mat(float x[], float y[], float z[], int matlist[], int nx,
                int ny, int nz, int mix_next[], int mix_mat[], int mix_zone[],
                float mix_vf[], int *mixlen, int matno, double radius)
{
    int       i, j, k, l, m, n;
    double    xcenter, ycenter, zcenter;
    double    dist;
    int       cnt;
    int       mixlen2;
    int       *itemp;
    double    dx, dy, dz;
    double    xx[10], yy[10], zz[10];

    mixlen2 = *mixlen;

    itemp = ALLOC_N (int, (nx + 1) * (ny + 1) * (nz + 1));

    xcenter = .5;
    ycenter = .5;
    zcenter = .5;
    for (i = 0; i < nx + 1; i++) {
        for (j = 0; j < ny + 1; j++) {
            for (k = 0; k < nz + 1; k++) {
                dist = sqrt((x[i] - xcenter) * (x[i] - xcenter) +
                            (y[j] - ycenter) * (y[j] - ycenter) +
                            (z[k] - zcenter) * (z[k] - zcenter));
                itemp[k * (nx + 1) * (ny + 1) + j * (nx + 1) + i] =
                    (dist < radius) ? 1 : 0;
            }
        }
    }
    for (i = 0; i < nx; i++) {
        for (j = 0; j < ny; j++) {
            for (k = 0; k < nz; k++) {
                cnt = itemp[(i) * (nx + 1) * (ny + 1) + (j) * (nx + 1) + k] +
                    itemp[(i) * (nx + 1) * (ny + 1) + (j + 1) * (nx + 1) + k] +
                    itemp[(i) * (nx + 1) * (ny + 1) + (j + 1) * (nx + 1) + k + 1] +
                    itemp[(i) * (nx + 1) * (ny + 1) + (j) * (nx + 1) + k + 1] +
                    itemp[(i + 1) * (nx + 1) * (ny + 1) + (j) * (nx + 1) + k] +
                    itemp[(i + 1) * (nx + 1) * (ny + 1) + (j + 1) * (nx + 1) + k] +
                    itemp[(i + 1) * (nx + 1) * (ny + 1) + (j + 1) * (nx + 1) + k + 1] +
                    itemp[(i + 1) * (nx + 1) * (ny + 1) + (j) * (nx + 1) + k + 1];
                if (cnt == 0) {
                    /* EMPTY */
                }
                else if (cnt == 8) {
                    matlist[i * nx * ny + j * nx + k] = matno;
                }
                else {
                    dx = (x[i + 1] - x[i]) / 11.;
                    dy = (y[j + 1] - y[j]) / 11.;
                    dz = (z[k + 1] - z[k]) / 11.;
                    for (l = 0; l < 10; l++) {
                        xx[l] = x[i] + (dx / 2.) + (l * dx);
                        yy[l] = y[j] + (dy / 2.) + (l * dy);
                        zz[l] = z[k] + (dz / 2.) + (l * dz);
                    }
                    cnt = 0;
                    for (l = 0; l < 10; l++) {
                        for (m = 0; m < 10; m++) {
                            for (n = 0; n < 10; n++) {
                                dist = sqrt((xx[l] - xcenter) *
                                            (xx[l] - xcenter) +
                                            (yy[m] - ycenter) *
                                            (yy[m] - ycenter) +
                                            (zz[n] - zcenter) *
                                            (zz[n] - zcenter));
                                cnt += (dist < radius) ? 1 : 0;
                            }
                        }
                    }
                    mix_mat[mixlen2] = matlist[i * nx * ny + j * nx + k];
                    mix_mat[mixlen2 + 1] = matno;
                    mix_next[mixlen2] = mixlen2 + 2;
                    mix_next[mixlen2 + 1] = 0;
                    mix_zone[mixlen2] = i * nx * ny + j * nx + k;
                    mix_zone[mixlen2 + 1] = i * nx * ny + j * nx + k;
                    mix_vf[mixlen2] = 1. - (((double)cnt) / 1000.);
                    mix_vf[mixlen2 + 1] = ((double)cnt) / 1000.;
                    matlist[i * nx * ny + j * nx + k] = -(mixlen2 + 1);
                    mixlen2 += 2;
                }
            }
        }
    }

    FREE (itemp);

    *mixlen = mixlen2;
}

/*-------------------------------------------------------------------------
 * Function:    build_rect2d
 *
 * Purpose:     Builds a rectilinear 2-d mesh and places it in the open
 *              data file.
 *
 * Return:      Success:        void
 *
 *              Failure:
 *
 * Programmer:  robb@cloud
 *              Wed Nov 23 10:13:51 EST 1994
 *
 * Modifications:
 *     Sean Ahern, Thu Jun 20 08:53:11 PDT 1996
 *     Changed the mesh name to be more mnemonic.
 *
 *     Sean Ahern, Fri Oct 11 17:08:42 PDT 1996
 *     Changed the mesh name in the _meshtvinfo variable.
 *
 *     Jim Reus, Wed Nov 13 07:24:24 PST 1996
 *     Added temperature (t) variable to demonstrate logplots.
 *     ...temperature is positive non-zero, linear in x and
 *        exponential in y.
 *
 *     Eric Brugger, Tue Nov 26 11:42:47 PST 1996
 *     I added the outputing of some meshtv defvars to the silo file.
 *
 *     Sean Ahern, Wed Jan 22 15:16:58 PST 1997
 *     Initialized some local pointer variables to NULL.  Got rid of unused
 *     variables and variables that were being set but not used.
 *
 *     Robb Matzke, Wed Jun 18 11:02:17 EST 1997
 *     Added the `ascii' variable to check labeling by ascii value in
 *     meshtvx.
 *
 *     Eric Brugger, Mon Sep 20 19:10:11 PDT 1999
 *     I modified the material numbers so that they are no longer
 *     numbered consecutively.
 *
 *     Sean Ahern, Thu Feb  7 13:49:18 PST 2002
 *     Added material names.
 *
 *-------------------------------------------------------------------------*/
/*ARGSUSED*/
static void
build_rect2d(DBfile * dbfile, int size, int order)
{
    int            cycle;
    float          time;
    double         dtime;
    int            ndims;
    int            nx, ny;
    int            dims[3], zdims[3];
    float         *coords[3];
    float         *x = NULL, *y = NULL;

    char          *meshname = NULL, *var1name = NULL, *var2name = NULL;
    char          *var3name = NULL, *var4name = NULL, *matname = NULL;

    float         *d=NULL, *p=NULL, *u=NULL, *v=NULL, *t=NULL, *distarr=NULL;

    char          *ascii=NULL, *asciiw=NULL;

    int            nmats;
    int            matnos[9];
    int           *matlist = NULL;
    int            dims2[3];
    int            mixlen;
    int           *mix_next = NULL, *mix_mat = NULL, *mix_zone = NULL;
    float         *mix_vf = NULL;

    DBoptlist     *optlist = NULL;
    char         **matnames = NULL;
    char         **matcolors = NULL;

    int            one = 1;
    int            i, j;
    double         xave, yave;
    double         xcenter, ycenter;
    double         dist;

    switch (size) {
        case 1:
            nx = 30;
            ny = 40;
            break;
        case 2:
            nx = 300;
            ny = 400;
            break;
        case 3:
            nx = 1000;
            ny = 1200;
            break;
    }

    x = ALLOC_N (float, nx + 1);
    y = ALLOC_N (float, ny + 1);
    d = ALLOC_N (float, nx * ny);
    p = ALLOC_N (float, nx * ny);
    u = ALLOC_N (float, (nx + 1) * (ny + 1));
    v = ALLOC_N (float, (nx + 1) * (ny + 1));
    t = ALLOC_N (float, (nx + 1) * (ny + 1));
    distarr = ALLOC_N (float, nx * ny);
    ascii = ALLOC_N (char, nx * ny);
    asciiw = ALLOC_N (char, nx * ny * 9);
    matlist = ALLOC_N (int, nx * ny);
    mix_next = ALLOC_N (int, 40 * ny);
    mix_mat  = ALLOC_N (int, 40 * ny);
    mix_zone = ALLOC_N (int, 40 * ny);
    mix_vf   = ALLOC_N (float, 40 * ny);

    /*
     * Create the mesh.
     */
    meshname = "quadmesh2d";
    coords[0] = x;
    coords[1] = y;
    ndims = 2;
    dims[0] = nx + 1;
    dims[1] = ny + 1;
    for (i = 0; i < nx + 1; i++)
        x[i] = i * (1. / nx);
    for (i = 0; i < ny + 1; i++)
        y[i] = i * (1. / nx);

    /*
     * Create the density and pressure arrays.
     */
    var1name = "d";
    var2name = "p";
    xcenter = .5;
    ycenter = .5;
    zdims[0] = nx;
    zdims[1] = ny;
    for (i = 0; i < nx; i++)
    {
        for (j = 0; j < ny; j++)
        {
            xave = (x[i]+x[i+1]) / 2.0 ;
            yave = (y[j]+y[j+1]) / 2.0 ;

            dist = sqrt((xave - xcenter) * (xave - xcenter) +
                        (yave - ycenter) * (yave - ycenter));

            d[j * nx + i] = dist;
            p[j * nx + i] = 1 / (dist + 0.0001);
        }
    }

    /*
     * Create the velocity component arrays.
     */
    var3name = "u";
    var4name = "v";
    xcenter = .5001;
    ycenter = .5001;
    for (i = 0; i < nx + 1; i++) {
        for (j = 0; j < ny + 1; j++) {
            dist = sqrt((x[i] - xcenter) * (x[i] - xcenter) +
                        (y[j] - ycenter) * (y[j] - ycenter));
            u[j * (nx + 1) + i] = (x[i] - xcenter) / dist;
            v[j * (nx + 1) + i] = (y[j] - ycenter) / dist;
        }
    }

    /*
     * Create the temperature array.
     */
    if (t != NULL)
    {
       double a,b,c,d;

       a = 1.;
       b = 3.;
       c = 5.;
       d = 7.;
       for (i=0; i<=nx; ++i)
       {
          double x;

          x = i / (double) nx;
          for (j=0; j<=ny; ++j)
          {
             double y;

             y = j / (double) ny;
             t[j*(nx+1) + i] = a + b*x + c*exp(d*y);
          }
       }
    }

    /*
     * Create the ascii label array.  The center of the mesh is labeled
     * `A' and the furthest from the center is labeled `Z'.
     */
    if (ascii) {
       double maxdist = 0.0;
       xcenter = .5;
       ycenter = .5;
       for (i=0; i<nx; i++) {
          for (j=0; j<ny; j++) {
            distarr[j*nx+i] = sqrt ((x[i] - xcenter) * (x[i] - xcenter) +
                                 (y[j] - ycenter) * (y[j] - ycenter));
            if (distarr[j*nx+i]>maxdist) maxdist = distarr[j*nx+i];
          }
       }
       for (i=0; i<nx*ny; i++) {
          ascii[i] = (char) ((int) ('A' + 26*distarr[i]/maxdist));
          asciiw[0*nx*ny+i] = 'M';
          asciiw[1*nx*ny+i] = 'a';
          asciiw[2*nx*ny+i] = 'r';
          asciiw[3*nx*ny+i] = 'k';
          asciiw[4*nx*ny+i] = ' ';
          asciiw[5*nx*ny+i] = 'M';
          asciiw[6*nx*ny+i] = 'i';
          asciiw[7*nx*ny+i] = 'l';
          asciiw[8*nx*ny+i] = '\0';
       }
    }

    /*
     * Create the material array.
     */
    matname = "mat1";
    nmats = 9;
    matnos[0] = 1;
    matnos[1] = 3;
    matnos[2] = 6;
    matnos[3] = 7;
    matnos[4] = 8;
    matnos[5] = 11;
    matnos[6] = 12;
    matnos[7] = 16;
    matnos[8] = 18;
    dims2[0] = nx;
    dims2[1] = ny;
    mixlen = 0;
    matnames = (char**)malloc(sizeof(char*)*nmats);
    matnames[0] = _db_safe_strdup("Green eggs");
    matnames[1] = _db_safe_strdup("Ham");
    matnames[2] = _db_safe_strdup("Air");
    matnames[3] = _db_safe_strdup("Ozone");
    matnames[4] = _db_safe_strdup("Diamond");
    matnames[5] = _db_safe_strdup("Pure water");
    matnames[6] = _db_safe_strdup("C8H10N4O2");
    matnames[7] = _db_safe_strdup("Wood");
    matnames[8] = _db_safe_strdup("Nothing important");

    matcolors = (char**)malloc(sizeof(char*)*nmats);
    matcolors[0] = _db_safe_strdup("Green");
    matcolors[1] = _db_safe_strdup("Red");
    matcolors[2] = _db_safe_strdup("Light Blue");
    matcolors[3] = _db_safe_strdup("Yellow");
    matcolors[4] = _db_safe_strdup("White");
    matcolors[5] = _db_safe_strdup("Blue");
    matcolors[6] = _db_safe_strdup("Black");
    matcolors[7] = _db_safe_strdup("Light Brown");
    matcolors[8] = _db_safe_strdup("#fcfcfc");

    /*
     * Put in material 1 as the background then overlay materials
     * 2 through 9.
     */
    fill_rect2d_bkgr (matlist, nx, ny, 1);

    fill_rect2d_mat (x, y, matlist, nx, ny, mix_next, mix_mat, mix_zone,
                     mix_vf, &mixlen, 3, 0.8);
    fill_rect2d_mat (x, y, matlist, nx, ny, mix_next, mix_mat, mix_zone,
                     mix_vf, &mixlen, 6, 0.7);
    fill_rect2d_mat (x, y, matlist, nx, ny, mix_next, mix_mat, mix_zone,
                     mix_vf, &mixlen, 7, 0.6);
    fill_rect2d_mat (x, y, matlist, nx, ny, mix_next, mix_mat, mix_zone,
                     mix_vf, &mixlen, 8, 0.5);
    fill_rect2d_mat (x, y, matlist, nx, ny, mix_next, mix_mat, mix_zone,
                     mix_vf, &mixlen, 11, 0.4);
    fill_rect2d_mat (x, y, matlist, nx, ny, mix_next, mix_mat, mix_zone,
                     mix_vf, &mixlen, 12, 0.3);
    fill_rect2d_mat (x, y, matlist, nx, ny, mix_next, mix_mat, mix_zone,
                     mix_vf, &mixlen, 16, 0.2);
    fill_rect2d_mat (x, y, matlist, nx, ny, mix_next, mix_mat, mix_zone,
                     mix_vf, &mixlen, 18, 0.1);

    if (mixlen > 40 * ny) {
        printf ("mixlen = %d\n", mixlen);
        exit(EXIT_SUCCESS);
    }

    /*
     * Write out the variables.
     */
    cycle = 48;
    time = 4.8;
    dtime = 4.8;

    /*
     * The length includes the terminating NULL character.
     */
    i = 30;
    DBWrite (dbfile, "_meshtvinfo", "mesh quadmesh2d;pseudocolor d",
             &i, 1, DB_CHAR);
    i = 44;
    DBWrite (dbfile, "_meshtv_defvars",
             "vel vector {u,v};speed scalar sqrt(u*u+v*v)",
             &i, 1, DB_CHAR);

    optlist = DBMakeOptlist(15);
    DBAddOption(optlist, DBOPT_CYCLE, &cycle);
    DBAddOption(optlist, DBOPT_TIME, &time);
    DBAddOption(optlist, DBOPT_DTIME, &dtime);
    DBAddOption(optlist, DBOPT_XLABEL, (char *) "X Axis");
    DBAddOption(optlist, DBOPT_YLABEL, (char *) "Y Axis");
    DBAddOption(optlist, DBOPT_XUNITS, (char *) "cm");
    DBAddOption(optlist, DBOPT_YUNITS, (char *) "cm");
    DBAddOption(optlist, DBOPT_ALLOWMAT0, &one);
    DBAddOption(optlist, DBOPT_MATNAMES, matnames);
    DBAddOption(optlist, DBOPT_MATCOLORS, matcolors);
#if 0
    i = DB_COLMAJOR;
    DBAddOption(optlist, DBOPT_MAJORORDER, &i);
#endif

    DBPutQuadmesh(dbfile, meshname, NULL, coords, dims, ndims,
                  DB_FLOAT, DB_COLLINEAR, optlist);
    DBPutQuadvar1(dbfile, var1name, meshname, d, zdims, ndims, NULL, 0,
                           DB_FLOAT, DB_ZONECENT, optlist);

    DBPutQuadvar1(dbfile, var2name, meshname, p, zdims, ndims, NULL, 0,
                  DB_FLOAT, DB_ZONECENT, optlist);

    DBPutQuadvar1(dbfile, var3name, meshname, u, dims, ndims, NULL, 0,
                  DB_FLOAT, DB_NODECENT, optlist);

    DBPutQuadvar1(dbfile, var4name, meshname, v, dims, ndims, NULL, 0,
                  DB_FLOAT, DB_NODECENT, optlist);

    DBPutQuadvar1(dbfile, "t", meshname, t, dims, ndims, NULL, 0, DB_FLOAT,
                  DB_NODECENT, optlist);

    DBPutMaterial(dbfile, matname, meshname, nmats, matnos, matlist, dims2,
                  ndims, mix_next, mix_mat, mix_zone, mix_vf, mixlen,
                  DB_FLOAT, optlist);

    if (ascii) {
       void *arr[9];
       char *arrnames[] = {"L0","L1","L2","L3","L4","L5","L6","L7","L8"};
       j = true;

       DBAddOption (optlist, DBOPT_ASCII_LABEL, &j);
       DBPutQuadvar1(dbfile, "ascii", meshname, ascii, zdims, ndims, NULL, 0,
                     DB_CHAR, DB_ZONECENT, optlist);
       arr[0] = &asciiw[0*nx*ny];
       arr[1] = &asciiw[1*nx*ny];
       arr[2] = &asciiw[2*nx*ny];
       arr[3] = &asciiw[3*nx*ny];
       arr[4] = &asciiw[4*nx*ny];
       arr[5] = &asciiw[5*nx*ny];
       arr[6] = &asciiw[6*nx*ny];
       arr[7] = &asciiw[7*nx*ny];
       arr[8] = &asciiw[8*nx*ny];
       DBPutQuadvar(dbfile, "asciiw", meshname, 9, (DBCAS_t) arrnames,
           arr, zdims, ndims, NULL, 0, DB_CHAR, DB_ZONECENT, optlist);
    }

    DBFreeOptlist(optlist);

    /*
     * Free the temporary storage.
     */
    FREE (x);
    FREE (y);
    FREE (d);
    FREE (p);
    FREE (u);
    FREE (v);
    FREE (t);
    FREE (distarr);
    FREE (ascii);
    FREE (asciiw);
    FREE (matlist);
    FREE (mix_next);
    FREE (mix_mat);
    FREE (mix_zone);
    FREE (mix_vf);
    for(i=0;i<nmats;i++)
    {
        FREE(matnames[i]);
        FREE(matcolors[i]);
    }
    FREE(matnames);
    FREE(matcolors);
}

/*-------------------------------------------------------------------------
 * Function:    build_curv2d
 *
 * Purpose:     Build a 2-d curvilinear mesh and place it in the open
 *              database.
 *
 * Return:      Success:        void
 *
 *              Failure:
 *
 * Programmer:
 *
 * Modifications:
 *    Robb Matzke, Sun Dec 18 17:39:27 EST 1994
 *    Fixed memory leak.
 *
 *    Eric Brugger, Fri Mar 15 09:54:58 PST 1996
 *    I added upper and lower offsets to the mesh and variables.
 *
 *    Sean Ahern, Thu Jun 20 08:53:11 PDT 1996
 *    Changed the mesh name to be more mnemonic.
 *
 *    Eric Brugger, Tue Jun 25 08:07:36 PDT 1996
 *    I added mixed zones to the material variable.
 *
 *    Eric Brugger, Tue Nov 26 11:42:47 PST 1996
 *    I added the outputing of some meshtv defvars to the silo file.
 *
 *    Sean Ahern, Wed Jan 22 15:16:58 PST 1997
 *    Got rid of unused variables and variables that were being set but not
 *    used.
 *
 *    Sean Ahern, Thu Feb  7 13:56:32 PST 2002
 *    Added material names.
 *
 *-------------------------------------------------------------------------*/
static void
build_curv2d(DBfile * dbfile, int size, int order)
{
    int            cycle;
    float          time;
    double         dtime;
    int            ndims;
    int            nx, ny;
    int            sn1, sn2, sz1, sz2;
    int            dims[3], zdims[3];
    int            lo_off[3], hi_off[3];
    float         *coords[3];
    float         *x, *y;

    char          *meshname, *var1name, *var2name, *var3name;
    char          *var4name, *matname;
    double        *d, *p;
    float         *u, *v;

    int            nmats;
    int            matnos[3];
    int           *matlist;
    int            dims2[3];
    int            mixlen;
    int           *mix_next, *mix_mat, *mix_zone;
    float         *mix_vf;

    DBoptlist     *optlist;
    char **matnames = NULL;

    int            i, j;
    double         xave, yave;
    double         xcenter, ycenter;
    double         theta, dtheta;
    double         r, dr;
    double         dist;
    float         *xx, *yy;

    switch (size) {
        case 1:
            nx = 30;
            ny = 40;
            break;
        case 2:
            nx = 300;
            ny = 400;
            break;
        case 3:
            nx = 1000;
            ny = 1200;
            break;
    }

    if (order == DB_ROWMAJOR) {
        sz1 = 1;
        sz2 = nx;
        sn1 = 1;
        sn2 = nx + 1;
    }
    else {
        sz1 = ny;
        sz2 = 1;
        sn1 = ny + 1;
        sn2 = 1;
    }

    x = ALLOC_N (float, (nx + 1) * (ny + 1));
    y = ALLOC_N (float, (nx + 1) * (ny + 1));
    d = ALLOC_N (double, nx * ny);
    p = ALLOC_N (double, nx * ny);
    u = ALLOC_N (float, (nx + 1) * (ny + 1));
    v = ALLOC_N (float, (nx + 1) * (ny + 1));
    xx = ALLOC_N (float, (nx + 1) * 2);
    yy = ALLOC_N (float, (nx + 1) * 2);
    matlist = ALLOC_N (int, nx * ny);
    mix_next = ALLOC_N (int, 2 * nx);
    mix_mat  = ALLOC_N (int, 2 * nx);
    mix_zone = ALLOC_N (int, 2 * nx);
    mix_vf   = ALLOC_N (float, 2 * nx);

    /*
     * Create the mesh.
     */
    meshname = "curvmesh2d";
    coords[0] = x;
    coords[1] = y;
    ndims = 2;
    dims[0] = nx + 1;
    dims[1] = ny + 1;
    dtheta = (180. / nx) * (3.1415926 / 180.);
    dr = 3. / ny;
    theta = 0;
    for (i = 0; i < nx + 1; i++) {
        r = 2.;
        for (j = 0; j < ny + 1; j++) {
            x[j * sn2 + i * sn1] = r * cos(theta);
            y[j * sn2 + i * sn1] = r * sin(theta);
            r += dr;
        }
        theta += dtheta;
    }

    /*
     * Create the density and pressure arrays.
     */
    var1name = "d";
    var2name = "p";
    xcenter = 0.;
    ycenter = 0.;
    zdims[0] = nx;
    zdims[1] = ny;
    for (i = 0; i < nx; i++) {
        for (j = 0; j < ny; j++) {
            xave = (x[j * sn2 + i * sn1] +
                    x[j * sn2 + (i + 1) * sn1] +
                    x[(j + 1) * sn2 + (i + 1) * sn1] +
                    x[(j + 1) * sn2 + i * sn1]) / 4.;
            yave = (y[j * sn2 + i * sn1] +
                    y[j * sn2 + (i + 1) * sn1] +
                    y[(j + 1) * sn2 + (i + 1) * sn1] +
                    y[(j + 1) * sn2 + i * sn1]) / 4.;
            dist = sqrt((xave - xcenter) * (xave - xcenter) +
                        (yave - ycenter) * (yave - ycenter));
            d[j * sz2 + i * sz1] = dist;
            p[j * sz2 + i * sz1] = 1. / (dist + .0001);
        }
    }

    /*
     * Create the velocity component arrays. The values from the
     * first 2 rows of the x and y arrays are first copied into the
     * arrays xx and yy which are stored in the fortran order.  The
     * indexing on the xx and yy arrays is then done for rectilinear
     * meshes. This generates an interesting vector field.
     */
    var3name = "u";
    var4name = "v";
    xcenter = 0.;
    ycenter = 0.;
    for (i = 0; i < nx + 1; i++) {
        for (j = 0; j < 2; j++) {
            xx [j * (nx + 1) + i] = x [j * sn2 + i * sn1];
            yy [j * (nx + 1) + i] = y [j * sn2 + i * sn1];
        }
    }
    for (i = 0; i < nx + 1; i++) {
        for (j = 0; j < ny + 1; j++) {
            dist = sqrt((xx[i] - xcenter) * (xx[i] - xcenter) +
                        (yy[j] - ycenter) * (yy[j] - ycenter));
            u[j * sn2 + i * sn1] = (xx[i] - xcenter) / dist;
            v[j * sn2 + i * sn1] = (yy[j] - ycenter) / dist;
        }
    }

    /*
     * Create the material array.
     */
    matname = "mat1";
    nmats = 3;
    matnos[0] = 1;
    matnos[1] = 2;
    matnos[2] = 3;
    dims2[0] = nx;
    dims2[1] = ny;
    mixlen = 0;
    matnames = (char**)malloc(sizeof(char*)*nmats);
    matnames[0] = _db_safe_strdup("First circle of Hell");
    matnames[1] = _db_safe_strdup("Second circle of Hell");
    matnames[2] = _db_safe_strdup("Third circle of Hell");

    /*
     * Put in the material in 3 shells.
     */
    for (i = 0; i < nx; i++) {
        /*
         * Add 9 layers of material 1.
         */
        for (j = 0; j < 9; j++) {
            matlist[j * sz2 + i * sz1] = 1;
        }

        /*
         * Add a layer of mixed material zones.
         */
        j = 9;
        matlist[j * sz2 + i * sz1] = - mixlen - 1;
        mix_next [mixlen]     = mixlen + 2;
        mix_next [mixlen + 1] = 0;
        mix_mat  [mixlen]     = 1;
        mix_mat  [mixlen + 1] = 2;
        mix_zone [mixlen]     = j * sz2 + i * sz1;
        mix_zone [mixlen + 1] = j * sz2 + i * sz1;
        mix_vf   [mixlen]     = 0.4;
        mix_vf   [mixlen + 1] = 0.6;
        mixlen += 2;

        /*
         * Add 10 layers of material 2.
         */
        for (j = 10; j < 20; j++) {
            matlist[j * sz2 + i * sz1] = 2;
        }

        /*
         * Make the remainder of layers material 3.
         */
        for (j = 20; j < ny; j++) {
            matlist[j * sz2 + i * sz1] = 3;
        }
    }

    /*
     * Write out the variables.
     */
    cycle = 48;
    time = 4.8;
    dtime = 4.8;
    lo_off [0] = 1;
    lo_off [1] = 1;
    hi_off [0] = 3;
    hi_off [1] = 1;

    /*
     * The length includes the terminating NULL character.
     */
    i = 30;
    DBWrite (dbfile, "_meshtvinfo", "mesh curvmesh2d;pseudocolor d",
             &i, 1, DB_CHAR);
    i = 44;
    DBWrite (dbfile, "_meshtv_defvars",
             "vel vector {u,v};speed scalar sqrt(u*u+v*v)",
             &i, 1, DB_CHAR);

    optlist = DBMakeOptlist(11);
    DBAddOption(optlist, DBOPT_CYCLE, &cycle);
    DBAddOption(optlist, DBOPT_TIME, &time);
    DBAddOption(optlist, DBOPT_DTIME, &dtime);
    DBAddOption(optlist, DBOPT_XLABEL, (char *) "X Axis");
    DBAddOption(optlist, DBOPT_YLABEL, (char *) "Y Axis");
    DBAddOption(optlist, DBOPT_XUNITS, (char *) "cm");
    DBAddOption(optlist, DBOPT_YUNITS, (char *) "cm");
    DBAddOption(optlist, DBOPT_LO_OFFSET, &lo_off);
    DBAddOption(optlist, DBOPT_HI_OFFSET, &hi_off);
    DBAddOption(optlist, DBOPT_MAJORORDER, &order);
    DBAddOption(optlist, DBOPT_MATNAMES, matnames);

    DBPutQuadmesh(dbfile, meshname, NULL, coords, dims, ndims, DB_FLOAT,
                  DB_NONCOLLINEAR, optlist);

    DBPutQuadvar1(dbfile, var1name, meshname, (float *)d, zdims, ndims, NULL,
                  0, DB_DOUBLE, DB_ZONECENT, optlist);

    DBPutQuadvar1(dbfile, var2name, meshname, (float *)p, zdims, ndims, NULL,
                  0, DB_DOUBLE, DB_ZONECENT, optlist);

    DBPutQuadvar1(dbfile, var3name, meshname, u, dims, ndims, NULL, 0,
                  DB_FLOAT, DB_NODECENT, optlist);

    DBPutQuadvar1(dbfile, var4name, meshname, v, dims, ndims, NULL, 0,
                  DB_FLOAT, DB_NODECENT, optlist);

    DBPutMaterial(dbfile, matname, meshname, nmats, matnos, matlist, dims2,
                  ndims, mix_next, mix_mat, mix_zone, mix_vf, mixlen,
                  DB_FLOAT, optlist);

    DBFreeOptlist(optlist);

    /*
     * Free the temporary storage.
     */
    FREE (x);
    FREE (y);
    FREE (d);
    FREE (p);
    FREE (u);
    FREE (v);
    FREE (xx);
    FREE (yy);
    FREE (matlist);
    FREE (mix_vf);
    FREE (mix_zone);
    FREE (mix_mat);
    FREE (mix_next);
    for(i=0;i<nmats;i++)
        FREE(matnames[i]);
    FREE(matnames);
}

/*-------------------------------------------------------------------------
 * Function:    build_ucd2d
 *
 * Purpose:     Build a 2-d UCD mesh and add it to the open database.
 *
 * Return:      Success:        void
 *
 *              Failure:
 *
 * Programmer:  Sean Ahern, Tue Mar 24 16:09:06 PST 1998
 *
 * Modifications:
 *  Sean Ahern, Thu Feb  7 14:22:40 PST 2002
 *  Added material names.
 *
 *-------------------------------------------------------------------------*/
/*ARGSUSED*/
static void
build_ucd2d(DBfile * dbfile, int size, int order)
{
    int             cycle;
    float           time;
    double          dtime;
    int             td;
    float          *coords[2];
    int             dims[1];
    float           x[13], y[13];
    float           d[13],u[13],v[13];
    float           p[12];
    int             matlist[12];
    char          **matnames = NULL;
    int             materials[4];
    float           mix_vf[50];
    int             mix_next[50];
    int             mix_mat[50];
    int             mix_zone[50];
    int             current_mix;
    int             nzones, nnodes;
    int             zonelist[44];
    int             zonelist_length;
    int             shapesize[2];
    int             shapecount[2];
    DBoptlist      *optlist = NULL;

    int             i;
    char            buf[256];
    int             topo_dim;

    /* Create the mesh */
    coords[0] = x;
    coords[1] = y;

    nnodes = 13;
    nzones = 12;

    td = 2;
    /* Set up the coordinates */

    x[0] = 0.0;     y[0] = 2.0;
    x[1] = 2.0;     y[1] = 0.0;
    x[2] = 1.5;     y[2] = 1.5;
    x[3] = 2.5;     y[3] = 1.5;
    x[4] = 4.0;     y[4] = 2.0;
    x[5] = 2.0;     y[5] = 1.5;
    x[6] = 1.5;     y[6] = 2.0;
    x[7] = 2.0;     y[7] = 2.0;
    x[8] = 2.5;     y[8] = 2.0;
    x[9] = 1.5;     y[9] = 2.5;
    x[10] = 2.0;     y[10] = 2.5;
    x[11] = 2.5;     y[11] = 2.5;
    x[12] = 2.0;     y[12] = 4.0;

    /* Set up the zonelist */
    i=0;
    /* Zone 0 */
    zonelist[i++] = 0;
    zonelist[i++] = 1;
    zonelist[i++] = 2;

    /* Zone 1 */
    zonelist[i++] = 1;
    zonelist[i++] = 4;
    zonelist[i++] = 3;

    /* Zone 2 */
    zonelist[i++] = 0;
    zonelist[i++] = 9;
    zonelist[i++] = 12;

    /* Zone 3 */
    zonelist[i++] = 12;
    zonelist[i++] = 11;
    zonelist[i++] = 4;

    /* Zone 4 */
    zonelist[i++] = 0;
    zonelist[i++] = 2;
    zonelist[i++] = 6;
    zonelist[i++] = 9;

    /* Zone 5 */
    zonelist[i++] = 2;
    zonelist[i++] = 1;
    zonelist[i++] = 3;
    zonelist[i++] = 5;

    /* Zone 6 */
    zonelist[i++] = 3;
    zonelist[i++] = 4;
    zonelist[i++] = 11;
    zonelist[i++] = 8;

    /* Zone 7 */
    zonelist[i++] = 9;
    zonelist[i++] = 10;
    zonelist[i++] = 11;
    zonelist[i++] = 12;

    /* Zone 8 */
    zonelist[i++] = 2;
    zonelist[i++] = 5;
    zonelist[i++] = 7;
    zonelist[i++] = 6;

    /* Zone 9 */
    zonelist[i++] = 5;
    zonelist[i++] = 3;
    zonelist[i++] = 8;
    zonelist[i++] = 7;

    /* Zone 10 */
    zonelist[i++] = 6;
    zonelist[i++] = 7;
    zonelist[i++] = 10;
    zonelist[i++] = 9;

    /* Zone 11 */
    zonelist[i++] = 7;
    zonelist[i++] = 8;
    zonelist[i++] = 11;
    zonelist[i++] = 10;

    zonelist_length = i;

    shapesize[0] = 3;
    shapesize[1] = 4;

    shapecount[0] = 4;
    shapecount[1] = 8;

    /* Set up data variables */
    for(i=0;i<nnodes;i++)
    {
        double xl,yl;

        xl= x[i] - 2.0;
        yl= y[i] - 2.0;

        d[i] = sqrt(xl*xl+yl*yl);
        u[i] = xl;
        v[i] = yl;
    }
    for(i=0;i<nzones;i++)
    {
        if (i<shapecount[0])
        {
            int index;
            double xavg, yavg;

            index = (i*shapesize[0]);
            xavg = (x[zonelist[index]] + x[zonelist[index+1]] + x[zonelist[index+2]])/3;
            yavg = (y[zonelist[index]] + y[zonelist[index+1]] + y[zonelist[index+2]])/3;

            xavg -= 2.0;
            yavg -= 2.0;

            p[i] = sqrt(xavg*xavg+yavg*yavg);
        } else
        {
            int index;
            double xavg, yavg;

            index = shapecount[0]*shapesize[0]+(i-shapecount[0])*shapesize[1];

            xavg = (x[zonelist[index]] + x[zonelist[index+1]] +
                    x[zonelist[index+2]] + x[zonelist[index+3]])/4;
            yavg = (y[zonelist[index]] + y[zonelist[index+1]] +
                    y[zonelist[index+2]] + y[zonelist[index+3]])/4;

            xavg -= 2.0;
            yavg -= 2.0;

            p[i] = sqrt(xavg*xavg+yavg*yavg);
        }
    }

    /* Set up the materials */
    materials[0] = 1;
    materials[1] = 2;
    materials[2] = 3;
    materials[3] = 4;
    matnames = (char**)malloc(sizeof(char*)*4);
    matnames[0] = _db_safe_strdup("Bottom");
    matnames[1] = _db_safe_strdup("Right");
    matnames[2] = _db_safe_strdup("Top");
    matnames[3] = _db_safe_strdup("Left");

#define SET_MIX(mat1,mat2) \
    matlist[i] = -(current_mix+1); \
    mix_mat[current_mix] = mat1; \
    mix_vf[current_mix] = 0.5; \
    mix_zone[current_mix] = i; \
    mix_next[current_mix] = current_mix + 2; \
    current_mix++; \
    mix_mat[current_mix] = mat2; \
    mix_vf[current_mix] = 0.5; \
    mix_zone[current_mix] = i; \
    mix_next[current_mix] = 0; \
    current_mix++

    current_mix = 0;
    for(i=0;i<nzones;i++)
    {
        if (i==5) {
            matlist[i] = 1;
        } else if (i==4) {
            matlist[i] = 4;
        } else if (i==6) {
            matlist[i] = 2;
        } else if (i==7) {
            matlist[i] = 3;
        } else if ((i==0) || (i==8)) {
            SET_MIX(1,4);
        } else if ((i==1) || (i==9)) {
            SET_MIX(1,2);
        } else if ((i==3) || (i==11)) {
            SET_MIX(2,3);
        } else if ((i==2) || (i==10)) {
            SET_MIX(4,3);
        }
    }

    /* Write out the variables */
    cycle = 48;
    time = 4.8;
    dtime = 4.8;

    /* The length includes the terminating NULL character. */
    strcpy(buf, "mesh ucdmesh2d;pseudocolor d");
    i = strlen(buf);
    DBWrite(dbfile, "_meshtvinfo", buf, &i, 1, DB_CHAR);

    strcpy(buf,"vel vector {u,v};speed scalar sqrt(u*u+v*v)");
    i = strlen(buf);
    DBWrite(dbfile, "_meshtv_defvars", buf, &i, 1, DB_CHAR);

    optlist = DBMakeOptlist(12);
    DBAddOption(optlist, DBOPT_CYCLE, &cycle);
    DBAddOption(optlist, DBOPT_TIME, &time);
    DBAddOption(optlist, DBOPT_DTIME, &dtime);
    DBAddOption(optlist, DBOPT_TOPO_DIM, (void *)&td);
    DBAddOption(optlist, DBOPT_XLABEL, (char *) "X Axis");
    DBAddOption(optlist, DBOPT_YLABEL, (char *) "Y Axis");
    DBAddOption(optlist, DBOPT_ZLABEL, (char *) "Z Axis");
    DBAddOption(optlist, DBOPT_XUNITS, (char *) "cm");
    DBAddOption(optlist, DBOPT_YUNITS, (char *) "cm");
    DBAddOption(optlist, DBOPT_ZUNITS, (char *) "cm");
    DBAddOption(optlist, DBOPT_MATNAMES, matnames);
    topo_dim = 2;
    DBAddOption(optlist, DBOPT_TOPO_DIM, &topo_dim);

    /* Write out the mesh */
    DBSetDeprecateWarnings(0);
    DBPutZonelist(dbfile, "ucd2d_zonelist", nzones, 2, zonelist,
                  zonelist_length, 0, shapesize, shapecount, 2);
    DBSetDeprecateWarnings(3);

    DBPutUcdmesh(dbfile, "ucdmesh2d", 2, NULL, coords, nnodes, nzones,
                 "ucd2d_zonelist", NULL, DB_FLOAT, optlist);

    /* Write out the material */
    dims[0] = nzones;
    DBPutMaterial(dbfile, "mat1", "ucdmesh2d", 4, materials, matlist, dims, 1,
                  mix_next, mix_mat, mix_zone, mix_vf, current_mix, DB_FLOAT,
                  optlist);

    /* Write out the variables */
    DBPutUcdvar1(dbfile, "d", "ucdmesh2d", d, nnodes, NULL, 0, DB_FLOAT,
                 DB_NODECENT, NULL);
    DBPutUcdvar1(dbfile, "u", "ucdmesh2d", u, nnodes, NULL, 0, DB_FLOAT,
                 DB_NODECENT, NULL);
    DBPutUcdvar1(dbfile, "v", "ucdmesh2d", v, nnodes, NULL, 0, DB_FLOAT,
                 DB_NODECENT, NULL);
    DBPutUcdvar1(dbfile, "p", "ucdmesh2d", p, nzones, NULL, 0, DB_FLOAT,
                 DB_ZONECENT, NULL);

    /* Free memory */
    DBFreeOptlist(optlist);
    for(i=0;i<4;i++)
        FREE(matnames[i]);
    FREE(matnames);
}

/*-------------------------------------------------------------------------
 * Function:    build_rect3d
 *
 * Purpose:     Build a 3-d rectilinear mesh and add it to the open database.
 *
 * Return:      Success:
 *
 *              Failure:
 *
 * Programmer:
 *
 * Modifications:
 *     Robb Matzke, Sun Dec 18 17:39:58 EST 1994
 *     Fixed memory leak.
 *
 *     Sean Ahern, Thu Jun 20 08:53:11 PDT 1996
 *     Changed the mesh name to be more mnemonic.
 *
 *     Eric Brugger, Tue Nov 26 11:42:47 PST 1996
 *     I added the outputing of some meshtv defvars to the silo file.
 *
 *     Jim Reus, Wed Nov 13 07:24:24 PST 1996
 *     Added temperature (t) variable to demonstrate logplots.
 *     ...temperature is positive non-zero, constant in x, linear
 *        in y and exponential in z.
 *
 *     Eric Brugger, Fri Dec 13 16:51:36 PST 1996
 *     I modified the routine to not use NX, NY, and NZ.
 *
 *     Eric Brugger, Wed Dec 18 14:50:48 PST 1996
 *     Changed the `p' variable so that it is one smaller than the
 *     mesh, since meshtv does not currently handle this case.
 *
 *     Sean Ahern, Wed Jan 22 15:16:58 PST 1997
 *     Got rid of unused variables and variables that were being set but not
 *     used.
 *
 *     Sean Ahern, Thu Feb  7 14:02:36 PST 2002
 *     Added material names.
 *
 *-------------------------------------------------------------------------*/
/*ARGSUSED*/
static void
build_rect3d(DBfile * dbfile, int size, int order)
{
    int            cycle;
    float          time;
    double         dtime;
    int            nx, ny, nz;
    int            ndims;
    int            dims[3], zdims[3];
    float         *coords[3];
    float         *x, *y, *z;

    char          *meshname, *var1name, *var2name, *var3name, *var4name;
    char          *var5name, *matname;
    float         *d, *p;
    float         *u, *v, *w;
    float         *t;

    int            nmats;
    int            matnos[9];
    int           *matlist;
    int            dims2[3];
    int            mixlen;
    int           *mix_next, *mix_mat, *mix_zone;
    float         *mix_vf;

    DBoptlist     *optlist;
    char         **matnames = NULL;

    int            i, j, k;
    double         xave, yave, zave;
    double         xcenter, ycenter, zcenter;
    double         dist;

    int            binf;
    ssize_t        wrsz;

    switch (size) {
        case 1:
            nx = 30;
            ny = 40;
            nz = 30;
            break;
        case 2:
            nx = 60;
            ny = 80;
            nz = 60;
            break;
        case 3:
            nx = 120;
            ny = 160;
            nz = 120;
            break;
    }

    x = ALLOC_N (float, nx + 1);
    y = ALLOC_N (float, ny + 1);
    z = ALLOC_N (float, nz + 1);
    d = ALLOC_N (float, nx * ny * nz);
    p = ALLOC_N (float, nx * ny * nz);
    u = ALLOC_N (float, (nx + 1) * (ny + 1) * (nz + 1));
    v = ALLOC_N (float, (nx + 1) * (ny + 1) * (nz + 1));
    w = ALLOC_N (float, (nx + 1) * (ny + 1) * (nz + 1));
    matlist = ALLOC_N (int, nx * ny * nz);
    mix_next = ALLOC_N (int, 50 * nx * ny);
    mix_mat  = ALLOC_N (int, 50 * nx * ny);
    mix_zone = ALLOC_N (int, 50 * nx * ny);
    mix_vf   = ALLOC_N (float, 50 * nx * ny);

    /*
     * Create the mesh.
     */
    meshname = "quadmesh3d";
    coords[0] = x;
    coords[1] = y;
    coords[2] = z;
    ndims = 3;
    dims[0] = nx + 1;
    dims[1] = ny + 1;
    dims[2] = nz + 1;
    for (i = 0; i < nx + 1; i++)
        x[i] = i * (1. / nx);
    for (i = 0; i < ny + 1; i++)
        y[i] = i * (1. / ny);
    for (i = 0; i < nz + 1; i++)
        z[i] = i * (1. / nz);

    /*
     * Create the density and pressure arrays.
     */
    var1name = "d";
    var2name = "p";
    xcenter = .5;
    ycenter = .5;
    zcenter = .5;
    zdims[0] = nx;
    zdims[1] = ny;
    zdims[2] = nz;
    for (i = 0; i < nx; i++)
    {
        for (j = 0; j < ny; j++)
        {
            for (k = 0; k < nz; k++)
            {
                xave = (x[i]+x[i+1]) / 2.0 ;
                yave = (y[j]+y[j+1]) / 2.0 ;
                zave = (z[k]+z[k+1]) / 2.0 ;

                dist = sqrt((xave - xcenter) * (xave - xcenter) +
                            (yave - ycenter) * (yave - ycenter) +
                            (zave - zcenter) * (zave - zcenter));

                d[k*nx*ny + j*nx + i] = dist;

                p[k*nx*ny + j*nx + i] = 1 / (dist + 0.0001);
            }
        }
    }

    /*
     * Create the velocity component arrays.
     */
    var3name = "u";
    var4name = "v";
    var5name = "w";
    xcenter = .5001;
    ycenter = .5001;
    zcenter = .5001;
    for (i = 0; i < nx + 1; i++) {
        for (j = 0; j < ny + 1; j++) {
            for (k = 0; k < nz + 1; k++) {
                dist = sqrt((x[i] - xcenter) * (x[i] - xcenter) +
                            (y[j] - ycenter) * (y[j] - ycenter) +
                            (z[k] - zcenter) * (z[k] - zcenter));
                u[k * (nx + 1) * (ny + 1) + j * (nx + 1) + i] = (x[i] - xcenter) / dist;
                v[k * (nx + 1) * (ny + 1) + j * (nx + 1) + i] = (y[j] - ycenter) / dist;
                w[k * (nx + 1) * (ny + 1) + j * (nx + 1) + i] = (z[k] - zcenter) / dist;
            }
        }
    }


    /*
     * Create the temperature array.
     */
    if ((t=ALLOC_N(float,(nx+1)*(ny+1)*(nz+1))) != NULL)
    {
       double a,b,c,d;

       a = 1.;
       b = 3.;
       c = 5.;
       d = 7.;
       for (i=0; i<nx+1; i++)
          for (j=0; j<ny+1; j++)
          {
             double y;

             y = j / (float) ny;
             for (k=0; k<nz+1; k++)
             {
                double z;

                z = k / (float) nz;
                t[k*(nx+1)*(ny+1) + j*(nx+1) + i] = a + b*y + c*exp(d*z);
             }
          }
    }

    /*
     * Create the material array.
     */
    matname = "mat1";
    nmats = 8;
    matnos[0] = 1;
    matnos[1] = 2;
    matnos[2] = 3;
    matnos[3] = 4;
    matnos[4] = 5;
    matnos[5] = 6;
    matnos[6] = 7;
    matnos[7] = 8;
    dims2[0] = nx;
    dims2[1] = ny;
    dims2[2] = nz;
    mixlen = 0;
    matnames = (char**)malloc(sizeof(char*)*nmats);
    matnames[0] = _db_safe_strdup("Brocolli/Cheddar soup");
    matnames[1] = _db_safe_strdup("Beef");
    matnames[2] = _db_safe_strdup("Gray smoke");
    matnames[3] = _db_safe_strdup("Wax");
    matnames[4] = _db_safe_strdup("Soapstone");
    matnames[5] = _db_safe_strdup("Bubble gum");
    matnames[6] = _db_safe_strdup("Platinum");
    matnames[7] = _db_safe_strdup("Nitrozanium");

    /*
     * Put in material 1 as the background then overlay materials
     * 2 through 9.
     */
    fill_rect3d_bkgr (matlist, nx, ny, nz, 1);

    fill_rect3d_mat (x, y, z, matlist, nx, ny, nz, mix_next, mix_mat, mix_zone,
                     mix_vf, &mixlen, 2, 0.7);
    fill_rect3d_mat (x, y, z, matlist, nx, ny, nz, mix_next, mix_mat, mix_zone,
                     mix_vf, &mixlen, 3, 0.6);
    fill_rect3d_mat (x, y, z, matlist, nx, ny, nz, mix_next, mix_mat, mix_zone,
                     mix_vf, &mixlen, 4, 0.5);
    fill_rect3d_mat (x, y, z, matlist, nx, ny, nz, mix_next, mix_mat, mix_zone,
                     mix_vf, &mixlen, 5, 0.4);
    fill_rect3d_mat (x, y, z, matlist, nx, ny, nz, mix_next, mix_mat, mix_zone,
                     mix_vf, &mixlen, 6, 0.3);
    fill_rect3d_mat (x, y, z, matlist, nx, ny, nz, mix_next, mix_mat, mix_zone,
                     mix_vf, &mixlen, 7, 0.2);
    fill_rect3d_mat (x, y, z, matlist, nx, ny, nz, mix_next, mix_mat, mix_zone,
                     mix_vf, &mixlen, 8, 0.1);

    if (mixlen > 50 * nx * ny) {
        printf ("mixlen = %d\n", mixlen);
        exit(EXIT_SUCCESS);
    }

    /*
     * Write out the variables.
     */
    cycle = 48;
    time = 4.8;
    dtime = 4.8;

    /*
     * The length includes the terminating NULL character.
     */
    i = 30;
    DBWrite (dbfile, "_meshtvinfo", "mesh quadmesh3d;pseudocolor d", &i, 1, DB_CHAR);
    i = 50;
    DBWrite (dbfile, "_meshtv_defvars",
             "vel vector {u,v,w};speed scalar sqrt(u*u+v*v+w*w)",
             &i, 1, DB_CHAR);

    optlist = DBMakeOptlist(11);
    DBAddOption(optlist, DBOPT_CYCLE, &cycle);
    DBAddOption(optlist, DBOPT_TIME, &time);
    DBAddOption(optlist, DBOPT_DTIME, &dtime);
    DBAddOption(optlist, DBOPT_XLABEL, (char *) "X Axis");
    DBAddOption(optlist, DBOPT_YLABEL, (char *) "Y Axis");
    DBAddOption(optlist, DBOPT_ZLABEL, (char *) "Z Axis");
    DBAddOption(optlist, DBOPT_XUNITS, (char *) "cm");
    DBAddOption(optlist, DBOPT_YUNITS, (char *) "cm");
    DBAddOption(optlist, DBOPT_ZUNITS, (char *) "cm");
    DBAddOption(optlist, DBOPT_MATNAMES, matnames);
#if 0
    i = DB_COLMAJOR;
    DBAddOption(optlist, DBOPT_MAJORORDER, &i);
#endif

    DBPutQuadmesh(dbfile, meshname, NULL, coords, dims, ndims, DB_FLOAT,
                  DB_COLLINEAR, optlist);
#ifndef _WIN32
    binf = open("rect3dz.bin", O_CREAT|O_TRUNC|O_WRONLY, S_IRUSR|S_IWUSR);
#else
    binf = open("rect3dz.bin", O_CREAT|O_TRUNC|O_WRONLY, S_IREAD|S_IWRITE);
#endif


    DBPutQuadvar1(dbfile, var1name, meshname, d, zdims, ndims, NULL, 0,
                  DB_FLOAT, DB_ZONECENT, optlist);
    wrsz = write(binf, d, zdims[0]*zdims[1]*zdims[2]*sizeof(float));
    assert(wrsz==zdims[0]*zdims[1]*zdims[2]*sizeof(float));

    DBPutQuadvar1(dbfile, var2name, meshname, p, zdims, ndims, NULL, 0,
                  DB_FLOAT, DB_ZONECENT, optlist);
    wrsz = write(binf, p, zdims[0]*zdims[1]*zdims[2]*sizeof(float));
    assert(wrsz==zdims[0]*zdims[1]*zdims[2]*sizeof(float));
    close(binf);
    printf("zsize = nz=%d, ny=%d, nx=%d\n", zdims[2], zdims[1], zdims[0]);

#ifndef _WIN32
    binf = open("rect3dn.bin", O_CREAT|O_TRUNC|O_WRONLY, S_IRUSR|S_IWUSR);
#else
    binf = open("rect3dn.bin", O_CREAT|O_TRUNC|O_WRONLY, S_IREAD|S_IWRITE);
#endif

    DBPutQuadvar1(dbfile, var3name, meshname, u, dims, ndims, NULL, 0,
                  DB_FLOAT, DB_NODECENT, optlist);
    wrsz = write(binf, u, dims[0]*dims[1]*dims[2]*sizeof(float));
    assert(wrsz==dims[0]*dims[1]*dims[2]*sizeof(float));

    DBPutQuadvar1(dbfile, var4name, meshname, v, dims, ndims, NULL, 0,
                  DB_FLOAT, DB_NODECENT, optlist);
    wrsz = write(binf, v, dims[0]*dims[1]*dims[2]*sizeof(float));
    assert(wrsz==dims[0]*dims[1]*dims[2]*sizeof(float));

    DBPutQuadvar1(dbfile, var5name, meshname, w, dims, ndims, NULL, 0,
                  DB_FLOAT, DB_NODECENT, optlist);
    wrsz = write(binf, w, dims[0]*dims[1]*dims[2]*sizeof(float));
    assert(wrsz==dims[0]*dims[1]*dims[2]*sizeof(float));
    close(binf);
    printf("size = nz=%d, ny=%d, nx=%d\n", dims[2], dims[1], dims[0]);

    if (t != NULL)
       DBPutQuadvar1(dbfile, "t", meshname, t
                    ,dims, ndims, NULL, 0, DB_FLOAT, DB_NODECENT
                    ,optlist
                    );

    DBPutMaterial(dbfile, matname, meshname, nmats, matnos, matlist, dims2,
                  ndims, mix_next, mix_mat, mix_zone, mix_vf, mixlen, DB_FLOAT,
                  optlist);

    DBFreeOptlist(optlist);

    /*
     * Free the temporary storage.
     */
    FREE (x);
    FREE (y);
    FREE (z);
    FREE (d);
    FREE (p);
    FREE (u);
    FREE (v);
    FREE (w);
    FREE (t);
    FREE (matlist);
    FREE (mix_next);
    FREE (mix_mat);
    FREE (mix_zone);
    FREE (mix_vf);
    for(i=0;i<nmats;i++)
        FREE(matnames[i]);
    FREE(matnames);
}

/*-------------------------------------------------------------------------
 * Function:    build_curv3d
 *
 * Purpose:     Build a 3-d Curvillinear  mesh and add it to the open
 *              database.
 *
 * Return:      Success:        void
 *
 *              Failure:
 *
 * Programmer:  Tony L. Jones
 *              May 30, 1995
 *
 * Modifications:
 *     Tony Jones      June 15, 1995
 *     Density and Pressure calculation was in err.  Previous
 *     algorithm was passing non-existent values to the mentioned
 *     arrays.  Problem fixed by decrementing the max loop index.
 *
 *     Sean Ahern, Thu Jun 20 08:53:11 PDT 1996
 *     Changed the mesh name to be more mnemonic.
 *
 *     Eric Brugger, Tue Nov 26 11:42:47 PST 1996
 *     I added the outputing of some meshtv defvars to the silo file.
 *
 *
 *     Jim Reus, Wed Nov 13 07:24:24 PST 1996
 *     Added temperature (t) variable to demonstrate logplots.
 *     ...temperature is positive non-zero, constant in x, linear
 *        in y and exponential in z.
 *
 *     Sean Ahern, Tue Jan  7 15:30:43 PST 1997
 *     Removed dependence on NX, NY, and NZ.
 *
 *     Sean Ahern, Wed Jan 22 15:16:58 PST 1997
 *     Initialized some pointer variables to NULL.  Got rid of unused
 *     variables and variables that were being set but not used.
 *
 *     Eric Brugger, Mon Feb 22 14:21:13 PST 1999
 *     Added code to force the y coordinates to be exactly zero on the
 *     y = 0 symmetry plane.
 *
 *     Jeremy Meredith, Thu Sep  2 11:58:13 PDT 1999
 *     Reversed the angular coordinates so they conform to the right
 *     hand rule.
 *
 *-------------------------------------------------------------------------
 */

/*ARGSUSED*/
static void
build_curv3d(DBfile * dbfile, int size, int order)
{
    int       cycle;
    float     time;
    double    dtime;
    float    *coords[3];

    float     *x = NULL;
    float     *y = NULL;
    float     *z = NULL;
    double    Xmin,Ymin,Zmin;
    double    Xmax,Ymax,Zmax;

    int       ndims, zdims[3];
    int       dims[3], dims2[3];

    char     *meshname, *var1name, *var2name, *var3name, *var4name;
    char     *var5name, *matname;

    float    *d = NULL, *p = NULL;
    float    *u = NULL;
    float    *v = NULL;
    float    *w = NULL;
    float    *t = NULL;

    int       first;
    int       nmats;
    int       matnos[3];
    int      *matlist = NULL;
    int       mixlen;
    int      *mix_next = NULL, *mix_mat = NULL;
    int      *mix_zone = NULL;
    float    *mix_vf = NULL;

    DBoptlist *optlist = NULL;
    char    **matnames = NULL;

    int       i, j, k, l;

    double    xave, yave;
    double    xcenter, ycenter;
    int       nx, ny, nz;

    double    theta, dtheta;
    double    r, dr;
    double    h, dh;
    double    dist;

    switch (size) {
        case 1:
            nx = 30;
            ny = 40;
            nz = 30;
            break;
        case 2:
            nx = 60;
            ny = 80;
            nz = 60;
            break;
        case 3:
            nx = 120;
            ny = 160;
            nz = 120;
            break;
    }

    x = ALLOC_N(float,(nx+1)*(ny+1)*(nz+1));
    y = ALLOC_N(float,(nx+1)*(ny+1)*(nz+1));
    z = ALLOC_N(float,(nx+1)*(ny+1)*(nz+1));
    d = ALLOC_N(float,nx*ny*nz);
    p = ALLOC_N(float,nx*ny*nz);
    u = ALLOC_N(float,(nx+1)*(ny+1)*(nz+1));
    v = ALLOC_N(float,(nx+1)*(ny+1)*(nz+1));
    w = ALLOC_N(float,(nx+1)*(ny+1)*(nz+1));
    matlist = ALLOC_N(int,nx*ny*nz);
    mix_next = ALLOC_N(int,50 * nx*ny*nz);
    mix_mat = ALLOC_N(int,50 * nx*ny*nz);
    mix_vf = ALLOC_N(float,50 * nx*ny*nz);

    /*
     * Create the mesh.
     */
    meshname = "curvmesh3d";
    coords[0] = x;
    coords[1] = y;
    coords[2] = z;

    ndims = 3;
    dims[0] = nx+1;
    dims[1] = ny+1;
    dims[2] = nz+1;

    dtheta = -(180. / nx) * (3.1415926536 / 180.);
    dh = 1;
    dr = 3. / ny;
    theta = 3.1415926536;

    first = 1;
    for (i = 0; i < nx+1; i++) {
        r = 2.;
        for (j = 0; j < ny+1; j++) {
            h = 0.;
            for (k = 0; k < nz+1; k++)
            {
               double Xp,Yp,Zp;
               int    a;

               Xp = r * cos(theta);
               Yp = r * sin(theta);
               Zp = h;
               if (first)
               {
                  Xmin  = Xp;
                  Xmax  = Xp;
                  Ymin  = Yp;
                  Ymax  = Yp;
                  Zmin  = Zp;
                  Zmax  = Zp;
                  first = 0;
               }
               else
               {
                  Xmin  = Min(Xmin,Xp);
                  Xmax  = Max(Xmax,Xp);
                  Ymin  = Min(Ymin,Yp);
                  Ymax  = Max(Ymax,Yp);
                  Zmin  = Min(Zmin,Zp);
                  Zmax  = Max(Zmax,Zp);
               }
               a        = k*(nx+1)*(ny+1) + j*(nx+1) + i;
               x[a]     = Xp;
               y[a]     = Yp;
               z[a]     = Zp;
               h       += dh;
            }
            r += dr;
        }
        theta += dtheta;
    }

    /*
     * Force the y values to be greater than or equal to zero so that
     * the y values are all planar on the y = 0 symmetry boundary.
     */
    for (i = 0; i < nx+1; i += nx)
    {
        for (j = 0; j < ny+1; j++)
        {
            for (k = 0; k < nz+1; k++)
            {
                l = k*(nx+1)*(ny+1) + j*(nx+1) + i;
                y[l] = 0.;
            }
        }
    }

    /*
     * Create the density and pressure arrays.
     */
    var1name = "d";
    var2name = "p";
    xcenter = 0.;
    ycenter = 0.;
    zdims[0] = nx;
    zdims[1] = ny;
    zdims[2] = nz;

    for (i = 0; i < nx; i++) {
        for (j = 0; j < ny; j++) {
            for (k = 0; k < nz; k++){
                xave = (x[k*(nx+1)*(ny+1) + j*(nx+1)+i]       +
                        x[k*(nx+1)*(ny+1) + j*(nx+1)+i+1]     +
                        x[k*(nx+1)*(ny+1) + (j+1)*(nx+1)+i+1] +
                        x[k*(nx+1)*(ny+1) + (j+1)*(nx+1)+i])  / 4.;

                yave = (y[k*(nx+1)*(ny+1) + j*(nx+1)+i]       +
                        y[k*(nx+1)*(ny+1) + j*(nx+1)+i+1]     +
                        y[k*(nx+1)*(ny+1) + (j+1)*(nx+1)+i+1] +
                        y[k*(nx+1)*(ny+1) + (j+1)*(nx+1)+i])  / 4.;

                 dist = sqrt ((xave - xcenter) * (xave - xcenter) +
                              (yave - ycenter) * (yave - ycenter) );
                d[k*(nx)*(ny) + j*(nx)+i] = dist;
                p[k*(nx)*(ny) + j*(nx)+i] = 1. / (dist + .0001);
            }
        }
    }

    /*
     * Create the velocity component arrays.
     */
    var3name = "u";
    var4name = "v";
    var5name = "w";
    xcenter = 0.;
    ycenter = 0.;

    for (i = 0; i < nx+1; i++) {
         for (j = 0; j < ny+1; j++) {
             for (k = 0; k < nz+1; k++){
                 dist = sqrt ((x[i] - xcenter) * (x[i] - xcenter) +
                              (y[j] - ycenter) * (y[j] - ycenter));
                 u[k*(nx+1)*(ny+1)+j*(nx+1)+i] = (x[i] - xcenter) / dist;
                 v[k*(nx+1)*(ny+1)+j*(nx+1)+i] = (y[j] - ycenter) / dist;
                 w[k*(nx+1)*(ny+1)+j*(nx+1)+i] = 0.;
             }
         }
    }

    /*
     * Create the temperature array.
     */
    if ((t=ALLOC_N(float,(nx+1)*(ny+1)*(nz+1))) != NULL)
    {
       double Ca,Cb,Cc,Cd;

       Ca = 1.;
       Cb = 3.;
       Cc = 5.;
       Cd = 7.;
       for (i=0; i<=nx; ++i)
          for (j=0; j<=ny; ++j)
             for (k=0; k<=nz; ++k)
             {
                double Yv,Zv;
                int    a;

                a    = k*(nx+1)*(ny+1) + j*(nx+1) + i;
                Yv   = (y[a]-Ymin) / (Ymax-Ymin);
                Zv   = (z[a]-Zmin) / (Zmax-Zmin);
                t[a] = Ca + Cb*Yv + Cc*exp(Cd*Zv);
             }
    }

    /*
     * Create the material array.
     */
    matname = "mat1";
    nmats = 3;
    matnos[0] = 1;
    matnos[1] = 2;
    matnos[2] = 3;
    dims2[0] = nx;
    dims2[1] = ny;
    dims2[2] = nz;
    matnames = (char**)malloc(sizeof(char*)*nmats);
    matnames[0] = _db_safe_strdup("Greed");
    matnames[1] = _db_safe_strdup("Charity");
    matnames[2] = _db_safe_strdup("Apathy");

    mixlen = 0;

    /*
     * Put in the material in 3 shells.
     */
    for (i = 0; i < nx; i++) {
        for (k = 0; k < nz; k++){
            for (j = 0; j < 10; j++){
                matlist[k*nx*ny+j*nx+i] = 1;
            }
            for (j = 10; j < 20; j++) {
                matlist[k*nx*ny+j*nx+i] = 2;
            }
            for (j = 20; j < ny; j++) {
                matlist[k*nx*ny+j*nx+i] = 3;
            }
        }
    }

    /*
     * Write out the variables.
     */
    cycle = 48;
    time  = 4.8;
    dtime = 4.8;

    /*
     * The length includes the terminating NULL character.
     */
    i = 30;
    DBWrite (dbfile, "_meshtvinfo", "mesh curvmesh3d;pseudocolor d",
             &i, 1, DB_CHAR);
    i = 50;
    DBWrite (dbfile, "_meshtv_defvars",
             "vel vector {u,v,w};speed scalar sqrt(u*u+v*v+w*w)",
             &i, 1, DB_CHAR);

    optlist = DBMakeOptlist (11);
    DBAddOption (optlist, DBOPT_CYCLE, &cycle);
    DBAddOption (optlist, DBOPT_TIME , &time);
    DBAddOption (optlist, DBOPT_DTIME, &dtime);
    DBAddOption (optlist, DBOPT_XLABEL, (char *) "X Axis");
    DBAddOption (optlist, DBOPT_YLABEL, (char *) "Y Axis");
    DBAddOption (optlist, DBOPT_ZLABEL, (char *) "Z Axis");
    DBAddOption (optlist, DBOPT_XUNITS, (char *) "cm");
    DBAddOption (optlist, DBOPT_YUNITS, (char *) "cm");
    DBAddOption (optlist, DBOPT_ZUNITS, (char *) "cm");
    DBAddOption (optlist, DBOPT_MATNAMES, matnames);

#if 0
    i = DB_COLMAJOR;
    DBAddOption (optlist, DBOPT_MAJORORDER, &i);
#endif

    DBPutQuadmesh (dbfile, meshname, NULL, coords, dims, ndims,
                   DB_FLOAT, DB_NONCOLLINEAR, optlist);

    DBPutQuadvar1 (dbfile, var1name, meshname, d, zdims, ndims, NULL, 0,
                   DB_FLOAT, DB_ZONECENT, optlist);

    DBPutQuadvar1 (dbfile, var2name, meshname, p, zdims, ndims, NULL, 0,
                   DB_FLOAT, DB_ZONECENT, optlist);

    DBPutQuadvar1 (dbfile, var3name, meshname, u, dims, ndims, NULL, 0,
                   DB_FLOAT, DB_NODECENT, optlist);

    DBPutQuadvar1 (dbfile, var4name, meshname, v, dims, ndims, NULL, 0,
                   DB_FLOAT, DB_NODECENT, optlist);

    DBPutQuadvar1 (dbfile, var5name, meshname, w, dims, ndims, NULL, 0,
                   DB_FLOAT, DB_NODECENT, optlist);

    DBPutQuadvar1 (dbfile, "t", meshname, t, dims, ndims, NULL, 0, DB_FLOAT,
                   DB_NODECENT, optlist);

    DBPutMaterial (dbfile, matname, meshname, nmats, matnos, matlist, dims2,
                   ndims, mix_next, mix_mat, mix_zone, mix_vf, mixlen,
                   DB_FLOAT, optlist);

    DBFreeOptlist (optlist);

    /* Free the temporary storage.  */
    FREE(x);
    FREE(y);
    FREE(z);
    FREE(d);
    FREE(p);
    FREE(u);
    FREE(v);
    FREE(w);
    FREE(t);
    FREE(matlist);
    FREE(mix_mat);
    FREE(mix_next);
    FREE(mix_zone);
    FREE(mix_vf);
    for(i=0;i<nmats;i++)
        FREE(matnames[i]);
    FREE(matnames);
}

/*-------------------------------------------------------------------------
 * Function:    build_ucd3d
 *
 * Purpose:     Build a 3-d UCD mesh and add it to the open database.
 *
 * Return:      Success:        void
 *
 *              Failure:
 *
 * Programmer:
 *
 * Modifications:
 *
 *     Mark Miller, Mon Oct  5 21:39:38 PDT 1998
 *     Fixed node numbering to match pictures in Silo's user manual
 *
 *     Robb Matzke, Sun Dec 18 17:40:58 EST 1994
 *     Fixed memory leak.
 *
 *     Sean Ahern, Thu Jun 20 08:53:11 PDT 1996
 *     Changed the mesh name to be more mnemonic.
 *
 *     Eric Brugger, Tue Nov 26 11:42:47 PST 1996
 *     I added the outputing of some meshtv defvars to the silo file.
 *
 *     Jim Reus, Wed Nov 13 07:24:24 PST 1996
 *     Added temperature (t) variable to demonstrate logplots.
 *     ...temperature is positive non-zero, constant in x, linear
 *        in y and exponential in z.
 *
 *     Eric Brugger, Wed Dec 18 16:41:58 PST 1996
 *     I corrected a bug with the initialization of the variable first.
 *     It was changed from an initial value of 0 to 1.
 *
 *     Sean Ahern, Wed Jan 22 15:16:58 PST 1997
 *     Got rid of unused variables and variables that were being set but not
 *     used.
 *
 *     Eric Brugger, Mon Feb 22 14:21:13 PST 1999
 *     Added code to force the y coordinates to be exactly zero on the
 *     y = 0 symmetry plane.
 *
 *     Jeremy Meredith, Wed Apr 14 16:22:15 PDT 1999
 *     Added mixed-material zones, and species, to this test case.
 *
 *     Lisa J. Roberts, Wed Jun 30 15:21:28 PDT 1999
 *     Modified the DBCalcExternalFacelist() call to select the proper
 *     bound method (due to the mixed-material change).
 *
 *     Eric Brugger, Mon Sep 20 19:10:11 PDT 1999
 *     I modified the material numbers so that they are no longer
 *     numbered consecutively.
 *
 *     Brad Whitlock, Wed Jan 18 16:21:11 PST 2006
 *     I added a nodal "ascii" variable to the mesh.
 *
 *-------------------------------------------------------------------------
 */

/*ARGSUSED*/
static void
build_ucd3d(DBfile * dbfile, int size, int order)
{
    int            cycle;
    float          time;
    double         dtime;
    int            td;
    float         *coords[3];
    float          x[2646], y[2646], z[2646];
    double         Xmin,Ymin,Zmin;
    double         Xmax,Ymax,Zmax;
    int            first;
    int            nfaces, nzones, nnodes;
    int            lfacelist, lzonelist;
    int            fshapesize, fshapecnt, zshapesize, zshapecnt;
    int            zonelist[16000];
    int            facelist[11200];
    int            zoneno[2800];

    char          *meshname, *var1name, *var2name, *var3name, *var4name;
    char          *var5name, *var6name, *matname;
    float         *vars[1];
    char          *varnames[1];
    float          d[2646], p[2646], u[2646], v[2646], w[2646];
    char           ascii[2646];
    float         *t;
    
    int            nmats;
    int            matnos[3];
    int            matlist[2000];
    int            mixlen;
    int            mix_next[900], mix_mat[900], mix_zone[900];
    float          mix_vf[900];

    int            nmatspec[3];
    float          specmf[5000];
    int            speclist[2000];
    int            mix_speclist[900];
    int            nspecmf;

    DBoptlist     *optlist;
    char         **matnames = NULL;

    DBfacelist    *fl;

    int            i, j, k, l;
    int            iz, onvalue = 1;
    double         xcenter, ycenter;
    double         theta, dtheta;
    double         r, dr;
    double         h, dh;
    double         dist;

    /*
     * Create the mesh.
     */
    meshname = "ucdmesh3d";
    coords[0] = x;
    coords[1] = y;
    coords[2] = z;

    nnodes = 21 * 21 * 6;
    nzones = 20 * 20 * 5;

    lzonelist = (20 * 20 * 5) * 8;
    zshapecnt = nzones;
    zshapesize = 8;

    td = 3;

    dh = 20. / 20.;
    dtheta = (180. / 20.) * (3.1415926 / 180.);
    dr = 3. / 5.;
    h = 0.;
    first = 1;
    for (i = 0; i < 21; i++) {
        theta = 0.;
        for (j = 0; j < 21; j++) {
            r = 2.;
            for (k = 0; k < 6; k++)
            {
               double Xv,Yv,Zv;
               int    a;

               Xv = r * cos(theta);
               Yv = r * sin(theta);
               Zv = h;
               if (first)
               {
                  Xmin  = Xv;
                  Xmax  = Xv;
                  Ymin  = Yv;
                  Ymax  = Yv;
                  Zmin  = Zv;
                  Zmax  = Zv;
                  first = 0;
               }
               else
               {
                  Xmin  = Min(Xmin,Xv);
                  Xmax  = Max(Xmax,Xv);
                  Ymin  = Min(Ymin,Yv);
                  Ymax  = Max(Ymax,Yv);
                  Zmin  = Min(Zmin,Zv);
                  Zmax  = Max(Zmax,Zv);
               }
               a        = i*126 + j*6 + k;
               x[a]     = Xv;
               y[a]     = Yv;
               z[a]     = Zv;
               r       += dr;
            }
            theta += dtheta;
        }
        h += dh;
    }

    /*
     * Force the y values to be greater than or equal to zero so that
     * the y values are all planar on the y = 0 symmetry boundary.
     */
    for (i = 0; i < 21; i++)
    {
        for (j = 0; j < 21; j += 20)
        {
            for (k = 0; k < 6; k++)
            {
                l = i*126 + j*6 + k;
                y[l] = 0.;
            }
        }
    }

    iz = 0;
    for (i = 0; i < 20; i++) {
        for (j = 0; j < 20; j++) {
            for (k = 0; k < 5; k++) {
                zonelist[iz    ] = (i + 0) * 126 + (j + 1) * 6 + k + 1;
                zonelist[iz + 1] = (i + 0) * 126 + (j + 0) * 6 + k + 1;
                zonelist[iz + 2] = (i + 1) * 126 + (j + 0) * 6 + k + 1;
                zonelist[iz + 3] = (i + 1) * 126 + (j + 1) * 6 + k + 1;
                zonelist[iz + 4] = (i + 0) * 126 + (j + 1) * 6 + k + 0;
                zonelist[iz + 5] = (i + 0) * 126 + (j + 0) * 6 + k + 0;
                zonelist[iz + 6] = (i + 1) * 126 + (j + 0) * 6 + k + 0;
                zonelist[iz + 7] = (i + 1) * 126 + (j + 1) * 6 + k + 0;
                iz += 8;
            }
        }
    }

    /*
     * Create the density and pressure arrays.
     */
    var1name = "d";
    var2name = "p";
    xcenter = 0.;
    ycenter = 0.;
    for (i = 0; i < 21; i++) {
        for (j = 0; j < 21; j++) {
            for (k = 0; k < 6; k++) {
                dist = sqrt((x[i * 126 + j * 6 + k] - xcenter) *
                            (x[i * 126 + j * 6 + k] - xcenter) +
                            (y[i * 126 + j * 6 + k] - ycenter) *
                            (y[i * 126 + j * 6 + k] - ycenter));
                d[i * 126 + j * 6 + k] = dist;
                p[i * 126 + j * 6 + k] = 1. / (dist + .0001);
            }
        }
    }

    /*
     * Create the velocity component arrays.
     */
    var3name = "u";
    var4name = "v";
    var5name = "w";
    var6name = "ascii";
    xcenter = 0.;
    ycenter = 0.;
    for (i = 0; i < 21; i++) {
        for (j = 0; j < 21; j++) {
            for (k = 0; k < 6; k++) {
                dist = sqrt((x[i] - xcenter) * (x[i] - xcenter) +
                            (y[j] - ycenter) * (y[j] - ycenter));
                u[i * 126 + j * 6 + k] = (x[i] - xcenter) / dist;
                v[i * 126 + j * 6 + k] = (y[j] - ycenter) / dist;
                w[i * 126 + j * 6 + k] = 0.;

                ascii[i * 126 + j * 6 + k] = (char)(65 + i);
            }
        }
    }

    /*
     * Create the temperature array.
     */
    if ((t=ALLOC_N(float,21*21*6)) != NULL)
    {
       double Ca,Cb,Cc,Cd;

       Ca = 1.;
       Cb = 3.;
       Cc = 5.;
       Cd = 7.;
       for (i=0; i<21; ++i)
          for (j=0; j<21; ++j)
             for (k=0; k<6; ++k)
             {
                double Yv,Zv;
                int    a;

                a    = i*21*6 + j*6 + k;
                Yv   = (y[a]-Ymin) / (Ymax-Ymin);
                Zv   = (z[a]-Zmin) / (Zmax-Zmin);
                t[a] = Ca + Cb*Yv + Cc*exp(Cd*Zv);
             }
    }

    /*
     * Create the material and species arrays.
     */
    matname = "mat1";
    nmats = 3;
    matnos[0] = 1;
    matnos[1] = 4;
    matnos[2] = 9;
    mixlen = 0;
    matnames = (char**)malloc(sizeof(char*)*4);
    matnames[0] = _db_safe_strdup("Inner goop");
    matnames[1] = _db_safe_strdup("Middle stuff");
    matnames[2] = _db_safe_strdup("Outer junk");

    /*
     * Material 1 has 2 species at static concentrations 0.1 and 0.9
     * Material 2 has 3 species at static concentrations 0.2, 0.3, and 0.5
     * Material 3 has a single species
     */
    nmatspec[0] = 2;
    nmatspec[1] = 3;
    nmatspec[2] = 1;
    nspecmf = 0;

    /*
     * Put in the material in 3 shells.
     */
    for (i = 0; i < 20; i++) {
        for (j = 0; j < 20; j++) {
            /* Clean zones, material 1 */
            for (k = 0; k < 2; k++) {
                int zone = i * 100 + j * 5 + k;

                matlist[zone] = 1;
                speclist[zone] = nspecmf + 1;
                specmf[nspecmf++] = 0.1;
                specmf[nspecmf++] = 0.9;
            }
            
            /* Mixed zones, materials 1 and 4 */
            for (k = 2; k < 3; k++) {
                int zone = i * 100 + j * 5 + k;

                matlist[zone] = -mixlen - 1;
                speclist[zone]= -mixlen - 1;

                /* material 1 */
                mix_mat [mixlen]   = 1;
                mix_next[mixlen]   = mixlen+2;
                mix_zone[mixlen]   = zone + 1;
                mix_vf  [mixlen]   = (float)i/20.;

                mix_speclist[mixlen] = nspecmf + 1;
                specmf[nspecmf++] = 0.1;
                specmf[nspecmf++] = 0.9;

                mixlen++;

                /* material 4 */
                mix_mat [mixlen] = 4;
                mix_next[mixlen] = 0;
                mix_zone[mixlen] = zone + 1;
                mix_vf  [mixlen] = 1.0 - ((float)i/20.);

                mix_speclist[mixlen] = nspecmf + 1;
                specmf[nspecmf++] = 0.2;
                specmf[nspecmf++] = 0.3;
                specmf[nspecmf++] = 0.5;

                mixlen++;
            }

            /* Clean zones, material 4 */
            for (k = 3; k < 4; k++) {
                int zone = i * 100 + j * 5 + k;
                matlist[zone] = 4;
                speclist[zone] = nspecmf + 1;
                specmf[nspecmf++] = 0.2;
                specmf[nspecmf++] = 0.3;
                specmf[nspecmf++] = 0.5;
            }

            /* Clean zones, material 9 */
            for (k = 4; k < 5; k++) {
                int zone = i * 100 + j * 5 + k;
                matlist[zone] = 9;
                speclist[zone] = 0;
            }
        }
    }

    /*
     * Calculate the external face list.
     */
    fl = DBCalcExternalFacelist(zonelist, nnodes, 0, &zshapesize,
                                &zshapecnt, 1, matlist, 0);

    nfaces = fl->nfaces;
    fshapecnt = fl->nfaces;
    fshapesize = 4;
    lfacelist = fl->lnodelist;
    for (i = 0; i < lfacelist; i++)
        facelist[i] = fl->nodelist[i];
    for (i = 0; i < nfaces; i++)
        zoneno[i] = fl->zoneno[i];

    DBFreeFacelist(fl);

    /*
     * Write out the variables.
     */
    cycle = 48;
    time = 4.8;
    dtime = 4.8;

    /*
     * The length includes the terminating NULL character.
     */
    i = 29;
    DBWrite (dbfile, "_meshtvinfo", "mesh ucdmesh3d;pseudocolor d",
             &i, 1, DB_CHAR);
    i = 50;
    DBWrite (dbfile, "_meshtv_defvars",
             "vel vector {u,v,w};speed scalar sqrt(u*u+v*v+w*w)",
             &i, 1, DB_CHAR);

    optlist = DBMakeOptlist(12);
    DBAddOption(optlist, DBOPT_CYCLE, &cycle);
    DBAddOption(optlist, DBOPT_TIME, &time);
    DBAddOption(optlist, DBOPT_DTIME, &dtime);
    DBAddOption(optlist, DBOPT_TOPO_DIM, (void *)&td);
    DBAddOption(optlist, DBOPT_XLABEL, (char *) "X Axis");
    DBAddOption(optlist, DBOPT_YLABEL, (char *) "Y Axis");
    DBAddOption(optlist, DBOPT_ZLABEL, (char *) "Z Axis");
    DBAddOption(optlist, DBOPT_XUNITS, (char *) "cm");
    DBAddOption(optlist, DBOPT_YUNITS, (char *) "cm");
    DBAddOption(optlist, DBOPT_ZUNITS, (char *) "cm");
    DBAddOption(optlist, DBOPT_MATNAMES, matnames);

    DBPutFacelist(dbfile, "fl1", nfaces, 3, facelist, lfacelist, 0, zoneno,
                  &fshapesize, &fshapecnt, 1, NULL, NULL, 0);

    DBSetDeprecateWarnings(0);
    DBPutZonelist(dbfile, "zl1", nzones, 3, zonelist, lzonelist, 0,
                  &zshapesize, &zshapecnt, 1);
    DBSetDeprecateWarnings(3);

    DBPutUcdmesh(dbfile, meshname, 3, NULL, coords, nnodes, nzones,
                 "zl1", "fl1", DB_FLOAT, optlist);

    vars[0] = d;
    varnames[0] = var1name;

    DBPutUcdvar(dbfile, var1name, meshname, 1, (DBCAS_t) varnames,
        vars, nnodes, NULL, 0, DB_FLOAT, DB_NODECENT, optlist);

    vars[0] = p;
    varnames[0] = var2name;

    DBPutUcdvar(dbfile, var2name, meshname, 1, (DBCAS_t) varnames,
        vars, nnodes, NULL, 0, DB_FLOAT, DB_NODECENT, optlist);

    vars[0] = u;
    varnames[0] = var3name;

    DBPutUcdvar(dbfile, var3name, meshname, 1, (DBCAS_t) varnames,
        vars, nnodes, NULL, 0, DB_FLOAT, DB_NODECENT, optlist);

    vars[0] = v;
    varnames[0] = var4name;

    DBPutUcdvar(dbfile, var4name, meshname, 1, (DBCAS_t) varnames,
        vars, nnodes, NULL, 0, DB_FLOAT, DB_NODECENT, optlist);

    vars[0] = w;
    varnames[0] = var5name;

    DBPutUcdvar(dbfile, var5name, meshname, 1, (DBCAS_t) varnames,
        vars, nnodes, NULL, 0, DB_FLOAT, DB_NODECENT, optlist);

    vars[0]     = t;
    varnames[0] = "t";
    DBPutUcdvar(dbfile, "t", meshname, 1, (DBCAS_t) varnames,
        vars, nnodes, NULL, 0, DB_FLOAT, DB_NODECENT, optlist);

    DBPutMaterial(dbfile, matname, meshname, nmats, matnos, matlist, &nzones,
                  1, mix_next, mix_mat, mix_zone, mix_vf, mixlen, DB_FLOAT,
                  optlist);

    DBPutMatspecies(dbfile, "species", matname, nmats, nmatspec, speclist,
                    &nzones, 1, nspecmf, specmf, mix_speclist, mixlen, 
                    DB_FLOAT, optlist);

    vars[0] = (float*) ascii;
    varnames[0] = var6name;
    DBAddOption(optlist, DBOPT_ASCII_LABEL, &onvalue);
    DBPutUcdvar(dbfile, var6name, meshname, 1, (DBCAS_t) varnames,
        vars, nnodes, NULL, 0, DB_CHAR, DB_NODECENT, optlist);

    DBFreeOptlist(optlist);

    /* Free variables */
    FREE(t);
    for(i=0;i<nmats;i++)
        FREE(matnames[i]);
    FREE(matnames);
}

/*-------------------------------------------------------------------------
 * Function:    build_poly3d
 *
 * Purpose:     Build a 3-d UCD mesh with polyhedra and add it to the
 *              open database.
 *
 * Return:      Success:        void
 *
 *              Failure:
 *
 * Programmer:
 *
 * Modifications:
 *
 *    Jeremy Meredith, Tue May 25 09:41:21 PDT 1999
 *    Added a (null) optlist to the call to PutZonelist2.
 *
 *    Lisa J. Roberts, Fri Apr  7 11:01:12 PDT 2000
 *    Removed flid, zlid, matid, meshid, var1id, var2id, var3id,
 *    var4id, var5id, and var6id, which were unused.
 *
 *    Sean Ahern, Thu Feb  7 14:07:41 PST 2002
 *    Added material names.
 *
 *-------------------------------------------------------------------------*/
/*ARGSUSED*/
static void
build_poly3d(DBfile *dbfile, int size, int order)
{
    int            cycle;
    float          time;
    double         dtime;
    char          *coordnames[3];
    float         *coords[3];
    float         *x, *y, *z;
    float          Ca, Cb, Cc, Cd;
    float          Xmin,Ymin,Zmin;
    float          Xmax,Ymax,Zmax;
    int            nx, ny, nz;
#if 0
    int            ndims = 3;
#endif
    int            nfaces, nzone1, nzone2, nzone3, nzones, nnodes;
    int            lo_offset, hi_offset;
    int            lfacelist, lzonelist;
    int            fshapesize[3], fshapecnt[3], nfshapes;
    int            zshapetype[3], zshapesize[3], zshapecnt[3], nzshapes;
    int           *zonelist;
    int           *facelist;
    int           *zoneno;

    char          *meshname, *var1name, *var2name, *var3name, *var4name;
    char          *var5name, *matname;
    float         *vars[1];
    char          *varnames[1];
    float         *d, *p, *u, *v, *w, *t;

    int            nmats;
    int            matnos[3];
    int           *matlist;
    int            mixlen;

    DBoptlist     *optlist;
    char         **matnames = NULL;

    DBfacelist    *fl;

    int            i, j, k;
    int            in, iz;
    double         xcenter, ycenter, zcenter;
    double         theta, dtheta;
    double         r, dr;
    double         h, dh;
    double         dist;

    switch (size) {
        case 1:
            nx = 20;
            ny = 20;
            nz = 10;
            break;
        case 2:
            nx = 40;
            ny = 40;
            nz = 20;
            break;
        case 3:
            nx = 80;
            ny = 80;
            nz = 40;
            break;
    }

    /*
     * Create the mesh.
     */
    meshname = "ucdmesh3d";
    coordnames[0] = "xcoords";
    coordnames[1] = "ycoords";
    coordnames[2] = "zcoords";

    nnodes = (nx + 1) * ((ny / 2 + 1) * (nz / 2) + (ny + 1) * (nz / 2 + 1));
    x = ALLOC_N (float, nnodes);
    y = ALLOC_N (float, nnodes);
    z = ALLOC_N (float, nnodes);
    coords[0] = x;
    coords[1] = y;
    coords[2] = z;

    in = 0;

    dh = 40. / nx;
    dtheta = (180. / (ny / 2)) * (3.1415926 / 180.);
    dr = 3. / nz;
    h = 0.;
    for (i = 0; i < nx + 1; i++) {
        theta = 0.;
        for (j = 0; j < ny / 2 + 1; j++) {
            r = 2.;
            for (k = 0; k < nz / 2; k++) {
               x[in]    = r * cos(theta);
               y[in]    = r * sin(theta);
               z[in]    = h;
               in++;
               r       += dr;
            }
            theta += dtheta;
        }
        h += dh;
    }

    dh = 40. / nx;
    dtheta = (180. / ny) * (3.1415926 / 180.);
    dr = 3. / nz;
    h = 0.;
    for (i = 0; i < nx + 1; i++) {
        theta = 0.;
        for (j = 0; j < ny + 1; j++) {
            r = 2. + (3. / 2.);
            for (k = nz / 2; k < nz + 1; k++) {
               x[in]    = r * cos(theta);
               y[in]    = r * sin(theta);
               z[in]    = h;
               in++;
               r       += dr;
            }
            theta += dtheta;
        }
        h += dh;
    }

    Xmin = x[0];
    Xmin = x[0];
    Xmax = x[0];
    Ymin = y[0];
    Ymax = y[0];
    Zmin = z[0];
    Zmax = z[0];
    for (i = 0; i < nnodes; i++) {
        Xmin = Min(Xmin, x[i]);
        Xmax = Max(Xmax, x[i]);
        Ymin = Min(Ymin, y[i]);
        Ymax = Max(Ymax, y[i]);
        Zmin = Min(Zmin, z[i]);
        Zmax = Max(Zmax, z[i]);
    }

    nzone1 = nx * (ny / 2) * (nz / 2 - 1);
    nzone2 = nx * (ny / 2) * 1;
    nzone3 = nx * ny * (nz / 2);
    nzones = nzone1 + nzone2 + nzone3;
    lo_offset = 0;
    hi_offset = 0;
    lzonelist = nzone1 * 8 + nzone2 * 38 + nzone3 * 8;
    zonelist = ALLOC_N (int, lzonelist);
    zshapecnt[0] = nzone1;
    zshapecnt[1] = nzone2;
    zshapecnt[2] = nzone3;
    zshapesize[0] = 8;
    zshapesize[1] = 38 * nzone2;
    zshapesize[2] = 8;
    zshapetype[0] = DB_ZONETYPE_HEX;
    zshapetype[1] = DB_ZONETYPE_POLYHEDRON;
    zshapetype[2] = DB_ZONETYPE_HEX;
    nzshapes = 3;

    iz = 0;
    for (i = 0; i < nx; i++) {
        for (j = 0; j < ny / 2; j++) {
            for (k = 0; k < nz / 2 - 1; k++) {
                zonelist[iz]     = (i)     * (ny / 2 + 1) * (nz / 2) +
                                   (j)     * (nz / 2) + k;
                zonelist[iz + 1] = (i)     * (ny / 2 + 1) * (nz / 2) +
                                   (j + 1) * (nz / 2) + k;
                zonelist[iz + 2] = (i + 1) * (ny / 2 + 1) * (nz / 2) +
                                   (j + 1) * (nz / 2) + k;
                zonelist[iz + 3] = (i + 1) * (ny / 2 + 1) * (nz / 2) +
                                   (j)     * (nz / 2) + k;
                zonelist[iz + 4] = (i)     * (ny / 2 + 1) * (nz / 2) +
                                   (j)     * (nz / 2) + k + 1;
                zonelist[iz + 5] = (i)     * (ny / 2 + 1) * (nz / 2) +
                                   (j + 1) * (nz / 2) + k + 1;
                zonelist[iz + 6] = (i + 1) * (ny / 2 + 1) * (nz / 2) +
                                   (j + 1) * (nz / 2) + k + 1;
                zonelist[iz + 7] = (i + 1) * (ny / 2 + 1) * (nz / 2) +
                                   (j)     * (nz / 2) + k + 1;
                iz += 8;
            }
        }
    }

    in = (nx + 1) * (ny / 2 + 1) * (nz / 2);
    for (i = 0; i < nx; i++) {
        for (j = 0; j < ny / 2; j++) {
            /* Number of faces */
            zonelist[iz] = 7;

            /* Bottom four sided face */
            zonelist[iz + 1]  = 4;
            zonelist[iz + 2]  = (i)     * (ny / 2 + 1) * (nz / 2) +
                                (j)     * (nz / 2) + (nz / 2) - 1;
            zonelist[iz + 3]  = (i)     * (ny / 2 + 1) * (nz / 2) +
                                (j + 1) * (nz / 2) + (nz / 2) - 1;
            zonelist[iz + 4]  = (i + 1) * (ny / 2 + 1) * (nz / 2) +
                                (j + 1) * (nz / 2) + (nz / 2) - 1;
            zonelist[iz + 5]  = (i + 1) * (ny / 2 + 1) * (nz / 2) +
                                (j)     * (nz / 2) + (nz / 2) - 1;

            /* Two five sided side faces */
            zonelist[iz + 6]  = 5;
            zonelist[iz + 7]  = (i)     * (ny / 2 + 1) * (nz / 2) +
                                (j)     * (nz / 2) + (nz / 2) - 1;
            zonelist[iz + 8]  = in + (i)     * (ny + 1) * (nz / 2 + 1) +
                                     ((2 * j))     * (nz / 2 + 1);
            zonelist[iz + 9]  = in + (i)     * (ny + 1) * (nz / 2 + 1) +
                                     ((2 * j) + 1) * (nz / 2 + 1);
            zonelist[iz + 10] = in + (i)     * (ny + 1) * (nz / 2 + 1) +
                                     ((2 * j + 1) + 1) * (nz / 2 + 1);
            zonelist[iz + 11] = (i)     * (ny / 2 + 1) * (nz / 2) +
                                (j + 1) * (nz / 2) + (nz / 2) - 1;
            zonelist[iz + 12] = 5;
            zonelist[iz + 13] = (i + 1) * (ny / 2 + 1) * (nz / 2) +
                                (j)     * (nz / 2) + (nz / 2) - 1;
            zonelist[iz + 14] = (i + 1) * (ny / 2 + 1) * (nz / 2) +
                                (j + 1) * (nz / 2) + (nz / 2) - 1;
            zonelist[iz + 15] = in + (i + 1) * (ny + 1) * (nz / 2 + 1) +
                                     ((2 * j + 1) + 1) * (nz / 2 + 1);
            zonelist[iz + 16] = in + (i + 1) * (ny + 1) * (nz / 2 + 1) +
                                     ((2 * j) + 1) * (nz / 2 + 1);
            zonelist[iz + 17] = in + (i + 1) * (ny + 1) * (nz / 2 + 1) +
                                     ((2 * j))     * (nz / 2 + 1);

            /* Two four sided side faces */
            zonelist[iz + 18] = 4;
            zonelist[iz + 19] = (i)     * (ny / 2 + 1) * (nz / 2) +
                                (j)     * (nz / 2) + (nz / 2) - 1;
            zonelist[iz + 20] = (i + 1) * (ny / 2 + 1) * (nz / 2) +
                                (j)     * (nz / 2) + (nz / 2) - 1;
            zonelist[iz + 21] = in + (i + 1) * (ny + 1) * (nz / 2 + 1) +
                                     ((2 * j))     * (nz / 2 + 1);
            zonelist[iz + 22] = in + (i)     * (ny + 1) * (nz / 2 + 1) +
                                     ((2 * j))     * (nz / 2 + 1);
            zonelist[iz + 23] = 4;
            zonelist[iz + 24] = (i)     * (ny / 2 + 1) * (nz / 2) +
                                (j + 1) * (nz / 2) + (nz / 2) - 1;
            zonelist[iz + 25] = in + (i)     * (ny + 1) * (nz / 2 + 1) +
                                     ((2 * j + 1) + 1) * (nz / 2 + 1);
            zonelist[iz + 26] = in + (i + 1) * (ny + 1) * (nz / 2 + 1) +
                                     ((2 * j + 1) + 1) * (nz / 2 + 1);
            zonelist[iz + 27] = (i + 1) * (ny / 2 + 1) * (nz / 2) +
                                (j + 1) * (nz / 2) + (nz / 2) - 1;

            /* Two four sided top faces */
            zonelist[iz + 28] = 4;
            zonelist[iz + 29] = in + (i)     * (ny + 1) * (nz / 2 + 1) +
                                     ((2 * j))     * (nz / 2 + 1);
            zonelist[iz + 30] = in + (i + 1) * (ny + 1) * (nz / 2 + 1) +
                                     ((2 * j))     * (nz / 2 + 1);
            zonelist[iz + 31] = in + (i + 1) * (ny + 1) * (nz / 2 + 1) +
                                     ((2 * j) + 1) * (nz / 2 + 1);
            zonelist[iz + 32] = in + (i)     * (ny + 1) * (nz / 2 + 1) +
                                     ((2 * j) + 1) * (nz / 2 + 1);
            zonelist[iz + 33] = 4;
            zonelist[iz + 34] = in + (i)     * (ny + 1) * (nz / 2 + 1) +
                                     ((2 * j + 1))     * (nz / 2 + 1);
            zonelist[iz + 35] = in + (i + 1) * (ny + 1) * (nz / 2 + 1) +
                                     ((2 * j + 1))     * (nz / 2 + 1);
            zonelist[iz + 36] = in + (i + 1) * (ny + 1) * (nz / 2 + 1) +
                                     ((2 * j + 1) + 1) * (nz / 2 + 1);
            zonelist[iz + 37] = in + (i)     * (ny + 1) * (nz / 2 + 1) +
                                     ((2 * j + 1) + 1) * (nz / 2 + 1);

            iz += 38;
        }
    }

    for (i = 0; i < nx; i++) {
        for (j = 0; j < ny; j++) {
            for (k = 0; k < nz / 2; k++) {
                zonelist[iz]     = in + (i)     * (ny + 1) * (nz / 2 + 1) +
                                        (j)     * (nz / 2 + 1) + k;
                zonelist[iz + 1] = in + (i)     * (ny + 1) * (nz / 2 + 1) +
                                        (j + 1) * (nz / 2 + 1) + k;
                zonelist[iz + 2] = in + (i + 1) * (ny + 1) * (nz / 2 + 1) +
                                        (j + 1) * (nz / 2 + 1) + k;
                zonelist[iz + 3] = in + (i + 1) * (ny + 1) * (nz / 2 + 1) +
                                        (j)     * (nz / 2 + 1) + k;
                zonelist[iz + 4] = in + (i)     * (ny + 1) * (nz / 2 + 1) +
                                        (j)     * (nz / 2 + 1) + k + 1;
                zonelist[iz + 5] = in + (i)     * (ny + 1) * (nz / 2 + 1) +
                                        (j + 1) * (nz / 2 + 1) + k + 1;
                zonelist[iz + 6] = in + (i + 1) * (ny + 1) * (nz / 2 + 1) +
                                        (j + 1) * (nz / 2 + 1) + k + 1;
                zonelist[iz + 7] = in + (i + 1) * (ny + 1) * (nz / 2 + 1) +
                                        (j)     * (nz / 2 + 1) + k + 1;
                iz += 8;
            }
        }
    }

    /*
     * Create the density and pressure arrays.
     */
    var1name = "d";
    var2name = "p";
    d = ALLOC_N (float, nnodes);
    p = ALLOC_N (float, nnodes);

    xcenter = 0.;
    ycenter = 0.;
    for (i = 0; i < nnodes; i++) {
        dist = sqrt((x[i] - xcenter) * (x[i] - xcenter) +
                    (y[i] - ycenter) * (y[i] - ycenter));
        d[i] = dist;
        p[i] = 1. / (dist + .0001);
    }

    /*
     * Create the velocity component arrays.
     */
    var3name = "u";
    var4name = "v";
    var5name = "w";
    u = ALLOC_N (float, nnodes);
    v = ALLOC_N (float, nnodes);
    w = ALLOC_N (float, nnodes);

    xcenter = 0.;
    ycenter = 3.5;
    zcenter = 20.;
    for (i = 0; i < nnodes; i++) {
        dist = sqrt((x[i] - xcenter) * (x[i] - xcenter) +
                    (y[i] - ycenter) * (y[i] - ycenter) +
                    (z[i] - zcenter) * (z[i] - zcenter));
        u[i] = (x[i] - xcenter) / dist;
        v[i] = (y[i] - ycenter) / dist;
        w[i] = (z[i] - zcenter) / dist;
    }

    /*
     * Create the temperature array.
     */
    t = ALLOC_N (float, nnodes);
    Ca = 1.;
    Cb = 3.;
    Cc = 5.;
    Cd = 7.;
    for (i = 0; i < nnodes; i++) {
        t[i] = Ca + Cb * ((y[i] - Ymin) / (Ymax - Ymin)) +
                Cc * exp(Cd * ((z[i] - Zmin) / (Zmax - Zmin)));
    }

    /*
     * Create the material array.
     */
    matname = "mat1";
    matlist = ALLOC_N (int, nzones);
    nmats = 3;
    matnos[0] = 1;
    matnos[1] = 2;
    matnos[2] = 3;
    mixlen = 0;
    matnames = (char**)malloc(sizeof(char*)*nmats);
    matnames[0] = _db_safe_strdup("Something important");
    matnames[1] = _db_safe_strdup("Of little interest");
    matnames[2] = _db_safe_strdup("Notable");

    /*
     * Put in the material in 3 shells.
     */
    in = 0;
    for (i = 0; i < nx; i++) {
        for (j = 0; j < ny / 2; j++) {
            for (k = 0; k < nz / 2 - 1; k++) {
                matlist[in] = 1;
                in++;
            }
        }
    }

    for (i = 0; i < nx; i++) {
        for (j = 0; j < ny / 2; j++) {
            matlist[in] = 2;
            in++;
        }
    }

    for (i = 0; i < nx; i++) {
        for (j = 0; j < ny; j++) {
            for (k = nz / 2; k < nz; k++) {
                matlist[in] = 3;
                in++;
            }
        }
    }

    /*
     * Calculate the external face list.
     */
    fl = DBCalcExternalFacelist2(zonelist, nnodes, lo_offset, hi_offset, 0,
                                 zshapetype, zshapesize, zshapecnt, nzshapes,
                                 matlist, 1);

    nfaces = fl->nfaces;
    nfshapes = fl->nshapes;
    for (i = 0; i < nfshapes; i++) {
        fshapecnt[i]  = fl->shapecnt[i];
        fshapesize[i] = fl->shapesize[i];
    }
    lfacelist = fl->lnodelist;
    facelist = ALLOC_N (int, lfacelist);
    for (i = 0; i < lfacelist; i++)
        facelist[i] = fl->nodelist[i];
    zoneno = ALLOC_N (int, nfaces);
    for (i = 0; i < nfaces; i++)
        zoneno[i] = fl->zoneno[i];

    DBFreeFacelist(fl);

    /*
     * Write out the variables.
     */
    cycle = 48;
    time = 4.8;
    dtime = 4.8;

    /*
     * The length includes the terminating NULL character.
     */
    i = 29;
    DBWrite (dbfile, "_meshtvinfo", "mesh ucdmesh3d;pseudocolor d",
             &i, 1, DB_CHAR);
    i = 50;
    DBWrite (dbfile, "_meshtv_defvars",
             "vel vector {u,v,w};speed scalar sqrt(u*u+v*v+w*w)",
             &i, 1, DB_CHAR);

    optlist = DBMakeOptlist(11);
    DBAddOption(optlist, DBOPT_CYCLE, &cycle);
    DBAddOption(optlist, DBOPT_TIME, &time);
    DBAddOption(optlist, DBOPT_DTIME, &dtime);
    DBAddOption(optlist, DBOPT_XLABEL, (char *) "X Axis");
    DBAddOption(optlist, DBOPT_YLABEL, (char *) "Y Axis");
    DBAddOption(optlist, DBOPT_ZLABEL, (char *) "Z Axis");
    DBAddOption(optlist, DBOPT_XUNITS, (char *) "cm");
    DBAddOption(optlist, DBOPT_YUNITS, (char *) "cm");
    DBAddOption(optlist, DBOPT_ZUNITS, (char *) "cm");
    DBAddOption(optlist, DBOPT_MATNAMES, matnames);

    DBPutFacelist(dbfile, "fl1", nfaces, 3, facelist, lfacelist, 0,
                        zoneno, fshapesize, fshapecnt, nfshapes,
                        NULL, NULL, 0);

    DBPutZonelist2(dbfile, "zl1", nzones, 3, zonelist, lzonelist, 0,
                         lo_offset, hi_offset, zshapetype, zshapesize,
                         zshapecnt, nzshapes, NULL);

    DBPutUcdmesh(dbfile, meshname, 3, (DBCAS_t) coordnames,
        coords, nnodes, nzones, "zl1", "fl1", DB_FLOAT, optlist);

    vars[0] = d;
    varnames[0] = var1name;

    DBPutUcdvar(dbfile, var1name, meshname, 1, (DBCAS_t) varnames,
        vars, nnodes, NULL, 0, DB_FLOAT, DB_NODECENT, optlist);

    vars[0] = p;
    varnames[0] = var2name;

    DBPutUcdvar(dbfile, var2name, meshname, 1, (DBCAS_t) varnames,
        vars, nnodes, NULL, 0, DB_FLOAT, DB_NODECENT, optlist);

    vars[0] = u;
    varnames[0] = var3name;

    DBPutUcdvar(dbfile, var3name, meshname, 1, (DBCAS_t) varnames,
        vars, nnodes, NULL, 0, DB_FLOAT, DB_NODECENT, optlist);

    vars[0] = v;
    varnames[0] = var4name;

    DBPutUcdvar(dbfile, var4name, meshname, 1, (DBCAS_t) varnames,
        vars, nnodes, NULL, 0, DB_FLOAT, DB_NODECENT, optlist);

    vars[0] = w;
    varnames[0] = var5name;

    DBPutUcdvar(dbfile, var5name, meshname, 1, (DBCAS_t) varnames,
        vars, nnodes, NULL, 0, DB_FLOAT, DB_NODECENT, optlist);

    vars[0]     = t;
    varnames[0] = "t";
    DBPutUcdvar(dbfile, "t", meshname, 1, (DBCAS_t) varnames,
        vars, nnodes, NULL, 0, DB_FLOAT, DB_NODECENT, optlist);

    DBPutMaterial(dbfile, matname, meshname, nmats, matnos,
                        matlist, &nzones, 1, NULL, NULL, NULL,
                        NULL, mixlen, DB_FLOAT, optlist);

    DBFreeOptlist(optlist);

    /*
     * Free the temporary storage.
     */
    FREE (x);
    FREE (y);
    FREE (z);
    FREE (d);
    FREE (p);
    FREE (u);
    FREE (v);
    FREE (w);
    FREE (t);
    FREE (matlist);
    FREE (zonelist);
    FREE (facelist);
    FREE (zoneno);
    for(i=0;i<nmats;i++)
        FREE(matnames[i]);
    FREE(matnames);
}

/*-------------------------------------------------------------------------
 * Function:    build_carray
 *
 * Purpose:     Build a compound array and add it to the open database.
 *
 * Return:      Success:
 *
 *              Failure:
 *
 * Programmer:  robb@cloud
 *              Wed Nov 23 10:18:41 EST 1994
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void
build_carray(DBfile * dbfile)
{
    char const    *ename[3];
    int            esize[3];
    float          val[18];
    int            i;

    ename[0] = "a";
    esize[0] = 4;
    ename[1] = "b";
    esize[1] = 6;
    ename[2] = "c";
    esize[2] = 8;

    for (i = 0; i < 18; i++)
        val[i] = sin(6.28 * i / 18.0);

    DBPutCompoundarray(dbfile, "carray",  /*array */
                       ename, esize, 3,  /*simple arrays */
                       val, 18, DB_FLOAT,  /*values */
                       NULL);   /*options */
}

/*-------------------------------------------------------------------------
 * Function:    build_curve
 *
 * Purpose:     Build a couple of curves and add them to the open database.
 *
 * Return:      void
 *
 * Programmer:  Robb Matzke
 *              robb@callisto.nuance.com
 *              May 15, 1996
 *
 * Modifications:
 *     Robb Matzke, 1999-04-12
 *     The hdf5 driver allows the caller to specify the name of the supplied
 *     data array, but the pdb driver makes the user know something about
 *     the implementation of the pdb driver.
 *
 *     Mark C. Miller, Wed Jul 14 15:25:16 PDT 2010
 *     Changed name for "sincurve_xvals" so that it does NOT collide with
 *     'friendly' named versions of itself.
 *-------------------------------------------------------------------------
 */
static void
build_curve (DBfile *dbfile, int driver)
{
   float        x[20], y[2][20] ;
   float        r[40], theta[40] ;
   int          i, one=1, coord_sys = DB_SPHERICAL;
   DBoptlist    *opts ;

   /*
    * Sine and cosine for 1/2 cycle.  Both curves have the same
    * X data points and share the data in the file.
    */
   for (i=0; i<20; i++) {
      x[i] = i * M_PI / 20.0 ;
      y[0][i] = sin (x[i]) ;
      y[1][i] = cos (x[i]) ;
   }

   DBSetFriendlyHDF5Names(1);
   opts = DBMakeOptlist (10) ;
   DBAddOption (opts, DBOPT_XLABEL, (char *) "X Axis") ;
   DBAddOption (opts, DBOPT_YLABEL, (char *) "Y Axis") ;
   DBAddOption (opts, DBOPT_XUNITS, (char *) "radians") ;
   DBAddOption (opts, DBOPT_HIDE_FROM_GUI, &one) ;

   /*
    * Write the `sincurve' curve. The hdf5 driver allows the user to specify
    * the name which will be used to store the x values, but the pdb driver
    * requires us to know where the values were stored.
    */
   DBPutCurve (dbfile, "sincurve", x, y[0], DB_FLOAT, 20, opts);

#ifndef _WIN32
#warning NEW FUNCTIONALITY TO LOOK AT
#endif
   {
       int j, dims=1;
       one=0;
       for (j = 0; j < 10; j++)
       {
           char tmp[32];
           double t = j;
           for (i=0; i<20; i++)
              y[0][i] *= ((float) (19-i))/19;
           snprintf(tmp,sizeof(tmp),"time_series%02d", j);
           DBMkDir(dbfile, tmp);
           snprintf(tmp,sizeof(tmp),"time_series%02d/sincurve", j);
           DBPutCurve (dbfile, tmp, x, y[0], DB_FLOAT, 20, opts);
           snprintf(tmp,sizeof(tmp),"time_series%02d/dtime", j);
           DBWrite(dbfile, tmp, &t, &dims, 1, DB_DOUBLE);
       }
       one=1;
   }

   /*
    * Write the `coscurve' curve. It shares x values with the `sincurve'
    * curve.
    */
   DBAddOption(opts, DBOPT_XVARNAME, (char *) "sincurve_xvals");
   DBPutCurve (dbfile, "coscurve", NULL, y[1], DB_FLOAT, 20, opts) ;
   DBClearOption(opts, DBOPT_XVARNAME);

   DBAddOption (opts, DBOPT_REFERENCE, (char *) "sincurve") ;
   DBPutCurve (dbfile, "sincurv1", NULL, NULL, DB_FLOAT, 20, opts);

   DBClearOptlist(opts);
   DBAddOption (opts, DBOPT_XLABEL, (char *) "radius") ;
   DBAddOption (opts, DBOPT_YLABEL, (char *) "angle") ;
   DBAddOption (opts, DBOPT_XUNITS, (char *) "radians") ;
   DBAddOption (opts, DBOPT_YUNITS, (char *) "meters") ;
   DBAddOption (opts, DBOPT_COORDSYS, &coord_sys );
  
   for (i=0; i<40; i++) {
      r[i] = 1.0;
      theta[i] = 2 * M_PI * (float) (i / 39.0);
   }

   DBPutCurve (dbfile, "circle", r, theta, DB_FLOAT, 40, opts);

   DBFreeOptlist (opts) ;
   DBSetFriendlyHDF5Names(0);
}

/*-------------------------------------------------------------------------
 * Function:    MakeFiles
 *
 * Purpose:     Given a suffix and a file type, create a suite of data
 *              files that can be used to test the specified driver.
 *
 * Programmer:  robb@cloud
 *              Wed Nov 23 10:06:05 EST 1994
 *
 * Modifications:
 *    Tony Jones, May 30, 1995
 *    Added a Makefile for a 3D curvillinear mesh
 *
 *    Sean Ahern, Fri Feb  7 13:36:24 PST 1997
 *    Removed the return value because it wasn't doing anything.
 *
 *    Sean Ahern, Tue Mar 24 16:08:11 PST 1998
 *    Added a 2D ucd test case.
 *
 *    Eric Brugger, Thu Apr  1 13:54:49 PST 1999
 *    Added a 3D polyhedra test case.
 *
 *-------------------------------------------------------------------------
 */
static void
MakeFiles(char *suffix, int size, int order, int type)
{
    DBfile        *dbfile;
    char           filename[256];

    /*
     * Create the 2D rectilinear data file.
     */
    sprintf(filename, "rect2d.%s", suffix);
    printf("   %s\n", filename);
    dbfile = DBCreate(filename, 0, DB_LOCAL, "2D rectilinear test file", type);
    build_rect2d(dbfile, size, order);
    DBClose(dbfile);

    /*
     * Create the 2D curvilinear data file.
     */
    sprintf(filename, "curv2d.%s", suffix);
    printf("   %s\n", filename);
    dbfile = DBCreate(filename, 0, DB_LOCAL, "2D curvilinear test file", type);
    build_curv2d(dbfile, size, order);
    DBClose(dbfile);

    /*
     * Create the 2D ucd data file.
     */
    sprintf(filename, "ucd2d.%s", suffix);
    printf("   %s\n", filename);
    dbfile = DBCreate(filename, 0, DB_LOCAL, "2D ucd test file", type);
    build_ucd2d(dbfile, size, order);
    DBClose(dbfile);

    /*
     * Create the 3D rectilinear data file.
     */
    sprintf(filename, "rect3d.%s", suffix);
    printf("   %s\n", filename);
    dbfile = DBCreate(filename, 0, DB_LOCAL, "3D rectilinear test file", type);
    build_rect3d(dbfile, size, order);
    DBClose(dbfile);

    /*
     * Create the 3D curvillinear data file.
     */
    sprintf (filename, "curv3d.%s", suffix) ;
    printf ("   %s\n", filename) ;
    dbfile = DBCreate (filename, 0, DB_LOCAL, "3D curvilinear test file", type);
    build_curv3d (dbfile, size, order) ;
    DBClose (dbfile) ;

    /*
     * Create the 3D ucd data file.
     */
    sprintf(filename, "ucd3d.%s", suffix);
    printf("   %s\n", filename);
    dbfile = DBCreate(filename, 0, DB_LOCAL, "3D ucd test file", type);
    build_ucd3d(dbfile, size, order);
    DBClose(dbfile);

    /*
     * Create the 3d arbitrary polyhedra data file.
     */
    sprintf(filename, "poly3d.%s", suffix);
    printf("   %s\n", filename);
    dbfile = DBCreate(filename, 0, DB_LOCAL, "3D ucd test file", type);
    build_poly3d(dbfile, size, order);
    DBClose(dbfile);

    /*
     * Create the Compound Array data file.
     */
    sprintf(filename, "carray.%s", suffix);
    printf("   %s\n", filename);
    dbfile = DBCreate(filename, 0, DB_LOCAL, "Compound Array test file", type);
    build_carray(dbfile);
    DBClose(dbfile);

    /*
     * Create curve data file.
     */
    sprintf (filename, "curve.%s", suffix);
    printf ("   %s\n", filename) ;
    dbfile = DBCreate (filename, 0, DB_LOCAL, "Curve test file", type) ;
    build_curve (dbfile, type) ;
    DBClose (dbfile) ;
}

/*-------------------------------------------------------------------------
 * Function:    main
 *
 * Purpose:     For each type of driver being tested (based on the
 *              #define's at the top of this file) build the file and
 *              put some things in it.
 *
 * Return:      Success:
 *
 *              Failure:
 *
 * Programmer:  robb@cloud
 *              Wed Nov 23 10:03:35 EST 1994
 *
 * Modifications:
 *     Robb Matzke, 2 Dec 1996
 *     Added AIO tests.
 *
 *     Sean Ahern, Wed Jan 22 15:23:55 PST 1997
 *     Got rid of the DEBUG driver testing.  Added a return value.
 *
 *     Robb Matzke, 1999-04-09
 *     Added argument parsing to control the driver which is used.
 *
 *-------------------------------------------------------------------------
 */
int
main(int argc, char *argv[])
{
    int            i;
    int            size;
    int            order;
    int            ntests = 0;
    int		   driver=DB_PDB;
    char	   *file_ext = "pdb";
    int            show_all_errors = FALSE;

    /*
     * parse the execute line.
     */
    size = 1;
    order = DB_ROWMAJOR;
    for (i = 1; i < argc; i++) {
        if (strcmp (argv[i], "-small") == 0)
            size = 1;
        else if (strcmp (argv[i], "-medium") == 0)
            size = 2;
        else if (strcmp (argv[i], "-large") == 0)
            size = 3;
        else if (strcmp (argv[i], "-fortran") == 0)
            order = DB_ROWMAJOR;
        else if (strcmp (argv[i], "-c") == 0)
            order = DB_COLMAJOR;
	else if (!strncmp(argv[i], "DB_PDB", 6)) {
	    driver = StringToDriver(argv[i]);
	    file_ext = "pdb";
	} else if (!strncmp(argv[i], "DB_HDF5", 7)) {
            driver = StringToDriver(argv[i]);
	    file_ext = "h5";
        } else if (!strcmp(argv[i], "show-all-errors")) {
            show_all_errors = 1;
	} else if (argv[i][0] != '\0') {
            printf ("Unknown execute line option.\n");
        }
    }

    /*
     * Abort on any errors.
     */
    DBShowErrors(show_all_errors?DB_ALL_AND_DRVR:DB_ABORT, NULL);

    printf("Testing %s driver...\n", file_ext);
    MakeFiles(file_ext, size, order, driver);
    ntests++;

    if (!ntests)
        printf("No tests performed.\n");

    CleanupDriverStuff();
    return(0);
}
