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
#include <std.c>

#define LEN(A)	(sizeof(A)/sizeof(A[0]))

int
main(int argc, char *argv[])
{
    int             i;
    DBfile         *dbfile = NULL;
    char           *coordnames[3];
    float          *coords[3];
    int		    driver=DB_PDB;
    char            *filename = "arbpoly-zoohybrid.silo";
    DBoptlist       *ol;
    int             retval = 0;

    for (i=1; i<argc; i++) {
	if (!strncmp(argv[i], "DB_PDB",6)) {
	    driver = StringToDriver(argv[i]);
	} else if (!strncmp(argv[i], "DB_HDF5", 7)) {
            driver = StringToDriver(argv[i]);
	} else if (argv[i][0] != '\0') {
	    fprintf(stderr, "%s: ignored argument `%s'\n", argv[0], argv[i]);
	}
    }

    DBShowErrors(DB_ABORT, NULL);
    printf("Creating test file \"%s\".\n", filename);
    dbfile = DBCreate(filename, DB_CLOBBER, DB_LOCAL, "arbitrary shape tests", driver);

    coordnames[0] = "xcoords";
    coordnames[1] = "ycoords";
    coordnames[2] = "zcoords";

    /* 2D tests */
    {
        /* create a 2d ucd mesh in 5 segments of 2 quads, 2 6-vertex polygons,
           2 tris, 1 quad and a final segment of 4 arbitrary polygons (tris,
           a 6-vertex polygon and a quad) */
        float x[] = {0,1,2,3,4,5,6,7,8,9,
                     0,1,2,3,4,5,6,7,8,9,
                     2.5,3.5,7.5,2.5,3.5,7.5};
        float y[] = {0,0,0,0,0,0,0,0,0,0,
                     1,1,1,1,1,1,1,1,1,1,
                    -1,-1,-1,2,2,2};
        float z[] = {0,0,0,0,0,0,0,0,0,0,
                     1,1,1,1,1,1,1,1,1,1,
                    -1,-1,-1,2,2,2};
        int shapesize[] = {4, 0, 3, 4, 6};
        /* the Silo docs include a 3D example where the shapesize entry
           for a DBPutZonelist2 call is the # of slots in the nodelist
           taken up by the given segment of arbitrary shapes. However,
           that convention appears to be supported in VisIt only for 3D
        int shapesize2[] = {4, 20, 3, 4, 6};*/
        int shapesize2[] = {4, 0, 3, 4, 6};
        int shapecnt[] =  {2, 4, 2, 1, 2};
        int shapetype[] = {DB_ZONETYPE_QUAD,
                           DB_ZONETYPE_POLYGON,
                           DB_ZONETYPE_TRIANGLE,
                           DB_ZONETYPE_QUAD,
                           DB_ZONETYPE_POLYGON};
        int nl[] =  {0,1,11,10,
                     1,2,12,11,

                     3,   6,17,16,
                     3,   7,17,6,
                     6,   22,8,18,25,17,7,
                     4,   8,9,19,18,
       
                     4,15,14,
                     5,15,4,

                     5,6,16,15,

                     20,3,13,23,12,2,
                     21,4,14,24,13,3};

        int ndcnts_phzl1[] = {4,4,3,3,6,4,3,3,4,6,6};
        int nl_phzl1[] =  {0,1,11,10,
                           1,2,12,11,

                           6,17,16,
                           7,17,6,
                           22,8,18,25,17,7,
                           8,9,19,18,
       
                           4,15,14,
                           5,15,4,

                           5,6,16,15,

                           20,3,13,23,12,2,
                           21,4,14,24,13,3};

        int ndcnts_phzl2[] = {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
                              2,2,2,2,2,2};
                        /* 0    1    2      3    4     5     6    7    8    9      10   11 */
        int nl_phzl2[] = {0,1, 1,2, 2,20, 20,3, 3,21, 21,4, 4,5, 5,6, 6,7, 7,22, 22,8, 8,9,

                        /* 12    13    14    15    16    17    18    19    20    21 */
                          0,10, 1,11, 2,12, 3,13, 4,14, 5,15, 6,16, 7,17, 8,18, 9,19,

 /*  22     23     24     25     26    27      28     29     30     31     32     33 */
   10,11, 11,12, 12,23, 23,13, 13,24, 24,14, 14,15, 15,16, 16,17, 17,25, 25,18, 18,19,

                        /* 34    35 */
                          4,15, 6,17};
        int edgcnts_phzl2[] = {4,4, 3,3,6,4, 3,3, 4, 6,6};
        int el_phzl2[] = {0,13,22,12,
                          1,14,23,13,

                          18,35,30,
                          8,19,35,
                          9,10,20,32,31,19,
                          11,21,33,20,

                          34,28,16,
                          6,17,34,

                          7,18,29,17,

                          2,3,15,25,24,14,
                          4,5,16,27,26,15};

        float nvar[] = {0,1,2,3,4,3,2,1,0,-1,
                        0,1,2,3,4,3,2,1,0,-1,
                        2.5,3.5,1.5,2.5,3.5,1.5};

        float zvar[] = {0,1,1,0,-1,-2,3,4,2,2,3};
        int pass;

        coords[0] = x;
        coords[1] = y;
        coords[2] = z;

        for (pass = 2; pass <= 3; pass++)
        {
            if (pass == 2)
            {
                DBMkDir(dbfile, "2D");
                DBSetDir(dbfile, "2D");
            }
            else
            {
                DBMkDir(dbfile, "2Dz");
                DBSetDir(dbfile, "2Dz");
            }

            /* Typical 2D mesh with obsolete zonelist method */
            DBPutUcdmesh(dbfile, "mesh1_zl1", pass, (DBCAS_t) coordnames, coords, LEN(x), 11, "zl1",
                NULL, DB_FLOAT, NULL);
            DBSetDeprecateWarnings(0);
            DBPutZonelist(dbfile, "zl1", 11, 2, nl, LEN(nl), 0, shapesize,
                shapecnt, 5);
            DBSetDeprecateWarnings(3);
            DBPutUcdvar1(dbfile, "n11", "mesh1_zl1", nvar, LEN(x), NULL, 0, DB_FLOAT,
                DB_NODECENT, NULL);
            DBPutUcdvar1(dbfile, "z11", "mesh1_zl1", zvar, LEN(zvar), NULL, 0, DB_FLOAT,
                DB_ZONECENT, NULL);

            /* Same as above except using DBPutZonelist2 (newer method) */
            DBPutUcdmesh(dbfile, "mesh1_zl2", pass, (DBCAS_t) coordnames, coords, LEN(x), 11, "zl2",
                NULL, DB_FLOAT, NULL);
            DBPutZonelist2(dbfile, "zl2", 11, 2, nl, LEN(nl), 0, 0, 0, shapetype,
                shapesize2, shapecnt, 5, 0);
            DBPutUcdvar1(dbfile, "n12", "mesh1_zl2", nvar, LEN(x), NULL, 0, DB_FLOAT,
                DB_NODECENT, NULL);
            DBPutUcdvar1(dbfile, "z12", "mesh1_zl2", zvar, LEN(zvar), NULL, 0, DB_FLOAT,
                DB_ZONECENT, NULL);

            /* Same as above except with some ghost zones */
            DBPutUcdmesh(dbfile, "mesh1g_zl2", pass, (DBCAS_t) coordnames, coords, LEN(x), 11, "zlg2",
                NULL, DB_FLOAT, NULL);
            DBPutZonelist2(dbfile, "zlg2", 11, 2, nl, LEN(nl), 0, 3, 3, shapetype,
                shapesize2, shapecnt, 5, 0);
            DBPutUcdvar1(dbfile, "n12g", "mesh1g_zl2", nvar, LEN(x), NULL, 0, DB_FLOAT,
                DB_NODECENT, NULL);
            DBPutUcdvar1(dbfile, "z12g", "mesh1g_zl2", zvar, LEN(zvar), NULL, 0, DB_FLOAT,
                DB_ZONECENT, NULL);

            /* same using ph-zonelist of faces only */
            ol = DBMakeOptlist(3);
            DBAddOption(ol, DBOPT_PHZONELIST, (char *) "phzl");
            DBPutUcdmesh(dbfile, "mesh1_phzl", pass, (DBCAS_t) coordnames, coords, LEN(x), 11, 0,
                NULL, DB_FLOAT, ol);
            DBFreeOptlist(ol);
            DBPutPHZonelist(dbfile, "phzl", 11, ndcnts_phzl1, LEN(nl_phzl1), nl_phzl1, 0,0,0,0,0,0,0,10,0);
            DBPutUcdvar1(dbfile, "n1phzl", "mesh1_phzl", nvar, LEN(x), NULL, 0, DB_FLOAT,
                DB_NODECENT, NULL);
            DBPutUcdvar1(dbfile, "z1phzl", "mesh1_phzl", zvar, LEN(zvar), NULL, 0, DB_FLOAT,
                DB_ZONECENT, NULL);

            /* same using ph-zonelist with explicit edge list */
            ol = DBMakeOptlist(3);
            DBAddOption(ol, DBOPT_PHZONELIST, (char *) "phzl2");
            DBPutUcdmesh(dbfile, "mesh1_phzl2", pass, (DBCAS_t) coordnames, coords, LEN(x), 11, 0,
                NULL, DB_FLOAT, ol);
            DBFreeOptlist(ol);
            DBPutPHZonelist(dbfile, "phzl2", LEN(nl_phzl2)/2, ndcnts_phzl2, LEN(nl_phzl2), nl_phzl2, 0,
                11, edgcnts_phzl2, LEN(el_phzl2), el_phzl2, 0, 0, 10, 0);
            DBPutUcdvar1(dbfile, "n1phzl2", "mesh1_phzl2", nvar, LEN(x), NULL, 0, DB_FLOAT,
                DB_NODECENT, NULL);
            DBPutUcdvar1(dbfile, "z1phzl2", "mesh1_phzl2", zvar, LEN(zvar), NULL, 0, DB_FLOAT,
                DB_ZONECENT, NULL);

            DBSetDir(dbfile, "..");
        }
    }

    /* 3D Tests */
    {
        /* coordinate data */
        float icdata[] = {
            0,   0, 0, 0,
            1,   1, 0, 0,
            2,   2, 0, 0,
            3,   0, 0, 1,
            4,   1, 0, 1,
            5,   2, 0, 1,
        
            6,   0, 1, 0,
            7,   1, 1, 0,
            8,   2, 1, 0,
            9,   0, 1, 1,
            10,  1, 1, 1,
            11,  2, 1, 1,
        
            12,  0, 2, 0,
            13,  1, 2, 0,
            14,  2, 2, 0,
            15,  0, 2, 1,
            16,  1, 2, 1,
            17,  2, 2, 1,
        
            18,  1, 3, 1,
        
            19,  2.75,0,1,
            20,  3,0,0.75,
            21,  3,0.25,1,
        
            22,  3, 1, 1,
            23,  3, 1, 0,
        
            24,  2.75,2,1,
            25,  3,1.75,1,
            26,  3,1.75,0.75,
        
            27,  3, 0, 0
        };
        
        /* node counts for ph zl */
        int nodecnts[] = {
            /* 0 (hex w/6 new faces) */     4,4,4,4,4,4,
            /* 1 (hex w/5 new faces) */     4,4,4,4,4,
            /* 2 (pyramid w/4 new faces) */ 3,3,3,3,
            /* 3 (pyramid w/4 new faces) */ 4,3,3,3,
            /* 4 (wedge w/3 new faces) */   4,4,3,
            /* 5 (pyramid w/3 new faces) */ 4,3,3,
            /* 6 (wedge w/3 new faces) */   4,4,3,
            /* 7 (tet w/3 new faces) */     3,3,3,
            /* 8 (tet w/2 new faces) */     3,3,
            /* 9 (arb w/6 new faces) */     5,5,5,4,4,3,
            /* 10 (arb w/4 new faces) */    5,5,4,3 
        };
        
        /* nodelist for all the faces in ph zl */
        int nodelist[] = {
        
            /* 6 new faces from zone 0 (hex) */
             0, 3, 9, 6,
             0, 1, 4, 3,
             0, 6, 7, 1,
             3, 4,10, 9,
             4, 1, 7,10,
             6, 9,10,7,
        
            /* 5 new faces from zone 1 (hex) */
             7,10,11, 8,
             8,11, 5, 2,
             1, 7, 8, 2,
             5,11,10, 4,
             1, 2, 5, 4,
        
           /* 4 new faces from zone 2 (pyramid) */ 
             7,16,10,
            10,16, 9,
             9,16, 6,
             6,16, 7,
        
           /* 4 new faces from zone 3 (pyramid) */
            15,12, 6, 9,
             9,16,15,
            15,16,12,
             6,12,16,
        
           /* 3 new faces from zone 4 (wedge) */
            10,11,17,16,
             7,16,17, 8,
             8,17,11,
        
           /* 3 new faces from zone 5 (pyramid) */
            6,12,13, 7,
            7,13,16,
           12,16,13,
        
           /* 3 new faces from zone 6 (wedge) */
           16,17,14,13,
            7,13,14, 8,
            8,14,17,
        
           /* 3 new faces from zone 7 (tet) */
           15,16,18,
           15,18,12,
           12,18,16,
        
           /* 2 new faces from zone 8 (tet) */
           16,13,18,
           12,18,13,
        
           /* 6 new faces from zone 9 (arb) */
           19, 21, 22, 11, 5,
           20, 27, 23, 22, 21,
           27, 20, 19, 5, 2,
           2, 8, 23, 27,
           8, 11, 22, 23,
           20, 21, 19,
        
           /* 4 new faces from zone 10 (arb) */
           22, 25, 24, 17, 11,
           8, 17, 24, 26, 23,
           22, 23, 26, 25,
           24, 25, 26
        
        };
        
        /* order-randomized nodelist for all the faces */
        int nodelist2[] = {
        
            /* 6 new faces from zone 0 (hex) */
             9, 6, 0, 3,
             0, 1, 4, 3,
             0, 6, 7, 1,
             3, 4,10, 9,
             1, 7,10, 4,
             6, 9,10,7,
        
            /* 5 new faces from zone 1 (hex) */
             7,10,11, 8,
             8,11, 5, 2,
             1, 7, 8, 2,
             5,11,10, 4,
             1, 2, 5, 4,
        
           /* 4 new faces from zone 2 (pyramid) */ 
             7,16,10,
             9,10,16,
             9,16, 6,
            16, 7, 6,
        
           /* 4 new faces from zone 3 (pyramid) */
             6, 9,15,12,
             9,16,15,
            15,16,12,
             6,12,16,
        
           /* 3 new faces from zone 4 (wedge) */
            16,10,11,17,
            17, 8, 7,16,
            11, 8,17,
        
           /* 3 new faces from zone 5 (pyramid) */
            6,12,13, 7,
            7,13,16,
           12,16,13,
        
           /* 3 new faces from zone 6 (wedge) */
           16,17,14,13,
            7,13,14, 8,
            8,14,17,
        
           /* 3 new faces from zone 7 (tet) */
           15,16,18,
           15,18,12,
           12,18,16,
        
           /* 2 new faces from zone 8 (tet) */
           13,18,16,
           12,18,13,
        
           /* 6 new faces from zone 9 (arb) */
           19, 21, 22, 11, 5,
           23, 22, 21, 20, 27,
           27, 20, 19, 5, 2,
           2, 8, 23, 27,
           22, 23, 8, 11,
           20, 21, 19,
        
           /* 4 new faces from zone 10 (arb) */
           22, 25, 24, 17, 11,
           23, 8, 17, 24, 26,
           22, 23, 26, 25,
           24, 25, 26
        };
        
        int facecnts[] = {
            /* zone 0 (hex) */     6,
            /* zone 1 (hex) */     6,
            /* zone 2 (pyramid) */ 5,
            /* zone 3 (pyramid) */ 5,
            /* zone 4 (wedge) */   5,
            /* zone 5 (pyramid) */ 5,
            /* zone 6 (wedge) */   5,
            /* zone 7 (tet) */     4,
            /* zone 8 (tet) */     4,
            /* zone 9 (arb) */     7,
            /* zone 10 (arb) */    6
        };
        
        int facelist[] = {
            /* zone 0 (hex) */
            0, 1, 2, 3, 4, 5,
        
            /* zone 1 (hex) */
            -5, 10, 8, 9, 7, 6,
        
            /* zone 2 (pyramid) */
            -6, 12, 13, 14, 11, 
        
            /* zone 3 (pyramid) */
            15, 16, 17, 18, -14,
        
            /* zone 4 (wedge) */
            21, 19, 20, -7, -12,
        
            /* zone 5 (pyramid) */
            -15, 23, 22, -19, 24,
        
            /* zone 6 (wedge) */
            -24, -21, 25, 26, 27,
        
            /* zone 7 (tet) */
            28, 29, 30, -18,
        
            /* zone 8 (tet) */
            31, 32, -31, -25,
        
            /* zone 9 (arb) */
            -8, 33, 34, 35, 36, 37, 38,
        
            /* zone 10 (arb) */
            -22, -38, 39, 40, 41, 42
            
        };
        
        /* order-randomized facelist */
        int facelist2[] = {
            /* zone 0 (hex) */
            3, 0, 4, 1, 2, 5,
        
            /* zone 1 (hex) */
            6, 8, 10, 7, 9, -5,
        
            /* zone 2 (pyramid) */
            -6, 12, 13, 14, 11, 
        
            /* zone 3 (pyramid) */
            15, 16, 17, 18, -14,
        
            /* zone 4 (wedge) */
            -7, 19, -12, 20, 21,
        
            /* zone 5 (pyramid) */
            23, -19, 24, 22, -15,
        
            /* zone 6 (wedge) */
            -24, -21, 25, 26, 27,
        
            /* zone 7 (tet) */
            28, 29, 30, -18,
        
            /* zone 8 (tet) */
            31, 32, -31, -25,
        
            /* zone 9 (arb) */
            38, 36, 34, -8, 35, 37, 33,
        
            /* zone 10 (arb) */
            40, 42, -38, 39, 41, -22
        };
        float nvar[] = {  1,   1,   1,
                        1.1, 1.1, 1.1,
                        3.5, 3.5, 3.5,
                        3.6, 3.6, 3.6,
                        9.0, 9.0, 9.0,
                        9.1, 9.1, 9.1,
                        27,
                        1.1, 1.05,
                        1.5,
                        3.6, 3.5,
                        9.1, 8.5, 8.45, 8.4}; 
                     /* 0  1  2   3   4   5   6  7  8   9    10 */
        float zvar[] = {4, 5, 5, 5.5, 6, 6.5, 6, 6, 5, 5.5, 5.7};
        float zvar2[] = {4, 5, 5, 5.5, 5, 5.7, 5.5, 6, 6.5, 6, 6};

        float *x, *y, *z;
        int nzones, nnodes, lnodelist, nfaces, lfacelist;
    
        /* build coordinate arrays from icdata */
        nnodes = LEN(icdata) / 4;
        x = (float *) malloc (nnodes * sizeof(float));
        y = (float *) malloc (nnodes * sizeof(float));
        z = (float *) malloc (nnodes * sizeof(float));
        for (i = 0; i < nnodes; i++)
        {
            x[i] = (float) icdata[4*i+1];
            y[i] = (float) icdata[4*i+2];
            z[i] = (float) icdata[4*i+3];
        }
    
        /* build faces */
        nfaces = LEN(nodecnts);
        lnodelist = LEN(nodelist);
    
        nzones = LEN(facecnts);
        lfacelist = LEN(facelist);
    
        coords[0] = x;
        coords[1] = y;
        coords[2] = z;

        DBMkDir(dbfile, "3D");
        DBSetDir(dbfile, "3D");
    
        /* Use DBPutzonelist and DBPutZonelist2 */
        DBPutUcdmesh(dbfile, "mesh1", 3, (DBCAS_t) coordnames, coords, nnodes, nzones, "zl1", 0, DB_FLOAT, 0);
        DBPutUcdvar1(dbfile, "n1", "mesh1", nvar, nnodes, NULL, 0, DB_FLOAT, DB_NODECENT, NULL);
        DBPutUcdvar1(dbfile, "z1", "mesh1", zvar, nzones, NULL, 0, DB_FLOAT, DB_ZONECENT, NULL);
        DBPutUcdmesh(dbfile, "mesh2", 3, (DBCAS_t) coordnames, coords, nnodes, nzones, "zl2", 0, DB_FLOAT, 0);
        DBPutUcdvar1(dbfile, "n2", "mesh2", nvar, nnodes, NULL, 0, DB_FLOAT, DB_NODECENT, NULL);
        DBPutUcdvar1(dbfile, "z2", "mesh2", zvar, nzones, NULL, 0, DB_FLOAT, DB_ZONECENT, NULL);
        DBPutUcdmesh(dbfile, "mesh2g", 3, (DBCAS_t) coordnames, coords, nnodes, nzones, "zl2g", 0, DB_FLOAT, 0);
        DBPutUcdvar1(dbfile, "n2g", "mesh2g", nvar, nnodes, NULL, 0, DB_FLOAT, DB_NODECENT, NULL);
        DBPutUcdvar1(dbfile, "z2g", "mesh2g", zvar, nzones, NULL, 0, DB_FLOAT, DB_ZONECENT, NULL);
        {
            /* Note: these are intentionally not the same order as for phzl */
            int nl[] = {
                /* first two hexes */
                0,3,4,1,6,9,10,7,
                1,4,5,2,7,10,11,8,

                /* 2 wedges */
                7,8,11,10,16,17,
                13,14,8,7,16,17,

                /* 2 tets */
                15,12,16,18,
                12,13,16,18,

                /* 2 arbitrary */
                7, /* 38 slots */
                3,  20,21,19,
                5,  19,5,2,27,20,
                5,  27,23,22,21,20,
                5,  19,21,22,11,5,
                4,  2,5,11,8,
                4,  8,11,22,23,
                4,  2,8,23,27,

                6, /* 31 slots */
                4,  23,26,25,22,
                5,  22,25,24,17,11,
                3,  8,11,17,
                4,  8,23,22,11,
                3,  26,24,25,
                5,  8,17,24,26,23,

                /* 3 pyramids */
                6,9,10,7,16,
                6,12,15,9,16,
                7,13,12,6,16};

            int shapecnt[] = {2, 2, 2, 2, 3};
            int shapesize[] = {8, 6, 4, 0, 5};
            int shapesize2[] = {8, 6, 4, 69, 5};
            int shapetype[] = {
                DB_ZONETYPE_HEX,
                DB_ZONETYPE_PRISM,
                DB_ZONETYPE_TET,
                DB_ZONETYPE_POLYHEDRON,
                DB_ZONETYPE_PYRAMID};

            DBSetDeprecateWarnings(0);
            DBPutZonelist(dbfile, "zl1", nzones, 3, nl, LEN(nl), 0, shapesize,
                shapecnt, 5);
            DBSetDeprecateWarnings(3);
            DBPutZonelist2(dbfile, "zl2", nzones, 3, nl, LEN(nl), 0, 0, 0, shapetype,
                shapesize2, shapecnt, 5, 0);
            DBPutZonelist2(dbfile, "zl2g", nzones, 3, nl, LEN(nl), 0, 1, 1, shapetype,
                shapesize2, shapecnt, 5, 0);
        }

        ol = DBMakeOptlist(3);
        DBAddOption(ol, DBOPT_PHZONELIST, (char *) "phzl");
        DBPutUcdmesh(dbfile, "mesh3", 3, (DBCAS_t) coordnames, coords, nnodes, nzones, 0, 0, DB_FLOAT, ol);
        DBFreeOptlist(ol);
        DBPutUcdvar1(dbfile, "n3", "mesh3", nvar, nnodes, NULL, 0, DB_FLOAT, DB_NODECENT, NULL);
        DBPutUcdvar1(dbfile, "z3", "mesh3", zvar2, nzones, NULL, 0, DB_FLOAT, DB_ZONECENT, NULL);
    
        DBPutPHZonelist(dbfile, "phzl",
            nfaces, nodecnts, lnodelist, nodelist, 0,
            nzones, facecnts, lfacelist, facelist, 0,
            0, nzones-1, 0);
    
        ol = DBMakeOptlist(3);
        DBAddOption(ol, DBOPT_PHZONELIST, (char *) "phzl_r");
        DBPutUcdmesh(dbfile, "mesh3r", 3, (DBCAS_t) coordnames, coords, nnodes, nzones, 0, 0, DB_FLOAT, ol);
        DBFreeOptlist(ol);
        DBPutUcdvar1(dbfile, "n3r", "mesh3r", nvar, nnodes, NULL, 0, DB_FLOAT, DB_NODECENT, NULL);
        DBPutUcdvar1(dbfile, "z3r", "mesh3r", zvar2, nzones, NULL, 0, DB_FLOAT, DB_ZONECENT, NULL);
    
        DBPutPHZonelist(dbfile, "phzl_r",
            nfaces, nodecnts, lnodelist, nodelist2, 0,
            nzones, facecnts, lfacelist, facelist2, 0,
            1, 9, 0);

        DBSetDir(dbfile, "..");

        free(x);
        free(y);
        free(z);
    }

    DBClose(dbfile);

    /* Read zonelists back and validate them now */
    {
        char *zlnames[] = {"2D/zl1", "2D/zl2", "2D/zlg2", "2Dz/zl1", "2Dz/zl2", "2Dz/zlg2", "3D/zl1", "3D/zl2", "3D/zl2g"};
        dbfile = DBOpen(filename, driver, DB_READ);
        for (int i = 0; i < sizeof(zlnames)/sizeof(zlnames[0]); i++)
        {
            DBzonelist *zl = DBGetZonelist(dbfile, zlnames[i]);
            int zlvalid = DBValidateZonelist(zl, -1);
            if (zlvalid != DB_VALIDATE_GOOD)
            {
	        fprintf(stderr, "DBValidateZonelist(\"%s\", -1) fails (returns %d).\n", zlnames[i], zlvalid);
                retval = 1;
            }
            DBFreeZonelist(zl);
        } 
        DBClose(dbfile);
    }

    CleanupDriverStuff();

    return retval;
}
