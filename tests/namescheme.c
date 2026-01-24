/*****************************************************************************
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
*****************************************************************************/

/*
 
  Modifications:
    Mark C. Miller, Wed Jul 14 15:30:09 PDT 2010
    I changed '$' external arrays to '#' (int valued) extern and added a new
    test for '$' (string valued) external arrays. I also added an example
    for the kinds of nameschemes as might be used for mult-block objects. 
*/

#include <silo.h>
#include <std.c>
#include <string.h>
#include <limits.h>

#define TEST_GET_NAME(NS,I,EXP)                                                                            \
if (!NS)                                                                                                   \
{                                                                                                          \
    fprintf(stderr, "Got NULL namescheme from DBMakeNamescheme at line %d\n", __LINE__);                   \
    return 1;                                                                                              \
}                                                                                                          \
else                                                                                                       \
{                                                                                                          \
    if (strcmp(DBGetName(NS, I), EXP) != 0)                                                                \
    {                                                                                                      \
        fprintf(stderr, "Namescheme at line %d failed for index %d. Expected \"%s\", got \"%s\"\n", \
            __LINE__, I, EXP, DBGetName(NS, I));                                                           \
        return 1;                                                                                          \
    }                                                                                                      \
}

/* Uses a hard-coded min field width of 3 */
#define TEST_GET_INDEX(STR, FLD, BASE, EXP)                                                                \
if (DBGetIndex(STR, FLD, 3, BASE) != (long long) EXP)                                                      \
{                                                                                                          \
    fprintf(stderr, "DBGetIndex() at line %d failed for field %d of \"%s\". Expected %lld, got %lld\n",    \
        __LINE__, FLD, STR, (long long) EXP, DBGetIndex(STR,FLD,3,BASE));                                  \
    return 1;                                                                                              \
}

#define TEST_STR(A,B)                                                                                      \
if (strcmp(A,B))                                                                                           \
{                                                                                                          \
    fprintf(stderr, "String comparison failed at line %d. Expected \"%s\", got \"%s\"\n",                  \
            __LINE__, A, B);                                                                               \
    return 1;                                                                                              \
}

#define FAILED(MSG)                                                                                        \
{                                                                                                          \
    fprintf(stderr, "%s at line %d.\n", MSG, __LINE__);                                                    \
    return 1;                                                                                              \
}

int main(int argc, char **argv)
{
    int i;
    int P[100], U[4], PFS[4] = {0,1,2,3};
    char const * const N[3] = {"red","green","blue"};
    char blockName[1024];
    int driver = DB_PDB;
    int eval_ns = 0;
    int show_all_errors = 0;
	DBnamescheme *ns, *ns2;
    char teststr[256];

    for (i=1; i<argc; i++) {
        if (!strncmp(argv[i], "DB_PDB", 6)) {
            driver = StringToDriver(argv[i]);
        } else if (!strncmp(argv[i], "DB_HDF5", 7)) {
            driver = StringToDriver(argv[i]);
        } else if (!strcmp(argv[i], "show-all-errors")) {
            show_all_errors = 1;
        } else if (!strcmp(argv[i], "eval-ns")) { /* test evaluate nameschemes */
            eval_ns = 1;
        } else if (argv[i][0] != '\0') {
            fprintf(stderr, "%s: ignored argument `%s'\n", argv[0], argv[i]);
        }
    }

    DBShowErrors(show_all_errors?DB_ALL_AND_DRVR:DB_ABORT, NULL);

    /* test namescheme with constant componets with and without delimters */
    {
        ns = DBMakeNamescheme("foo/bar/gorfo_0");
        TEST_GET_NAME(ns, 0, "foo/bar/gorfo_0");
        TEST_GET_NAME(ns, 1, "foo/bar/gorfo_0");
        TEST_GET_NAME(ns, 122, "foo/bar/gorfo_0");
        DBFreeNamescheme(ns);
        ns = DBMakeNamescheme("@foo/bar/gorfo_0@");
        TEST_GET_NAME(ns, 0, "foo/bar/gorfo_0");
        TEST_GET_NAME(ns, 1, "foo/bar/gorfo_0");
        TEST_GET_NAME(ns, 122, "foo/bar/gorfo_0");
        DBFreeNamescheme(ns);
        ns = DBMakeNamescheme("@/Density@");
        TEST_GET_NAME(ns, 0, "/Density");
        TEST_GET_NAME(ns, 1, "/Density");
        TEST_GET_NAME(ns, 513, "/Density");
        TEST_GET_NAME(ns, 134571, "/Density");
        DBFreeNamescheme(ns);
        ns = DBMakeNamescheme("radar");
        TEST_GET_NAME(ns, 0, "radar");
        TEST_GET_NAME(ns, 1, "radar");
        TEST_GET_NAME(ns, 137, "radar");
        DBFreeNamescheme(ns);
        ns = DBMakeNamescheme("/radar/");
        TEST_GET_NAME(ns, 0, "/radar/");
        TEST_GET_NAME(ns, 1, "/radar/");
        TEST_GET_NAME(ns, 137, "/radar/");
        DBFreeNamescheme(ns);
        ns = DBMakeNamescheme("-radar-");
        TEST_GET_NAME(ns, 0, "radar");
        TEST_GET_NAME(ns, 1, "radar");
        TEST_GET_NAME(ns, 137, "radar");
        DBFreeNamescheme(ns);
        ns = DBMakeNamescheme("_radar_");
        TEST_GET_NAME(ns, 0, "_radar_");
        TEST_GET_NAME(ns, 1, "_radar_");
        TEST_GET_NAME(ns, 137, "_radar_");
        DBFreeNamescheme(ns);
        ns = DBMakeNamescheme("_radar");
        TEST_GET_NAME(ns, 0, "_radar");
        TEST_GET_NAME(ns, 1, "_radar");
        TEST_GET_NAME(ns, 137, "_radar");
        DBFreeNamescheme(ns);
        ns = DBMakeNamescheme("radar_");
        TEST_GET_NAME(ns, 0, "radar_");
        TEST_GET_NAME(ns, 1, "radar_");
        TEST_GET_NAME(ns, 137, "radar_");
        DBFreeNamescheme(ns);
        ns = DBMakeNamescheme("|radar|");
        TEST_GET_NAME(ns, 0, "radar");
        TEST_GET_NAME(ns, 1, "radar");
        TEST_GET_NAME(ns, 137, "radar");
        DBFreeNamescheme(ns);
        ns = DBMakeNamescheme("|radar");
        TEST_GET_NAME(ns, 0, "radar");
        TEST_GET_NAME(ns, 1, "radar");
        TEST_GET_NAME(ns, 137, "radar");
        DBFreeNamescheme(ns);
        ns = DBMakeNamescheme("radar|");
        TEST_GET_NAME(ns, 0, "radar");
        TEST_GET_NAME(ns, 1, "radar");
        TEST_GET_NAME(ns, 137, "radar");
        DBFreeNamescheme(ns);
        ns = DBMakeNamescheme("@radar");
        TEST_GET_NAME(ns, 0, "radar");
        TEST_GET_NAME(ns, 1, "radar");
        TEST_GET_NAME(ns, 137, "radar");
        DBFreeNamescheme(ns);
        ns = DBMakeNamescheme("radar@");
        TEST_GET_NAME(ns, 0, "radar");
        TEST_GET_NAME(ns, 1, "radar");
        TEST_GET_NAME(ns, 137, "radar");
        DBFreeNamescheme(ns);
    }

    /* Test a somewhat complex expression */ 
    ns = DBMakeNamescheme("@foo_%+03d@3-((n % 3)*(4+1)+1/2)+1");
    TEST_GET_NAME(ns, 25, "foo_+01");
    DBFreeNamescheme(ns);

    /* test returned string is different for successive calls (Dan Laney bug) */
    ns = DBMakeNamescheme("@/foo/bar/proc-%d@n");
    snprintf(teststr, sizeof(teststr), "%s %s", DBGetName(ns,0), DBGetName(ns,123));
    TEST_STR(teststr, "/foo/bar/proc-0 /foo/bar/proc-123")
    DBFreeNamescheme(ns);

    /* Test ?:: operator */
    ns = DBMakeNamescheme("@foo_%d@(n-5)?14:77:");
    TEST_GET_NAME(ns, 5, "foo_77");
    TEST_GET_NAME(ns, 6, "foo_14");
    DBFreeNamescheme(ns);

    /* Test multiple conversion specifiers */
    ns = DBMakeNamescheme("|foo_%03dx%03d|n/5|n%5");
    TEST_GET_NAME(ns, 17, "foo_003x002");
    TEST_GET_NAME(ns, 20, "foo_004x000");
    TEST_GET_NAME(ns, 3, "foo_000x003");
    DBFreeNamescheme(ns);

    /* Test embedded string value results */
    ns = DBMakeNamescheme("@foo_%s@(n-5)?'leader':'follower':");
    TEST_GET_NAME(ns, 4, "foo_leader");
    TEST_GET_NAME(ns, 5, "foo_follower");
    TEST_GET_NAME(ns, 6, "foo_leader");
    DBFreeNamescheme(ns);

    /* Test array-based references to int valued arrays and whose
       array names are more than just one character long. */
    for (i = 0; i < 100; i++)
        P[i] = i*5;
    for (i = 0; i < 4; i++)
        U[i] = i*i;
    ns = DBMakeNamescheme("@foo_%03dx%03d@#Place[n]@#Upper[n%4]", P, U);
    TEST_GET_NAME(ns, 17, "foo_085x001");
    TEST_GET_NAME(ns, 18, "foo_090x004");
    TEST_GET_NAME(ns, 19, "foo_095x009");
    TEST_GET_NAME(ns, 20, "foo_100x000");
    TEST_GET_NAME(ns, 21, "foo_105x001");
    DBFreeNamescheme(ns);

    /* Test array-based references to char* valued array */
    ns = DBMakeNamescheme("Hfoo_%sH$Noodle[n%3]", N);
    TEST_GET_NAME(ns, 17, "foo_blue");
    TEST_GET_NAME(ns, 6, "foo_red");
    DBFreeNamescheme(ns);

    /* Test namescheme as it might be used for multi-block objects */
    /* In particular, this test creates nameschems suitable for the data
       produced by 'multi_file' test in multidir mode. */
    ns = DBMakeNamescheme("|multi_file.dir/%03d/%s%d.%s|n/36|'ucd3d'|n/36|'pdb'");
    ns2 = DBMakeNamescheme("|/block%d/mesh1|n");
    strcpy(blockName, DBGetName(ns, 123)); /* filename part */
    strcat(blockName, ":");
    strcat(blockName, DBGetName(ns2, 123)); /* blockname part */
    TEST_STR(blockName, "multi_file.dir/003/ucd3d3.pdb:/block123/mesh1")
    strcpy(blockName, DBGetName(ns, 0)); /* filename part */
    strcat(blockName, ":");
    strcat(blockName, DBGetName(ns2, 0)); /* blockname part */
    TEST_STR(blockName, "multi_file.dir/000/ucd3d0.pdb:/block0/mesh1")
    strcpy(blockName, DBGetName(ns, 287)); /* filename part */
    strcat(blockName, ":");
    strcat(blockName, DBGetName(ns2, 287)); /* blockname part */
    TEST_STR(blockName, "multi_file.dir/007/ucd3d7.pdb:/block287/mesh1")
    DBFreeNamescheme(ns);
    DBFreeNamescheme(ns2);

    /* Test DBnamescheme construction from arrays in a Silo file */
    {
        DBfile *dbfile;
        int dims[3];
        int strListLen;
        char const * const FileNumbers[] = {"1","2","3"};

        /* uses same namescheme as above but am placing arrays in different dir
           relative to where I will place the namesheme */
        char *ns1 = "@foo_%03dx%03d@#../arr_dir/Place[n]@#../arr_dir/Upper[n%4]";
        char *ns1r;
        /* Use absolute path to external array; 'H' is delim char */
        char *ns2 = "Hfoo_%sH$/arr_dir/Noodle[n%3]";
        char *ns2r;
        char *strList = 0;
        /* use paths relative to the MB mesh object in which the nameschems
           are embedded */
        char *ns3 = "|/meshes/mesh1/dom_%s_%d|$../ns_arrays/Noodle[n]|n*2+1";
        char *ns3r;

        /* Test McCandless' namescheme example */
        char *ns4 = "@%s%s@(n/4)?'myfilename.':'':@(n/4)?$/arr_dir/FileNumbers[n/4-1]:'':";
        char *ns4r;

        dbfile = DBCreate("namescheme.silo", DB_CLOBBER, DB_LOCAL,
            "Test namescheme constructor with external arrays in file", driver);

        /* Put the external arrays in arr_dir */
        DBMkDir(dbfile, "arr_dir");
        DBSetDir(dbfile, "arr_dir");
        dims[0] = 100;
        DBWrite(dbfile, "Place", P, dims, 1, DB_INT);
        dims[0] = 4;
        DBWrite(dbfile, "Upper", U, dims, 1, DB_INT);
        DBStringArrayToStringList(FileNumbers, 3, &strList, &strListLen);
        dims[0] = strListLen;
        DBWrite(dbfile, "FileNumbers", strList, dims, 1, DB_CHAR);
        free(strList);
        DBStringArrayToStringList(N, 3, &strList, &strListLen);
        dims[0] = strListLen;
        DBWrite(dbfile, "Noodle", strList, dims, 1, DB_CHAR);
        DBSetDir(dbfile, "..");

        DBMkDir(dbfile, "dir_1");
        DBSetDir(dbfile, "dir_1");
        dims[0] = strlen(ns1)+1;
        DBWrite(dbfile, "ns1", ns1, dims, 1, DB_CHAR);
        DBSetDir(dbfile, "..");
        DBMkDir(dbfile, "dir_2");
        DBMkDir(dbfile, "dir_2/dir_3");
        DBSetDir(dbfile, "/dir_2/dir_3");
        dims[0] = strlen(ns2)+1;
        DBWrite(dbfile, "ns2", ns2, dims, 1, DB_CHAR);
        dims[0] = strlen(ns4)+1;
        DBWrite(dbfile, "ns4", ns4, dims, 1, DB_CHAR);

        DBSetDir(dbfile, "/");
        DBMkDir(dbfile, "meshes");
        DBSetDir(dbfile, "meshes");
        DBMkDir(dbfile, "ns_arrays");
        dims[0] = strListLen;
        DBWrite(dbfile, "ns_arrays/Noodle", strList, dims, 1, DB_CHAR);
        free(strList);

        DBMkDir(dbfile, "mesh1");
        DBSetDir(dbfile, "mesh1");
        DBMkDir(dbfile, "dom_red_1");
        DBMkDir(dbfile, "dom_green_3");
        DBMkDir(dbfile, "dom_blue_5");
        DBSetDir(dbfile, "dom_red_1");
        {
            int ndims = 2;
            int dims[] = {4,2};
            float xcoords[] = {0, 1, 2, 3};
            float ycoords[] = {0, 1};
            float *coords[2] = {xcoords, ycoords};
            DBPutQuadmesh(dbfile, "qmesh", 0, coords, dims, ndims, DB_FLOAT, DB_COLLINEAR, 0);
        }
        DBSetDir(dbfile, "../dom_green_3");
        {
            int ndims = 2;
            int dims[] = {4,2};
            float xcoords[] = {0, 1, 2, 3};
            float ycoords[] = {1, 2};
            float *coords[2] = {xcoords, ycoords};
            DBPutQuadmesh(dbfile, "qmesh", 0, coords, dims, ndims, DB_FLOAT, DB_COLLINEAR, 0);
        }
        DBSetDir(dbfile, "../dom_blue_5");
        {
            int ndims = 2;
            int dims[] = {4,2};
            float xcoords[] = {0, 1, 2, 3};
            float ycoords[] = {2, 3};
            float *coords[2] = {xcoords, ycoords};
            DBPutQuadmesh(dbfile, "qmesh", 0, coords, dims, ndims, DB_FLOAT, DB_COLLINEAR, 0);
        }
        DBSetDir(dbfile, "..");
        {
            int btype = DB_QUADMESH;
            DBoptlist *optlist = DBMakeOptlist(2);
            DBAddOption(optlist, DBOPT_MB_BLOCK_NS, ns3);
            DBAddOption(optlist, DBOPT_MB_BLOCK_TYPE, &btype);
            DBPutMultimesh(dbfile, "mmesh", 3, 0, 0, optlist);
            DBFreeOptlist(optlist);
        }
        DBClose(dbfile);

        dbfile = DBOpen("namescheme.silo", DB_UNKNOWN, DB_READ);
        DBSetDir(dbfile, "dir_1");
        ns1r = (char*)DBGetVar(dbfile, "ns1");

        /* Use the '0, DBfile*, 0' form of args to constructor */
        ns = DBMakeNamescheme(ns1r, 0, dbfile, 0);

        /* Ok, lets test the constructed namescheme */
        TEST_GET_NAME(ns, 17, "foo_085x001");
        TEST_GET_NAME(ns, 18, "foo_090x004");
        TEST_GET_NAME(ns, 19, "foo_095x009");
        TEST_GET_NAME(ns, 20, "foo_100x000");
        TEST_GET_NAME(ns, 21, "foo_105x001");
        DBFreeNamescheme(ns);
        free(ns1r);

        DBSetDir(dbfile, "/dir_2/dir_3");
        ns2r = (char *)DBGetVar(dbfile, "ns2");

        /* Use the '0, DBfile*, 0' form of args to constructor */
        ns = DBMakeNamescheme(ns2r, 0, dbfile, 0);

        TEST_GET_NAME(ns, 17, "foo_blue");
        TEST_GET_NAME(ns, 6, "foo_red");
        DBFreeNamescheme(ns);
        free(ns2r);

        /* Test invalid namescheme construction */
        ns = DBMakeNamescheme("@foo/bar/gorfo_%d@#n");
        if (ns) FAILED("DBMakeNamescheme succeeded on an invalid string");
        ns = DBMakeNamescheme("@foo/bar/gorfo_%d@#n", 0, dbfile, 0);
        if (ns) FAILED("DBMakeNamescheme succeeded on an invalid string");

        /* Test construction via retrieval from MB object */
        DBSetDir(dbfile, "/meshes/mesh1");
        {
            DBmultimesh *mm = DBGetMultimesh(dbfile, "mmesh");
            ns = DBMakeNamescheme(mm->block_ns, 0, dbfile, "/meshes/mesh1");
            TEST_GET_NAME(ns, 0, "/meshes/mesh1/dom_red_1");
            TEST_GET_NAME(ns, 1, "/meshes/mesh1/dom_green_3");
            TEST_GET_NAME(ns, 2, "/meshes/mesh1/dom_blue_5");
            DBFreeNamescheme(ns);
            DBFreeMultimesh(mm);
        }

        /* Test McCandless' example */
        DBSetDir(dbfile, "/");
        ns4r = (char *)DBGetVar(dbfile, "/dir_2/dir_3/ns4");
        ns = DBMakeNamescheme(ns4r, 0, dbfile, 0);
        TEST_GET_NAME(ns, 0, "");
        TEST_GET_NAME(ns, 1, "");
        TEST_GET_NAME(ns, 4, "myfilename.1");
        TEST_GET_NAME(ns, 15, "myfilename.3");
        DBFreeNamescheme(ns);
        free(ns4r);

        DBClose(dbfile);
    }

    /* Test McCandless' example (new way) */
    ns = DBMakeNamescheme("@%s@(n/4)?'&myfilename.%d&n/4':'':@");
    TEST_GET_NAME(ns, 0, "");
    TEST_GET_NAME(ns, 1, "");
    TEST_GET_NAME(ns, 4, "myfilename.1");
    TEST_GET_NAME(ns, 15, "myfilename.3");
    DBFreeNamescheme(ns);

    /* Text Exodus material volume fraction variable convention */
    ns = DBMakeNamescheme("@%s@n?'&VOLFRC_%d&n':'VOID_FRC':@");
    TEST_GET_NAME(ns, 0, "VOID_FRC");
    TEST_GET_NAME(ns, 1, "VOLFRC_1");
    TEST_GET_NAME(ns, 2, "VOLFRC_2");
    TEST_GET_NAME(ns, 10, "VOLFRC_10");
    TEST_GET_NAME(ns, 2746, "VOLFRC_2746");
    DBFreeNamescheme(ns);

    /* Test Al Nichol's case of using same external array multiple times */
    ns = DBMakeNamescheme("|chemA_016_00000%s%.0d|#PFS[(n/4) % 4]?'.':'':|#PFS[(n/4) % 4]", PFS);
    TEST_GET_NAME(ns, 0, "chemA_016_00000");
    TEST_GET_NAME(ns, 1, "chemA_016_00000");
    TEST_GET_NAME(ns, 2, "chemA_016_00000");
    TEST_GET_NAME(ns, 3, "chemA_016_00000");
    TEST_GET_NAME(ns, 4, "chemA_016_00000.1");
    TEST_GET_NAME(ns, 5, "chemA_016_00000.1");
    TEST_GET_NAME(ns, 8, "chemA_016_00000.2");
    TEST_GET_NAME(ns, 11, "chemA_016_00000.2");
    TEST_GET_NAME(ns, 15, "chemA_016_00000.3");
    DBFreeNamescheme(ns);

    /* Test using namescheme as a simple integer mapping */
    ns = DBMakeNamescheme("|chemA_0x%04X|n");
    TEST_GET_INDEX(DBGetName(ns,  1), 0, 0, 1);
    TEST_GET_INDEX(DBGetName(ns, 50), 0, 0, 50);
    TEST_GET_INDEX(DBGetName(ns, 37), 0, 0, 37);
    DBFreeNamescheme(ns);

    /* simple offset by -2 mapping */
    ns = DBMakeNamescheme("|%04d_domain|n-2");
    TEST_GET_INDEX(DBGetName(ns,0), 0, 0, -2);
    TEST_GET_INDEX(DBGetName(ns,1), 0, 0, -1);
    TEST_GET_INDEX(DBGetName(ns,2), 0, 0,  0);
    TEST_GET_INDEX(DBGetName(ns,3), 0, 0,  1);
    TEST_GET_INDEX(DBGetName(ns,4), 0, 0,  2);
    DBFreeNamescheme(ns);

    /* Get different fields as indices from nameschemed strings */
    ns = DBMakeNamescheme("|foo_%03d_%03d|n/5|n%5");
    TEST_GET_INDEX(DBGetName(ns,0),  0, 0, 0);
    TEST_GET_INDEX(DBGetName(ns,0),  1, 0, 0);
    TEST_GET_INDEX(DBGetName(ns,18), 0, 0, 3);
    TEST_GET_INDEX(DBGetName(ns,17), 1, 0, 2);
    DBFreeNamescheme(ns);

    /* Case where index is bigger than an int */
    ns = DBMakeNamescheme("|block_0x%llX|n");
    TEST_GET_INDEX(DBGetName(ns, 0x7FFFFFFF), 0, 0,  0x7FFFFFFF); /* max for an int */
    TEST_GET_INDEX(DBGetName(ns,0x7FFFFFFFF), 0, 0, 0x7FFFFFFFF); /* make sure another `F` works */
    DBFreeNamescheme(ns);

    /* Test inferring base 2 (binary, leading '0b') */
    TEST_GET_INDEX("block_0b0101", 0, 0, 5);
    TEST_GET_INDEX("block_0b0101_0b1100", 1, 0, 12);

    /* Test inferring base 8 (octal, leading '0') */
    TEST_GET_INDEX("slice1_035", 0, 0, 29);

    /* Test non-standard base 5 */
    TEST_GET_INDEX("VisIt_docs_section_0002_chapter_1234", 1, 5, 194);

    /* Test negative values */
    TEST_GET_INDEX("block_-0b0101", 0, 0, -5);

    /* Test some cases that could lead to error */
    TEST_GET_INDEX("block_0b0", 1, 0, LLONG_MAX);
    TEST_GET_INDEX("block_+", 1, 0, LLONG_MAX);
    TEST_GET_INDEX("block_0x", 1, 0, LLONG_MAX);
    TEST_GET_INDEX("VisIt_docs_section_0002_chapter_1234", 5, 5, LLONG_MAX);

    /* Test the convenience method, DBSPrintf */
    snprintf(teststr, sizeof(teststr), "%s, %s",
        DBSPrintf("block_%d,level_%04d", 505, 17),
        DBSPrintf("side_%s_%cx%g", "leader",'z',1.0/3));
    TEST_STR(teststr, "block_505,level_0017, side_leader_zx0.333333")

    /* Test case where fewer expressions than conversion specs */
    ns = DBMakeNamescheme("|/domain_%03d/laser_beam_power_%d|n/1|");
    if (ns) FAILED("DBMakeNamescheme succeeded on an invalid string");

    /* Test DBEvalNamescheme() logic. Requires multi_file test to have run
       with `use-ns` command-line option. This will have produced a root
       file with multiblock objects in it using nameschemes. */
    if (eval_ns)
    {
        /* Because of how Autotool's testing works, the test may be run next to
           or two directories below where the files exist. And, we have to handle
           either the hdf5 or the pdb variants multi_file can produce. */
        char const *rootFiles[] = {"ucd3d_root.pdb", "ucd3d_root.h5",
            "../../ucd3d_root.pdb", "../../ucd3d_root.h5"};
        int const nRootCandidates = sizeof(rootFiles)/sizeof(rootFiles[0]);

        for (i = 0; i < nRootCandidates; i++)
        {
            DBfile *dbfile;
            DBmultimesh *mm_w_ns, *mm;
            DBmultimat *mmat;
            DBnamescheme *fns, *bns;

            if (DBStat(rootFiles[i]) == -1) continue;

            /* Get a multi-block object from the root file without
               evaluating nameschemes */
            dbfile = DBOpen(rootFiles[i], DB_UNKNOWN, DB_READ);
            mm_w_ns = DBGetMultimesh(dbfile, "mesh1");
            DBClose(dbfile);

            /* if the acquired multiblock object is not using nameschemes,
               we cannot perform this test */
            if (mm_w_ns->file_ns == 0 || mm_w_ns->block_ns == 0)
                FAILED("selected root file candidate has no nameschemes.");

            fns = DBMakeNamescheme(mm_w_ns->file_ns);
            bns = DBMakeNamescheme(mm_w_ns->block_ns);

            /* Test the utility function DBGenerateMBBlockName */
            if (driver == DB_PDB)
            {
                TEST_STR(DBGenerateMBBlockName(17, fns, bns, 0, 0), "ucd3d0.pdb:/block17/mesh1");
            }
            else
            {
                TEST_STR(DBGenerateMBBlockName(17, fns, bns, 0, 0), "ucd3d0.h5:/block17/mesh1");
            }

            /* Get some multi-block objects from the root file while also
               evaluating nameschemes */
            dbfile = DBOpen(rootFiles[i], DB_UNKNOWN, DB_READ);
            DBSetEvalNameschemesFile(dbfile,1);
            mm = DBGetMultimesh(dbfile, "mesh1");
            mmat = DBGetMultimat(dbfile, "mat1");
            DBClose(dbfile);

            /* if the acquired multiblock object has nameschemes, the test fails */
            if (mm->file_ns != 0 || mm->block_ns != 0)
                FAILED("Requested eval nameschemes but still get them.");
            if (mmat->file_ns != 0 || mmat->block_ns != 0)
                FAILED("Requested eval nameschemes but still get them.");

            /* Ok, now lets generate a Silo object block name from mm_w_ns and compare it
               to what we get for the same block in mm (where nameschemes were evaluated). */
            snprintf(teststr, sizeof(teststr), "%s:%s", DBGetName(fns,5), DBGetName(bns,5));
            TEST_STR(teststr, mm->meshnames[5]);
            snprintf(teststr, sizeof(teststr), "%s:%s", DBGetName(fns,73), DBGetName(bns,73));
            TEST_STR(teststr, mm->meshnames[73]);
            DBFreeNamescheme(fns);
            DBFreeNamescheme(bns);

            /* Just explicitly test an entry in a multimat */
            if (driver == DB_PDB)
            {
                TEST_STR("ucd3d2.pdb:/block73/mat1", mmat->matnames[73]);
            }
            else
            {
                TEST_STR("ucd3d2.h5:/block73/mat1", mmat->matnames[73]);
            }

            DBFreeMultimesh(mm_w_ns);
            DBFreeMultimesh(mm);
            DBFreeMultimat(mmat);

            /* Arriving here at the end of the loop's body, we're done with the test */
            break; 
        }
        if (i == nRootCandidates)
            FAILED("test-eval requested but no root file candidates found");
    }
    
    /* hackish way to cleanup the circular cache used internally */
    DBSPrintf(0);
    DBGetName(0,-1);

    CleanupDriverStuff();

    return 0;
}
